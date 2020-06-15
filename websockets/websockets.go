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

type Server struct {
	conn       *websocket.Conn
	recvChan   chan *Message
	sendChan   chan []byte
	closeChan  chan bool
	MsgHandler MsgHandler
}

func NewServer(conn *websocket.Conn, msgHandler MsgHandler) *Server {
	return &Server{
		conn:       conn,
		recvChan:   make(chan *Message, 1),
		closeChan:  make(chan bool),
		sendChan:   make(chan []byte),
		MsgHandler: msgHandler,
	}
}

func DefaultMsgHandler(msg *Message) error {
	log.Printf("[DEFAULT TEXT HANDLER] Received Text message: %v", msg)
	return nil
}

func (srv *Server) HandlerFunc(ctx *gin.Context) {
	var upgrader = websocket.Upgrader{
		ReadBufferSize:  1024,
		WriteBufferSize: 1024,
		CheckOrigin:     DefaultValidateOrigin,
	}

	conn, err := upgrader.Upgrade(ctx.Writer, ctx.Request, nil)

	srv.conn = conn

	if err != nil {
		errMsg := fmt.Sprintf("Failed to upgrade connection to websockets: %v", err)

		resp := struct {
			Success bool
			Message string
		}{false, errMsg}

		ctx.AsciiJSON(500, resp)
	}

	srv.Listen()
}

func (srv *Server) Close() {
	close(srv.closeChan)

}

func (srv *Server) Listen() {
	go srv.receiveMessages()
	go srv.writeMessages()
	go srv.processMessages()
}

func (srv *Server) Reply(msg []byte) {
	srv.sendChan <- msg
}

func (srv *Server) writeMessages() {
	log.Printf("Writing messages")
	for {
		select {
		case data := <-srv.sendChan:
			{
				srv.conn.WriteMessage(websocket.TextMessage, data)
			}
			break
		case <-srv.closeChan:
			{
				log.Printf("Closing writing of Messages")
				return
			}
		}
	}
}

func (srv *Server) receiveMessages() {
	log.Printf("Receiving messages")
	for {
		select {
		case <-srv.closeChan:
			{
				log.Printf("Closing receiving of Messages")
				return
			}
		default:
			{
				msgType, data, err := srv.conn.ReadMessage()

				if err != nil {
					srv.Close()
				}

				switch msgType {
				case websocket.TextMessage:
					fallthrough
				case websocket.BinaryMessage:
					{
						srv.recvChan <- &Message{uint8(msgType), data, err}
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

func (srv *Server) processMessages() {
	log.Printf("Processing messages")
	for {
		select {
		case msg := <-srv.recvChan:
			log.Printf("Received message: %v\n", msg)
			srv.MsgHandler(msg)
		case <-srv.closeChan:
			log.Printf("Closing processing")
			return
		}
	}
}
