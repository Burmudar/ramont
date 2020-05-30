package main

import (
	"github.com/go-vgo/robotgo"
)

func main() {
	//robotgo.ScrollMouse(10, "up")
	//robotgo.MouseClick("left", true)
	robotgo.KeyTap("cmd")
	robotgo.Sleep(5)
	//GOTCHA: Key combinations have to be done in REVERSE order like below
	robotgo.KeyTap("4", "shift", "cmd")
	//robotgo.MoveMouse(100, 200) // 1.0, 100.0)
	robotgo.Sleep(55)
}
