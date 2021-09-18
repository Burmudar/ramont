package main

import (
	"encoding/json"

	"github.com/Burmudar/ramon-go/ramont"
	"github.com/gin-gonic/gin"
)

func handleEvent(transport *ramont.UnixTransport, ev ramont.Event) error {
	d, err := json.Marshal(ev)
	if err != nil {
		return err
	}

	transport.Send(d)

	return nil
}

func EventHandler() ramont.EventHandelerFunc {

	path := "/home/william/programming/ramont/unix-input-socket/_builds/uv.socket"
	//path := "/home/william/programming/ramont/unix-input-socket/src/uv.socket"

	transport := ramont.NewUnixTransport(path)

	return func(ev ramont.Event) error {
		return handleEvent(transport, ev)
	}
}

func start() {
	r := gin.Default()

	r.GET("/ws", ramont.EventAwareHandler(EventHandler()))

	r.Static("static", "static")
	r.LoadHTMLFiles("static/index.html")
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
