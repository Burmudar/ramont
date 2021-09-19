package ramont

import (
	"encoding/json"
	"log"
	"sync"
	"sync/atomic"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/gorilla/websocket"
)

var globalServerStore *serverStore

type EventHandelerFunc func(event *Event) error

func initServerStore() {
	globalServerStore = &serverStore{store: make(map[uint32]Server)}
}

type Server interface {
	AssignID(id uint32)
	ID() uint32
	Start()
}

type WebSocketServer struct {
	id            uint32
	transport     Transport
	dataHandlerFn DataHandelerFunc
}

type serverStore struct {
	store map[uint32]Server
	mutex sync.Mutex
	guid  uint32
}

func (str *serverStore) genId() uint32 {
	atomic.AddUint32(&str.guid, 1)
	return str.currId()
}

func (str *serverStore) currId() uint32 {
	return atomic.LoadUint32(&str.guid)
}

func (str *serverStore) register(s Server) {
	str.mutex.Lock()
	defer str.mutex.Unlock()
	s.AssignID(str.genId())
	str.store[s.ID()] = s
}

func (str *serverStore) deregister(s Server) {
	str.mutex.Lock()
	defer str.mutex.Unlock()
	delete(str.store, s.ID())
}

func NewWebSocketServer(conn *websocket.Conn, handler DataHandelerFunc) Server {
	srv := WebSocketServer{}
	srv.transport = NewWebSocketTransport(conn, srv.HandleMessage)
	srv.dataHandlerFn = handler

	return &srv
}

func dataToEventWrap(eventHandler EventHandelerFunc) DataHandelerFunc {
	return func(dataType string, data []byte) error {
		if event, err := processEventData(dataType, data); err != nil {
			return nil
		} else {
			return eventHandler(&event)
		}
	}
}

func OnEventHandeler(handler EventHandelerFunc) func(ctx *gin.Context) {
	return func(ctx *gin.Context) {
		conn, err := UpgradeConn(ctx.Writer, ctx.Request)
		if err != nil {
			ctx.AbortWithStatusJSON(500, struct {
				Message string
			}{"failed to upgrade connection to support websockets"})
			return
		}

		srv := NewWebSocketServer(conn, dataToEventWrap(handler))

		if globalServerStore == nil {
			initServerStore()
		}

		globalServerStore.register(srv)
		log.Printf("Registered server: %v", srv.ID())

		srv.Start()
	}
}

func (s *WebSocketServer) AssignID(id uint32) {
	s.id = id

}

func (s *WebSocketServer) ID() uint32 {
	return s.id
}

func (s *WebSocketServer) reply(msg []byte) error {
	s.transport.Send(msg)
	return nil
}

func (s *WebSocketServer) processData(msgType string, data []byte) ([]byte, error) {
	err := s.dataHandlerFn(msgType, data)
	msg, _ := json.Marshal(struct {
		Type    string `json:"type"`
		ID      uint32 `json:"serverID"`
		Message string `json:"message"`
	}{"control", s.id, "received"})
	return msg, err
}

func (s *WebSocketServer) HandleMessage(msg *Message) error {
	if msg.Error != nil {
		log.Fatalf("Problem occured while receiving msg: %v", msg.Error)
	}

	bmsg := BasicMsg{}
	d := msg.Data
	err := json.Unmarshal(d, &bmsg)
	if err != nil {
		if string(msg.Data) == "ping" {
		} else {
			t := time.Now()
			log.Printf("Time: %v Error handling message - Unmarshall Error: '%v' Data: %v", t, err, string(msg.Data))
			return err
		}
	}

	switch bmsg.Type {
	case "basic":
		{
			msg, _ := json.Marshal(struct {
				Type    string `json:"type"`
				ID      uint32 `json:"serverID"`
				Message string `json:"message"`
			}{"control", s.id, "pong"})

			s.reply(msg)
		}
	default:
		{
			log.Printf("[INFO] Processing '%s' data", bmsg.Type)
			msg, err := s.processData(bmsg.Type, msg.Data)
			if err != nil {
				log.Printf("[ERROR] Problem processing event: %v", err)
			}
			log.Printf("[INFO] Replying -> %v", msg)
			s.reply(msg)
		}
	}

	return err
}

func (s *WebSocketServer) Start() {
	s.transport.Listen()

	go func() {
		select {
		case <-s.transport.CloseChan():
			{
				globalServerStore.deregister(s)
				log.Printf("Transport closed. Deregistered server: %v", s.id)
				log.Printf("Closing Ramont server")
				return
			}
		}
	}()
}

type BasicMsg struct {
	Type string `json:"type"`
}
