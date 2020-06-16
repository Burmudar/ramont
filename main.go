package main

import (
	"github.com/Burmudar/ramon-go/ramont"
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

func start() {
	r := gin.Default()

	r.GET("/ws", ramont.HandleFunc)

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
