package websockets

import (
	"fmt"
	"log"
	"net/http"

	"github.com/Burmudar/ramon-go/ramont/network"
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

type WebSocketTransport struct {
	conn       *websocket.Conn
	recvChan   chan *network.Message
	sendChan   chan []byte
	closeChan  chan bool
	MsgHandler network.MsgHandler
}

func NewTransport(conn *websocket.Conn, msgHandler network.MsgHandler) network.Transport {
	return &WebSocketTransport{
		conn:       conn,
		recvChan:   make(chan *network.Message, 1),
		closeChan:  make(chan bool),
		sendChan:   make(chan []byte),
		MsgHandler: msgHandler,
	}
}

func DefaultMsgHandler(msg *network.Message) error {
	log.Printf("[DEFAULT TEXT HANDLER] Received Text message: %v", msg)
	return nil
}

func (t *WebSocketTransport) Hijack(ctx *gin.Context) error {
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
/*
func HandlerFunc(msgHandler network.MsgHandler) gin.HandlerFunc {
	return func(ctx *gin.Context) {
		t := NewTransport(nil, msgHandler)
		wsTransport, _ := t.(*WebSocketTransport)
		if err := wsTransport.Hijack(ctx); err != nil {
			resp := struct {
				Success bool   `json:"success"`
				Message string `json:"Message"`
			}{false, fmt.Sprintf("%s", err)}
			ctx.AsciiJSON(500, resp)
		}
		t.Listen()
	}
}
*/

func (srv *WebSocketTransport) CloseChan() chan bool {
	return srv.closeChan
}

func (srv *WebSocketTransport) Close() {
	close(srv.closeChan)

}

func (t *WebSocketTransport) Listen() {
	go t.receiveMessages()
	go t.writeMessages()
	go t.processMessages()
}

func (srv *WebSocketTransport) Send(msg []byte) {
	srv.sendChan <- msg
}

func (t *WebSocketTransport) writeMessages() {
	log.Printf("Writing messages")
	for {
		select {
		case data := <-t.sendChan:
			{
				t.conn.WriteMessage(websocket.TextMessage, data)
			}
			break
		case <-t.closeChan:
			{
				log.Printf("Closing writing of Messages")
				return
			}
		}
	}
}

func (t *WebSocketTransport) receiveMessages() {
	log.Printf("Receiving messages")
	for {
		select {
		case <-t.closeChan:
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
						t.recvChan <- &network.Message{uint8(msgType), data, err}
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

func (t *WebSocketTransport) processMessages() {
	log.Printf("Processing messages")
	for {
		select {
		case msg := <-t.recvChan:
			log.Printf("Received message: %v\n", msg)
			t.MsgHandler(msg)
		case <-t.closeChan:
			log.Printf("Closing processing")
			return
		}
	}
}
