use std::sync::Mutex;

use evdev_rs::{
    enums::{BusType, EventCode, EventType, InputProp, EV_ABS, EV_KEY, EV_REL, EV_SYN},
    DeviceWrapper, InputEvent, TimeVal, UInputDevice, UninitDevice,
};

pub struct Mouse {
    name: String,
    device: UInputDevice,
    last_move: Mutex<Coordinate>,
}

pub struct Trackpad {
    name: String,
    device: UInputDevice,
    last_move: Mutex<Coordinate>,
    x_dimension: Dimension,
    y_dimension: Dimension,
}

pub trait VirtualDevice {
    fn r#move(&self, x: i32, y: i32) -> Result<(), std::io::Error>;
    fn sync(&self) -> Result<(), std::io::Error>;
}

struct Coordinate {
    x: i32,
    y: i32,
}

struct Dimension {
    width: i32,
    height: i32,
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
            last_move: Mutex::new(Coordinate { x: 0, y: 0 }),
        }
    }
}

impl VirtualDevice for Mouse {
    fn r#move(&self, x: i32, y: i32) -> Result<(), std::io::Error> {
        let c = *self.last_move.lock().unwrap();

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

    fn sync(&self) -> Result<(), std::io::Error> {
        self.device.write_event(&InputEvent::new(
            &ZERO_TIME,
            &EventCode::EV_SYN(EV_SYN::SYN_REPORT),
            0,
        ))
    }
}

impl Trackpad {
    pub fn new(name: &str, x_dimension: Dimension, y_dimension: Dimension) -> Trackpad {
        let device = UninitDevice::new().unwrap();

        device.set_name(name);

        let x_abs_info = evdev_rs::AbsInfo {
            value: 0,
            minimum: x_dimension.width,
            maximum: x_dimension.height,
            fuzz: 0,
            flat: 0,
            resolution: 1080,
        };
        let y_abs_info = evdev_rs::AbsInfo {
            value: 0,
            minimum: y_dimension.width,
            maximum: y_dimension.height,
            fuzz: 0,
            flat: 0,
            resolution: 1080,
        };
        device.set_abs_info(&EventCode::EV_ABS(EV_ABS::ABS_X), &x_abs_info);
        device.set_abs_info(&EventCode::EV_ABS(EV_ABS::ABS_Y), &y_abs_info);

        device
            .enable(&EventType::EV_ABS)
            .expect("failed to enable event type EV_ABS");
        device
            .enable(&EventCode::EV_ABS(EV_ABS::ABS_X))
            .expect("failed to enable event ABS_X");
        device
            .enable(&EventCode::EV_ABS(EV_ABS::ABS_Y))
            .expect("failed to enable event ABS_Y");

        device.enable(&EventType::EV_KEY);
        device
            .enable(&EventCode::EV_KEY(EV_KEY::KEY_TOUCHPAD_ON))
            .expect("failed to enable KEY_TOUCHPAD_ON");
        device
            .enable(&EventCode::EV_KEY(EV_KEY::KEY_TOUCHPAD_OFF))
            .expect("failed to enable KEY_TOUCHPAD_OFF");
        device
            .enable(&InputProp::INPUT_PROP_DIRECT)
            .expect("failed to enable property INPUT_PROP_DIRECT");

        device
            .enable(&EventCode::EV_KEY(EV_KEY::BTN_LEFT))
            .expect("failed to enable EV_KEY BTN_LEFT");
        device
            .enable(&EventCode::EV_KEY(EV_KEY::BTN_MIDDLE))
            .expect("failed to enable EV_KEY BTN_MIDDLE");
        device
            .enable(&EventCode::EV_KEY(EV_KEY::BTN_RIGHT))
            .expect("failed to enable EV_KEY BTN_RIGHT");

        let udevice = UInputDevice::create_from_device(&device)
            .expect("failed to create UInputDevice from raw device");

        Trackpad {
            name: name.to_string(),
            device: udevice,
            x_dimension,
            y_dimension,
            last_move: Mutex::new(Coordinate { x: 0, y: 0 }),
        }
    }
}

impl VirtualDevice for Trackpad {
    fn r#move(&self, x: i32, y: i32) -> Result<(), std::io::Error> {
        let c = *self.last_move.lock().unwrap();

        self.device.write_event(&InputEvent::new(
            &ZERO_TIME,
            &EventCode::EV_ABS(EV_ABS::ABS_Y),
            x,
        ));
        self.device.write_event(&InputEvent::new(
            &ZERO_TIME,
            &EventCode::EV_ABS(EV_ABS::ABS_X),
            y,
        ));

        c.x = x;
        c.y = y;

        self.sync()
    }

    fn sync(&self) -> Result<(), std::io::Error> {
        self.device.write_event(&InputEvent::new(
            &ZERO_TIME,
            &EventCode::EV_SYN(EV_SYN::SYN_REPORT),
            0,
        ))
    }
}
