class WebSocketClient {
    conn:WebSocket
    Open:boolean
    logger:Function
    msgQueue:Array<any>

    OnSend:CallableFunction
    OnMessage:CallableFunction
    OnOpen:CallableFunction

    constructor(URI:string, log = console.log) {
        this.conn = new WebSocket(URI)
        this.Open = false;
        this.logger = (o:any) => log(o);

        this.conn.onopen = () => this.handleOpen();
        this.conn.onmessage = (msg:MessageEvent) => this.handleMessage(msg);
        this.conn.onclose = () => this.handleClose();
    }

    handleOpen():void {
        this.logger("WebSocket open");
        this.Open = true;
    }

    handleMessage(msg:MessageEvent):void {
        let obj = JSON.parse(msg.data);
        this.logger(`Message Received: ${msg.data}`);
        this.logger('Calling OnMessage');
        if (this.OnMessage) {
            this.OnMessage(obj);
        }
    }

    handleClose():void {
        this.logger("WebSocket closed");
        this.Open = false;
    }

    close():void {
        this.conn.close;
    }

    send(obj:any):void {
        let msg = JSON.stringify(obj);
        let now = new Date()
        this.logger(`TS: ${now.toISOString()} Sending: ${msg}`);
        this.conn.send(msg);
    }

}

enum EventType {
    MouseDown= "mousedown",
    MouseMove = "mousemove",
    MouseUp = "mouseup",
    TouchStart = "touchstart",
    TouchMove = "touchmove",
    TouchEnd = "touchend",

}

enum MoveEventType {
    MouseStart = "mouse_start",
    MouseMove = "mouse_move",
    MouseEnd = "mouse_end",
    TouchStart = "touch_start",
    TouchMove = "touch_move",
    TouchEnd = "touch_end",
}

class MoveEventMessage {
    type:MoveEventType
    unitX: number
    unitY: number
    static readonly eventMap:Map<string, MoveEventType> = new Map([
            [EventType.MouseDown, MoveEventType.MouseStart],
            [EventType.MouseMove, MoveEventType.MouseMove],
            [EventType.MouseUp, MoveEventType.MouseEnd],
            [EventType.TouchStart, MoveEventType.TouchStart],
            [EventType.TouchMove, MoveEventType.TouchMove],
            [EventType.TouchEnd, MoveEventType.TouchEnd],
    ]);

    constructor(type:MoveEventType, X:number, Y:number){
        this.type = type;
        this.unitX = X;
        this.unitY = Y;
    }

    static from(e:MouseEvent, width:number, height:number):MoveEventMessage {
        return new MoveEventMessage(
            MoveEventMessage.resolveEventType(e.type),
            e.offsetX / width,
            e.offsetY / height
        );
    }

    static fromAll(e:TouchEvent, xOffset = 0, yOffset = 0):MoveEventMessage[] {
        let result:MoveEventMessage[] = [];
        let eventType = MoveEventMessage.resolveEventType(e.type);
        for(var i = 0; i < e.changedTouches.length; i++) {
            let msg = new MoveEventMessage(
                eventType,
                // TODO: Do we need to do this for touch events ?
                e.changedTouches[i].clientX - xOffset,
                e.changedTouches[i].clientY - yOffset
            );

            result.push(msg);
        }

        return result
    }

    static resolveEventType(type:string):MoveEventType{
        return MoveEventMessage.eventMap.get(type);
    }
}

class TrackpadListener {
    domEL:HTMLElement
    dragging:boolean
    onEvent:CallableFunction

    constructor(node:HTMLElement, onEvent:CallableFunction) {
        this.domEL = node;
        this.dragging = false;
        this.onEvent = onEvent;
        this.registerMouseListeners();
    }

    registerMouseListeners():void {
        let rect = this.domEL.getBoundingClientRect();
        let width = rect.width;
        let height = rect.height;
        let eventFn = (event:MouseEvent) => {
            let msg = MoveEventMessage.from(event, width, height);
            this.onEvent(msg, this.dragging)
        }

        this.domEL.onmousedown= (e:MouseEvent) =>{
            this.dragging = true;
            eventFn(e);
        };

        this.domEL.onmousemove= (e:MouseEvent) =>{
            this.dragging = false;
            eventFn(e);
        };

        this.domEL.onmouseup= (e:MouseEvent) =>{
            eventFn(e);
        };
    }

    registerTouchListeners():void {
        let rect = this.domEL.getBoundingClientRect();
        let xOffset = rect.x;
        let yOffset = rect.y;
        let eventFn = (event:TouchEvent) => {
            let msgs = MoveEventMessage.fromAll(event, xOffset, yOffset);
            msgs.forEach((i) => this.onEvent(i, this.dragging));
        }

        this.domEL.addEventListener(EventType.TouchStart, (e:TouchEvent) =>{
            this.dragging = true;
            eventFn(e);
        }, false );

        this.domEL.addEventListener(EventType.TouchMove, (e:TouchEvent) =>{
            this.dragging = false;
            eventFn(e);
        }, false );

        this.domEL.addEventListener(EventType.TouchEnd, (e:TouchEvent) =>{
            eventFn(e);
        }, false );
    }
}

class App {

    transport:WebSocketClient

    onTrackpadMove(e:MoveEventMessage) {
        this.transport.send(e);
    }

    run():void {
        let url = new URL(document.URL);
        url.protocol = "ws";
        url.pathname = "/ws";
        this.transport = new WebSocketClient(url.href);

        let el = document.getElementById("trackpad");

        let trackpad = new TrackpadListener(el, (e:MoveEventMessage) => this.onTrackpadMove(e));
    }
}

var app = new App();
app.run();
