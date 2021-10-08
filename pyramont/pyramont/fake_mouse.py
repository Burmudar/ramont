import asyncio
import math
import os
import json
import time
import libevdev

class FakeMouse(object):

    def __init__(self, device_path:str):
        self.path = device_path
        self.device:libevdev.Device = None
        self.uinput:libevdev.Device = None
        self.moves = []
        self.last_coords:tuple = (0,0)

    async def _open_device(self, path:str)->libevdev.Device:
        device = None
        with open(path, "rb") as fd:
            device = libevdev.Device(fd)
            print(f"Device name: {device.name}")
            print(f"AbsX: {device.absinfo[libevdev.EV_REL.ABS_X]}")
            print(f"AbsY: {device.absinfo[libevdev.EV_REL.ABS_Y]}")

        return device

    async def init(self)->None:
        self.device = await self._open_device(self.path)
        self.uinput = self.device.create_uinput_device()
        await asyncio.sleep(1)

    def _delta_move(self, x, y) -> tuple:
        lx, ly = self.last_coords
        return x - lx, y - ly

    def _last_move(self, x, y) -> None:
        self.last_coords = (x,y)

    async def _flush_moves(self):
        await asyncio.sleep(0.012)
        print("slept 0.012")
        self.uinput.send_events(self.moves)
        print("sent movements")
        self.moves = []

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
                await self._flush_moves()
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

    def __init__(self, queue, mouse, translator):
        self.queue = queue
        self.translator = translator
        self.mouse = mouse
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
            await self.mouse.move(x,y)

    def stop(self):
        self._handle.cancel()


def screen_dimensions():
    import Xlib
    import Xlib.display
    resolution = Xlib.display.Display().screen().root.get_geometry()
    return resolution.width, resolution.height


async def main(path:str, mouse_path)-> None:
    queue = asyncio.Queue()

    mouse = FakeMouse(mouse_path)
    await mouse.init()
    print("mouse initialized")

    width, height = screen_dimensions()
    translator = EventTranslator(width, height)

    client_handler = ClientEventHandler(queue)
    event_consumer = EventConsumer(queue, mouse, translator)
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

    return candidates

def cli():
    import sys
    path = sys.argv[1]
    mouse_path = ""
    if len(sys.argv) < 3:
        candidates = device_path_candidates("/dev/input/by-path")
        paths = '\n'.join(map(lambda f: os.path.join("/dev/input/by-path", f), candidates))
        print(f"Path candidates:\n{paths}")
        sys.exit(1)
    else:
        mouse_path = sys.argv[2]

    asyncio.run(main(path, mouse_path))


if __name__ == "__main__":
    cli()
