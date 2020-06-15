package main

import (
	"encoding/json"
	"log"

	"github.com/Burmudar/ramon-go/websockets"
	"github.com/gin-gonic/gin"
	"github.com/go-vgo/robotgo"
)

func screenshotOnMac() {
	//robotgo.ScrollMouse(10, "up")
	//robotgo.MouseClick("left", true)
	robotgo.KeyTap("cmd")
	robotgo.Sleep(5)
	//GOTCHA: Key combinations have to be done in REVERSE order like below
	robotgo.KeyTap("4", "shift", "cmd")
	//robotgo.MoveMouse(100, 200) // 1.0, 100.0)
	robotgo.Sleep(55)
}

type BasicMsg struct {
	Type string `json:"Type"`
	Rest json.RawMessage
}

type Replier struct {
	server *websockets.Server
}

func (r *Replier) Pong(ping *BasicMsg) error {

	msg, err := json.Marshal(struct {
		Type    string `json:"type"`
		Message string `json:"message"`
	}{"control", "received"})

	if err != nil {
		panic(err)
	}

	r.server.Reply(msg)

	return err
}

type EventType string

type MouseEvent struct {
	Type    EventType `json:"type"`
	OffsetX float32   `json:"offsetX"`
	OffsetY float32   `json:"offsetY"`
}

func unmarshalMouseEvent(eventType EventType, raw json.RawMessage) *MouseEvent {
	event := MouseEvent{Type: eventType}
	err := json.Unmarshal(raw, &event)
	log.Printf("Unmarshall Error: %v", err)

	return &event
}

func (r *Replier) BasicHandler(msg *websockets.Message) error {
	if msg.Error != nil {
		log.Fatalf("Problem occured while receiving msg: %v", msg.Error)
	}

	bmsg := BasicMsg{}
	err := json.Unmarshal(msg.Data, &bmsg)
	log.Printf("Unmarshall Error: %v", err)

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
	}

	log.Printf("MouseEvent: %v", event)

	return err
}

func start() {
	r := gin.Default()

	replier := Replier{server: websockets.NewServer(nil, nil)}
	replier.server.MsgHandler = replier.BasicHandler

	r.GET("/ws", replier.server.HandlerFunc)

	r.LoadHTMLFiles("html/index.html")
	r.GET("/", func(ctx *gin.Context) {
		ctx.HTML(200, "index.html", nil)
	})

	r.GET("/ping", func(c *gin.Context) {
		c.JSON(200, gin.H{
			"message": "pong",
		})
	})
	r.Run() // listen and serve on 0.0.0.0:8080 (for windows "localhost:8080")
}

func main() {
	start()
}
