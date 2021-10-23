import asyncio
import os
import json
import time
import libevdev
from libevdev import InputAbsInfo, InputEvent

class FakeDevice(object):

    def __init__(self):
        self.device:libevdev.Device = None
        self.uinput:libevdev.Device = None
        self.moves = []

    async def create_device(self):
        pass

    async def init(self)->None:
        self.device = await self.create_device()
        self.uinput = self.device.create_uinput_device()
        await asyncio.sleep(1)

    async def flush(self):
        await asyncio.sleep(0.012)
        print("slept 0.012")
        self.uinput.send_events(self.moves)
        print("sent movements")
        self.moves = []

    async def move(self, x, y) -> None:
        pass

class RealMouse(FakeDevice):

    def __init__(self, path:str):
        super(FakeDevice).__init__()
        self.device:libevdev.Device = None
        self.uinput:libevdev.Device = None
        self.path = path
        self.moves = []
        self.last_coords:tuple = (0,0)

    async def create_device(self)->libevdev.Device:
        device = None
        with open(self.path, "rb") as fd:
            device = libevdev.Device(fd)
            print(f"Device name: {device.name}")
            print(f"AbsX: {device.absinfo[libevdev.EV_REL.ABS_X]}")
            print(f"AbsY: {device.absinfo[libevdev.EV_REL.ABS_Y]}")

        return device

    def _delta_move(self, x, y) -> tuple:
        lx, ly = self.last_coords
        return x - lx, y - ly

    def _last_move(self, x, y) -> None:
        self.last_coords = (x,y)

    async def move(self, x, y) -> None:
        dx, dy = self._delta_move(x,y)

        self._last_move(x, y)

        try:
            print(f"last move: {self.last_coords}")
            print(f"adding mouse movements ({dx}, {dy})")
            self.moves.extend([libevdev.InputEvent(libevdev.EV_REL.REL_X, value=dx),
                    libevdev.InputEvent(libevdev.EV_REL.REL_Y, value=dy)])
            if len(self.moves) == 10:
                print(f"flushing moves")
                self.moves.append(libevdev.InputEvent(libevdev.EV_SYN.SYN_REPORT, value=0))
                await self.flush()
        except Exception as e:
            print(e)

class FakeMouse(FakeDevice):
    def __init__(self, name):
        super(FakeDevice).__init__()
        self.name = name

    async def create_device(self)->libevdev.Device:
        dev = libevdev.Device()
        dev.name = self.name

        # need to enable these so that linux thinks this is a mouse!
        dev.enable(libevdev.EV_REL.REL_X)
        dev.enable(libevdev.EV_REL.REL_Y)
        dev.enable(libevdev.EV_KEY.BTN_LEFT)
        dev.enable(libevdev.EV_KEY.BTN_MIDDLE)
        dev.enable(libevdev.EV_KEY.BTN_RIGHT)

        dev.id["bustype"] = 0x03
        dev.id["version"] = 1
        dev.id["vendor"] = 0x01
        dev.id["product"] = 0x01

        return dev

    async def move(self, x, y) -> None:
        try:
            print(f"trackpad({x}, {y})")
            self.moves = [
                    InputEvent(libevdev.EV_REL.REL_X, x),
                    InputEvent(libevdev.EV_REL.REL_Y, y),
                    InputEvent(libevdev.EV_SYN.SYN_REPORT, 0)
                    ]

            await self.flush()
        except Exception as e:
            print(e)


