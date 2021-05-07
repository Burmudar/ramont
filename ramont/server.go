package ramont

import (
	"encoding/json"
	"log"
	"sync"
	"sync/atomic"

	"github.com/Burmudar/ramon-go/ramont/network"
	"github.com/Burmudar/ramon-go/ramont/websockets"
	"github.com/gin-gonic/gin"
	"github.com/gorilla/websocket"
)

var globalServerStore *serverStore

func initServerStore() {
	globalServerStore = &serverStore{store: make(map[uint32]Server)}
}

type Server interface {
	AssignID(id uint32)
	ID() uint32
	Start()
}

type WebSocketServer struct {
	id        uint32
	transport network.Transport
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

func NewWebSocketServer(conn *websocket.Conn) Server {
	srv := WebSocketServer{}
	srv.transport = websockets.NewTransport(conn, srv.HandleMessage)

	return &srv
}

func HandleFunc(ctx *gin.Context) {

	conn, err := websockets.UpgradeConn(ctx.Writer, ctx.Request)
	if err != nil {
		ctx.AbortWithStatusJSON(500, struct {
			Message string
		}{"failed to upgrade connection to support websockets"})
		return
	}

	srv := NewWebSocketServer(conn)

	if globalServerStore == nil {
		initServerStore()
	}

	globalServerStore.register(srv)
	log.Printf("Registered server: %v", srv.ID())

	srv.Start()
}

type EventType string

type MouseEvent struct {
	Type    EventType `json:"type"`
	OffsetX float64   `json:"offsetX"`
	OffsetY float64   `json:"offsetY"`
}

func unmarshalMouseEvent(eventType EventType, raw json.RawMessage) *MouseEvent {
	event := MouseEvent{Type: eventType}
	err := json.Unmarshal(raw, &event)
	log.Printf("Unmarshall Error: %v value: %v", err, string(raw))

	return &event
}

func (s *WebSocketServer) AssignID(id uint32) {
	s.id = id

}

func (s *WebSocketServer) ID() uint32 {
	return s.id
}

func (s *WebSocketServer) HandleMessage(msg *network.Message) error {
	if msg.Error != nil {
		log.Fatalf("Problem occured while receiving msg: %v", msg.Error)
	}

	bmsg := BasicMsg{}
	d := msg.Data
	err := json.Unmarshal(d, &bmsg)
	if err != nil {
		if string(msg.Data) == "ping" {
		}
		log.Printf("Error handling message - Unmarshall Error: '%v' Data: %v", err, string(msg.Data))
		return err
	}

	var event *MouseEvent
	switch bmsg.Type {
	case "mouse_start":
		{
			event = unmarshalMouseEvent("mouse_start", msg.Data)
		}
		break
	case "mouse_move":
		{
			event = unmarshalMouseEvent("mouse_move", msg.Data)
		}
		break
	case "mouse_end":
		{
			event = unmarshalMouseEvent("mouse_end", msg.Data)
		}
		break
	case "basic":
		{
			msg, _ := json.Marshal(struct {
				Type    string `json:"type"`
				ID      uint32 `json:"serverID"`
				Message string `json:"message"`
			}{"control", s.id, "pong"})

			s.transport.Send(msg)
		}
	default:
		{
			log.Printf("Unknown event: %v", string(msg.Data))
		}
	}

	if event != nil {
		log.Printf("Server: %v MouseEvent: %v", s.id, event)
		msg, _ := json.Marshal(struct {
			Type    string `json:"type"`
			ID      uint32 `json:"serverID"`
			Message string `json:"message"`
		}{"control", s.id, "received"})

		s.transport.Send(msg)
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
