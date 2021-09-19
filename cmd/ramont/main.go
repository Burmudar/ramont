package main

import (
	"encoding/json"
	"log"
	"os"
	"path"

	"github.com/Burmudar/ramon-go/ramont"
	"github.com/gin-gonic/gin"
)

func handleEvent(transport ramont.Transport, ev *ramont.Event) error {
	log.Printf("Handling event -> %v", ev)
	d, err := json.Marshal(ev)
	if err != nil {
		return err
	}

	transport.Send(d)

	return nil
}

func OnEvent() ramont.EventHandelerFunc {

	path := "/home/william/programming/ramont/unix-input-socket/_builds/uv.socket"
	//path := "/home/william/programming/ramont/unix-input-socket/src/uv.socket"

	transport := ramont.NewUnixTransport(path)

	err := transport.Listen()
	if err != nil {
		log.Fatalf("[ERROR] Unix Transport listening failure: %v", err)
	}

	return func(ev *ramont.Event) error {
		return handleEvent(transport, ev)
	}
}

func start() {
	r := gin.Default()

	r.GET("/ws", ramont.OnEventHandeler(OnEvent()))

	p := path.Join(os.Getenv("RAMONT_STATIC"), "index.html")
	r.Static("static", os.Getenv("RAMONT_STATIC"))
	r.LoadHTMLFiles(p)
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
