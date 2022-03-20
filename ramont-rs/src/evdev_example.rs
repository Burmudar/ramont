use std::thread;
use std::time::Duration;

use evdev_rs::enums;
use evdev_rs::enums::BusType;
use evdev_rs::enums::EventCode;
use evdev_rs::enums::EventType;
use evdev_rs::enums::EV_KEY;
use evdev_rs::enums::EV_REL;
use evdev_rs::DeviceWrapper;
use evdev_rs::InputEvent;
use evdev_rs::TimeVal;
use evdev_rs::UninitDevice;

fn create_fake_device() -> impl DeviceWrapper {
    let dev = UninitDevice::new().unwrap();
    dev.set_name("Test device");
    dev.enable(&EventType::EV_REL)
        .unwrap_or_else(|err| eprintln!("failed to enable event type EV_REL: {}", err));
    dev.enable(&EventCode::EV_REL(EV_REL::REL_X))
        .unwrap_or_else(|err| eprintln!("failed to enable property EV_REL::REL_X: {}", err));
    dev.enable(&EventCode::EV_REL(EV_REL::REL_Y))
        .unwrap_or_else(|err| eprintln!("failed to enable property EV_REL::REL_Y: {}", err));
    dev.enable(&EventType::EV_KEY)
        .unwrap_or_else(|err| eprintln!("failed to enable event type EV_KEY: {}", err));
    dev.enable(&EventCode::EV_KEY(EV_KEY::BTN_LEFT))
        .unwrap_or_else(|err| eprintln!("failed to enable property EV_REL::BTN_LEFT: {}", err));
    dev.enable(&EventCode::EV_KEY(EV_KEY::BTN_MIDDLE))
        .unwrap_or_else(|err| eprintln!("failed to enable property EV_REL::BTN_MIDDLE: {}", err));
    dev.enable(&EventCode::EV_KEY(EV_KEY::BTN_RIGHT))
        .unwrap_or_else(|err| eprintln!("failed to enable property EV_REL::BTN_RIGHT: {}", err));

    dev.set_bustype(BusType::BUS_USB as u16);
    dev.set_version(1);
    dev.set_vendor_id(0x01);
    dev.set_product_id(0x01);

    return dev;
}

fn main() {
    let dev = create_fake_device();
    let udev = evdev_rs::UInputDevice::create_from_device(&dev).unwrap();

    let zero_time = TimeVal::new(0, 0);

    const DELTA: i32 = 5;
    for _ in 1..100 {
        udev.write_event(&InputEvent::new(
            &zero_time,
            &enums::EventCode::EV_REL(enums::EV_REL::REL_X),
            DELTA,
        ))
        .unwrap();
        udev.write_event(&InputEvent::new(
            &zero_time,
            &enums::EventCode::EV_REL(enums::EV_REL::REL_Y),
            DELTA,
        ))
        .unwrap();
        udev.write_event(&InputEvent::new(
            &zero_time,
            &enums::EventCode::EV_SYN(enums::EV_SYN::SYN_REPORT),
            0,
        ))
        .unwrap();
        println!("sleeping");
        thread::sleep(Duration::from_millis(10));
    }
    println!("done");
}
