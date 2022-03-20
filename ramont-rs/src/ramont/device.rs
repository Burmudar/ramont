use std::sync::Mutex;

use evdev_rs::{
    enums::{BusType, EventCode, EventType, EV_KEY, EV_REL, EV_SYN},
    DeviceWrapper, InputEvent, TimeVal, UInputDevice, UninitDevice,
};

pub struct Mouse {
    name: String,
    device: UInputDevice,
    last_move: Mutex<Move>,
}

struct Coordinate {
    x: i32,
    y: i32,
}

enum Move {
    Point(Coordinate),
}

impl Move {
    fn new(x: i32, y: i32) -> Move {
        Move::Point(Coordinate { x, y })
    }
}

const ZERO_TIME: TimeVal = TimeVal::new(0, 0);

impl Mouse {
    pub fn new(name: &str) -> Mouse {
        let device = UninitDevice::new().unwrap();

        device.set_name(name);

        device
            .enable(&EventType::EV_REL)
            .expect("failed to enable EV_REL event");
        device
            .enable(&EventCode::EV_REL(EV_REL::REL_X))
            .expect("failed to enable REL_X event code");
        device
            .enable(&EventCode::EV_REL(EV_REL::REL_Y))
            .expect("failed to enable REL_Y event code");

        device
            .enable(&EventType::EV_KEY)
            .expect("failed to enable EV_KEY event type");
        device
            .enable(&EventCode::EV_KEY(EV_KEY::BTN_LEFT))
            .expect("failed to enable EV_KEY BTN_LEFT");
        device
            .enable(&EventCode::EV_KEY(EV_KEY::BTN_MIDDLE))
            .expect("failed to enable EV_KEY BTN_MIDDLE");
        device
            .enable(&EventCode::EV_KEY(EV_KEY::BTN_RIGHT))
            .expect("failed to enable EV_KEY BTN_RIGHT");

        device.set_bustype(BusType::BUS_USB as u16);
        device.set_version(1);
        device.set_vendor_id(0x01);
        device.set_product_id(0x01);

        let udevice = UInputDevice::create_from_device(&device)
            .expect("failed to create UInputDevice from raw device");

        Mouse {
            name: name.to_string(),
            device: udevice,
            last_move: Mutex::new(Move::new(0, 0)),
        }
    }

    pub fn r#move(&self, x: i32, y: i32) -> Result<(), std::io::Error> {
        let p = *self.last_move.lock().unwrap();

        let Move::Point(c) = p;
        // we do this to get the delta of the the old and new point
        // essentially we're answering "How many units must I move from the old position to the new
        // position.
        let x = (c.x - x).abs();
        let y = (c.y - y).abs();

        self.device.write_event(&InputEvent::new(
            &ZERO_TIME,
            &EventCode::EV_REL(EV_REL::REL_X),
            x,
        ));
        self.device.write_event(&InputEvent::new(
            &ZERO_TIME,
            &EventCode::EV_REL(EV_REL::REL_Y),
            y,
        ));

        c.x = x;
        c.y = y;

        self.sync()
    }

    pub fn sync(&self) -> Result<(), std::io::Error> {
        self.device.write_event(&InputEvent::new(
            &ZERO_TIME,
            &EventCode::EV_SYN(EV_SYN::SYN_REPORT),
            0,
        ))
    }
}