class Trackpad(FakeDevice):

    def __init__(self, name, xdim:tuple = (0,0), ydim:tuple = (0,0)):
        super(FakeDevice).__init__()
        self.name = name
        self.x_dimension = xdim
        self.y_dimension = ydim


    async def create_device(self)->libevdev.Device:
        dev = libevdev.Device()
        dev.name = self.name

        xAbsInfo = InputAbsInfo(minimum=self.x_dimension[0], maximum=self.x_dimension[1], resolution=1080)
        yAbsInfo = InputAbsInfo(minimum=self.y_dimension[0], maximum=self.y_dimension[1], resolution=1080)
        # for ABS movement to work and the device to be recognize you need to convince linux that his is a trackpad
        dev.enable(libevdev.EV_ABS.ABS_X, xAbsInfo)
        dev.enable(libevdev.EV_ABS.ABS_Y, yAbsInfo)
        # if you don't enable these two things ... linux doesn't regard this as a valid device/trackpad
        dev.enable(libevdev.EV_KEY.BTN_TOUCH)
        dev.enable(libevdev.INPUT_PROP_DIRECT)

        # if you only enable these along with EV_REL.REL_X/Y then linux will see this as a mouse
        dev.enable(libevdev.EV_KEY.BTN_LEFT)
        dev.enable(libevdev.EV_KEY.BTN_MIDDLE)
        dev.enable(libevdev.EV_KEY.BTN_RIGHT)

        dev.id["bustype"] = 0x03
        dev.id["version"] = 1
        dev.id["vendor"] = 0x01
        dev.id["product"] = 0x01

        return dev

    async def move(self, x, y) -> None:
        try:
            print(f"trackpad({x}, {y})")
            self.moves = [
                    InputEvent(libevdev.EV_ABS.ABS_X, x),
                    InputEvent(libevdev.EV_ABS.ABS_Y, y),
                    InputEvent(libevdev.EV_SYN.SYN_REPORT, 0)
                    ]

            await self.flush()
        except Exception as e:
            print(e)


class ClientEventHandler(object):

    def __init__(self, queue):
        self.queue = queue

    async def process(self, reader:asyncio.StreamReader) -> dict:
        data = await reader.readline()
        event = json.loads(data)
        return event

    async def handle(self, reader:asyncio.StreamReader, writer:asyncio.StreamWriter) -> None:
        print("client connected - processing")
        while True:
            ts = time.perf_counter()
            event = await self.process(reader)
            await self.queue.put((event, ts))


class EventTranslator(object):

    def __init__(self, width, height):
        self.width = width;
        self.height = height;

    def translate(self, event) -> tuple:
        print(f"translating {event}")
        x = float(event["unitX"]) * self.width
        y = float(event["unitY"]) * self.height

        return (int(x),int(y))


class EventConsumer(object):

    def __init__(self, queue, device, translator):
        self.queue = queue
        self.translator = translator
        self.device = device
        self._handle = None

    def start(self):
        self._handle = asyncio.create_task(self._process())

    async def _process(self) -> None:
        while True:
            event, ts = await self.queue.get()
            elapsed = time.perf_counter() - ts
            print(f"event -> {event} ns: {elapsed:0.5f}")

            x, y = self.translator.translate(event)
            print(f"translated coords {x},{y}")
            await self.device.move(x,y)

    def stop(self):
        self._handle.cancel()


def screen_dimensions():
    import Xlib
    import Xlib.display
    resolution = Xlib.display.Display().screen().root.get_geometry()
    return resolution.width, resolution.height


async def main(path:str, device_path)-> None:
    queue = asyncio.Queue()

    device = Trackpad("ramont trackpad", xdim=(0, 1275), ydim=(0, 1360))
    await device.init()
    print("device initialized")

    width, height = screen_dimensions()
    translator = EventTranslator(width, height)

    client_handler = ClientEventHandler(queue)
    event_consumer = EventConsumer(queue, device, translator)
    event_consumer.start()
    print("event consumer started")

    print(f"opening connection {path}")
    server = await asyncio.start_unix_server(client_handler.handle, path)


    print(f"serving forever")
    await server.serve_forever()


def device_path_candidates(path:str) ->list:
    import re

    m = re.compile(".*-event-mouse$")

    candidates = []
    for _, _, files in os.walk(path):
        check = lambda file: m.search(file)
        candidates.extend(filter(check, files))

    candidates = '\n'.join(map(lambda f: os.path.join("/dev/input/by-path", f), candidates))
    return candidates

def cli():
    import sys
    path = sys.argv[1]

    asyncio.run(main(path, ""))


if __name__ == "__main__":
    cli()
