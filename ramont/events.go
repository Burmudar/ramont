package ramont

import (
	"encoding/json"
	"log"
)

type EventType string

type Event interface {
	TypeOf() EventType
}

type MouseEvent struct {
	Type    EventType `json:"type"`
	OffsetX float64   `json:"offsetX"`
	OffsetY float64   `json:"offsetY"`
}

func (me *MouseEvent) TypeOf() EventType {
	return me.Type
}

func unmarshalMouseEvent(eventType EventType, raw json.RawMessage) *MouseEvent {
	event := MouseEvent{Type: eventType}
	err := json.Unmarshal(raw, &event)
	log.Printf("Unmarshall Error: %v value: %v", err, string(raw))

	return &event
}

func processEventData(eventType string, data []byte) (Event, error) {
    event := unmarshalMouseEvent(EventType(eventType), data)
    return event, nil
}

