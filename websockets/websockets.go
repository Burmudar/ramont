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

type Message interface{}

type Server struct {
	conn      *websocket.Conn
	recvChan  chan Message
	sendChan  chan []byte
	closeChan chan bool
}

func NewServer(conn *websocket.Conn) *Server {
	return &Server{
		conn:      conn,
		recvChan:  make(chan Message, 1),
		closeChan: make(chan bool),
		sendChan:  make(chan []byte),
	}
}

func HandlerFunc(ctx *gin.Context) {
	var upgrader = websocket.Upgrader{
		ReadBufferSize:  1024,
		WriteBufferSize: 1024,
		CheckOrigin:     DefaultValidateOrigin,
	}

	conn, err := upgrader.Upgrade(ctx.Writer, ctx.Request, nil)

	if err != nil {
		errMsg := fmt.Sprintf("Failed to upgrade connection to websockets: %v", err)

		resp := struct {
			Success bool
			Message string
		}{false, errMsg}

		ctx.AsciiJSON(500, resp)
	}

	srv := NewServer(conn)

	go srv.Listen()

}

func decodeTextMsg(data []byte) (Message, error) {
	return nil, nil
}

func (srv *Server) Close() {
	close(srv.closeChan)

}

func (srv *Server) Listen() {
	go srv.receiveMessages()
	go srv.processMessages()
	go srv.writeMessages()
}

func (srv *Server) Reply(msg string) {
	srv.sendChan <- []byte(msg)
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
					{
						if msg, err := decodeTextMsg(data); err != nil {
							log.Printf("Error decoding msg -> Type:%v Error: %v", msgType, err)
							srv.Reply("Error decoding message")
						} else {
							srv.recvChan <- msg
						}
					}
					break
				case websocket.PingMessage, websocket.BinaryMessage, websocket.PongMessage:
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
			srv.Reply("Received message")
		case <-srv.closeChan:
			log.Printf("Closing processing")
			return
		}
	}
}
