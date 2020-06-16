package websockets

import (
	"fmt"
	"log"
	"net/http"

	"github.com/gin-gonic/gin"
	"github.com/gorilla/websocket"
)

type WebSocketContext struct {
	gin.Context
	Ws *websocket.Conn
}

type ValidateOriginFn func(r *http.Request) bool

var DefaultValidateOrigin ValidateOriginFn = AllOriginValid

func AllOriginValid(r *http.Request) bool {
	return true
}

type MsgHandler func(msg *Message) error

type Message struct {
	Type  uint8
	Data  []byte
	Error error
}

type Transport struct {
	conn       *websocket.Conn
	recvChan   chan *Message
	sendChan   chan []byte
	CloseChan  chan bool
	MsgHandler MsgHandler
}

func NewTransport(conn *websocket.Conn, msgHandler MsgHandler) *Transport {
	return &Transport{
		conn:       conn,
		recvChan:   make(chan *Message, 1),
		CloseChan:  make(chan bool),
		sendChan:   make(chan []byte),
		MsgHandler: msgHandler,
	}
}

func DefaultMsgHandler(msg *Message) error {
	log.Printf("[DEFAULT TEXT HANDLER] Received Text message: %v", msg)
	return nil
}

func (t *Transport) Hijack(ctx *gin.Context) error {
	if conn, err := UpgradeConn(ctx.Writer, ctx.Request); err != nil {
		return fmt.Errorf("Failed to upgrade connection to websockets: %v", err)
	} else {
		t.conn = conn
	}
	return nil
}

func UpgradeConn(w http.ResponseWriter, r *http.Request) (*websocket.Conn, error) {
	var upgrader = websocket.Upgrader{
		ReadBufferSize:  1024,
		WriteBufferSize: 1024,
		CheckOrigin:     DefaultValidateOrigin,
	}

	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		return nil, err
	}

	return conn, nil
}

//This should probably be moved out
func HandlerFunc(msgHandler MsgHandler) gin.HandlerFunc {
	return func(ctx *gin.Context) {
		t := NewTransport(nil, msgHandler)
		if err := t.Hijack(ctx); err != nil {
			resp := struct {
				Success bool   `json:"success"`
				Message string `json:"Message"`
			}{false, fmt.Sprintf("%s", err)}
			ctx.AsciiJSON(500, resp)
		}
		t.Listen()
	}
}

func (srv *Transport) Close() {
	close(srv.CloseChan)

}

func (t *Transport) Listen() {
	go t.receiveMessages()
	go t.writeMessages()
	go t.processMessages()
}

func (srv *Transport) Reply(msg []byte) {
	srv.sendChan <- msg
}

func (t *Transport) writeMessages() {
	log.Printf("Writing messages")
	for {
		select {
		case data := <-t.sendChan:
			{
				t.conn.WriteMessage(websocket.TextMessage, data)
			}
			break
		case <-t.CloseChan:
			{
				log.Printf("Closing writing of Messages")
				return
			}
		}
	}
}

func (t *Transport) receiveMessages() {
	log.Printf("Receiving messages")
	for {
		select {
		case <-t.CloseChan:
			{
				log.Printf("Closing receiving of Messages")
				return
			}
		default:
			{
				msgType, data, err := t.conn.ReadMessage()

				if err != nil {
					t.Close()
				}

				switch msgType {
				case websocket.TextMessage:
					fallthrough
				case websocket.BinaryMessage:
					{
						t.recvChan <- &Message{uint8(msgType), data, err}
					}
					break
				case websocket.PingMessage, websocket.PongMessage:
					fallthrough
				default:
					{
						log.Printf("Ignoring msg -> Type: %v", msgType)
					}
				}
			}
		}
	}
}

func (t *Transport) processMessages() {
	log.Printf("Processing messages")
	for {
		select {
		case msg := <-t.recvChan:
			log.Printf("Received message: %v\n", msg)
			t.MsgHandler(msg)
		case <-t.CloseChan:
			log.Printf("Closing processing")
			return
		}
	}
}
