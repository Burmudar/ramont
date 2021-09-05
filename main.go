package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"log"
	"net"
	"time"

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

func unixHandler() ramont.EventHandelerFunc {
	var d net.Dialer

	remoteAddr := net.UnixAddr{Name: "/home/william/programming/ramont/unix-input-socket/src/uv.socket", Net: "Unix"}
	conn, err := d.Dial("unix", remoteAddr.String())
	if err != nil {
		log.Fatalf("Failed to dial: %v", err)
	}

	unixEventSendChan := make(chan []byte, 1)

	go func(recvChan chan []byte) {
		defer conn.Close()

		writer := bufio.NewWriter(conn)
		//reader := bufio.NewReader(conn)
		for {
			select {
			case data := <-recvChan:
				{
					writer.Write(data)
					writer.Flush()
                    log.Printf("Time: %v Writing to unix socket", time.Now())
					//log.Printf("reading from unix socket")
					//content, err := reader.ReadString('\n')
					//log.Printf("Error[%v] Unix> %s\n", err, content)
				}
			}
		}
	}(unixEventSendChan)

	return func(ev ramont.Event) error {

		d, err := json.Marshal(ev)
		if err != nil {
			return err
		}

		unixEventSendChan <- d

		return nil
	}
}

func start() {
	r := gin.Default()

	r.GET("/ws", ramont.EventAwareHandler(unixHandler()))

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

func unix() {

	count := 0
	var d net.Dialer

	remoteAddr := net.UnixAddr{Name: "/home/william/programming/virtual-mouse/echo_socket", Net: "Unix"}
	conn, err := d.Dial("unix", remoteAddr.String())
	if err != nil {
		log.Fatalf("Failed to dial: %v", err)
	}

	writer := bufio.NewWriter(conn)
	reader := bufio.NewReader(conn)

	for count != 100 {

		count++
		data := fmt.Sprintf("%d\n", count)
		fmt.Printf("us>%s", data)

		if _, err := writer.WriteString(data); err != nil {
			log.Fatal(err)
		}
		writer.Flush()
		log.Println("Flushed.")

		if data, err := reader.ReadBytes('\n'); err != nil {
			log.Fatal(err)
		} else {
			log.Printf("them> %s", string(data))
		}
	}

}

func main() {
	start()
	//unix()
}
