package ramont

import (
	"bufio"
	"log"
	"net"
	"time"
)

type MsgHandler func(msg *Message) error

type Message struct {
	Type  uint8
	Data  []byte
	Error error
}

type Transport interface {
	CloseChan() chan bool
	Close()
	Listen() error
	Send(msg []byte)
}

type UnixTransport struct {
	Path     string
	Conn     net.Conn
	socketCh chan []byte
	doneCh   chan bool
}

func NewUnixTransport(path string) *UnixTransport {
	return &UnixTransport{path, nil, nil, nil}
}

func (u *UnixTransport) connect(address string) (net.Conn, error) {
	var d net.Dialer

	remoteAddr := net.UnixAddr{Name: address, Net: "Unix"}
	return d.Dial("unix", remoteAddr.String())
}

func (u *UnixTransport) CloseChan() chan bool {
	return u.doneCh
}

func (u *UnixTransport) Close() {
	close(u.doneCh)
	u.Conn.Close()
}

func (u *UnixTransport) Send(data []byte) {
	log.Printf("[DEBUG] Sending event data to -> %s", u.Path)
	u.socketCh <- data
}

func (u *UnixTransport) processChan(recvChan chan []byte) {
	writer := bufio.NewWriter(u.Conn)

	for {
		select {
		case data := <-recvChan:
			{
				if _, err := writer.Write(data); err != nil {
					log.Println("[ERROR] Failed to write to unix socket")
				} else {
					//each event is seperated by a newline
					writer.WriteByte('\n')
					writer.Flush()
					log.Printf("[DEBUG] Time: %v Writing to unix socket - data: %v", time.Now(), string(data))
				}
				// Should the unix socket ack a read ?
				//log.Printf("reading from unix socket")
				//content, err := reader.ReadString('\n')
				//log.Printf("Error[%v] Unix> %s\n", err, content)
			}
		case <-u.doneCh:
			{
				log.Printf("Closing processing of data for unix transport")
				return
			}
		}
	}
}

func (u *UnixTransport) startListener() chan []byte {
	c := make(chan []byte, 1)
	go u.processChan(c)
	return c
}

func (u *UnixTransport) Listen() error {
	var err error
	u.Conn, err = u.connect(u.Path)

	if err != nil {
		return err
	}

	u.doneCh = make(chan bool)
	u.socketCh = u.startListener()

	log.Printf("[DEBUG] Started Unix Listener")

	return nil

}
