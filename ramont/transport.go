package ramont

import (
	"bufio"
	"log"
	"net"
	"time"
)


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

func (u *UnixTransport) Close() {
	close(u.socketCh)
}

func (u *UnixTransport) Send(data []byte) {
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
					log.Printf("Time: %v Writing to unix socket - data: %v", time.Now(), string(data))
				}
				// Should the unix socket ack a read ?
				//log.Printf("reading from unix socket")
				//content, err := reader.ReadString('\n')
				//log.Printf("Error[%v] Unix> %s\n", err, content)
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

	u.socketCh = u.startListener()
	return nil

}
