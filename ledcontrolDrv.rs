use std::fs::{OpenOptions};
use std::io::{Read, Write};
use std::thread;
use std::time;

fn main()
{
    println!("Starting LED Intensity Control");
    let mut file = OpenOptions::new().read(true).write(true).open("/dev/LedController").expect("Failed to open file");

    let mut contents = String::new();

    loop
    {
        file.write_all(b"Led1_intensity=0\n");
        file.write_all(b"Led1_intensity=25\n");
        file.write_all(b"Led1_intensity=50\n");
        file.write_all(b"Led1_intensity=75\n");
        file.write_all(b"Led1_intensity=100\n");

	file.write_all(b"Led2_intensity=0\n");
        file.write_all(b"Led2_intensity=25\n");
        file.write_all(b"Led2_intensity=50\n");
        file.write_all(b"Led2_intensity=75\n");
        file.write_all(b"Led2_intensity=100\n");

	file.write_all(b"Led3_intensity=0\n");
        file.write_all(b"Led3_intensity=25\n");
        file.write_all(b"Led3_intensity=50\n");
        file.write_all(b"Led3_intensity=75\n");
        file.write_all(b"Led3_intensity=100\n");

        thread::sleep(time::Duration::from_millis(10000));
    }
}

