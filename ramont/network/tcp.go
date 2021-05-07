package network

type MsgHandler func(msg *Message) error

type Message struct {
	Type  uint8
	Data  []byte
	Error error
}

type Transport interface {
	CloseChan() chan bool
	Close()
	Listen()
	Send(msg []byte)
}
