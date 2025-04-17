use std::fs::{OpenOptions};
use std::io::{Read, Write};
use std::thread;
use std::time;

fn main()
{
    let mut led1;
    let mut led2;
    let mut led3;
    let mut clicks;

    let mut led1_status = String::new();
    let mut led2_status = String::new();
    let mut led3_status = String::new();
    let mut clicks_count = String::new();
    
    loop
    {
	// Opening Files
	led1 = OpenOptions::new().read(true).write(true).open("/sys/kernel/LedController/led1").expect("Failed to open file");
	led2 = OpenOptions::new().read(true).write(true).open("/sys/kernel/LedController/led2").expect("Failed to open file");
	led3 = OpenOptions::new().read(true).write(true).open("/sys/kernel/LedController/led3").expect("Failed to open file");
	clicks = OpenOptions::new().read(true).write(true).open("/sys/kernel/LedController/clicks").expect("Failed to open file");
	// Reading from files
	led1.read_to_string(&mut led1_status).expect("");
        led2.read_to_string(&mut led2_status).expect("");
	led3.read_to_string(&mut led3_status).expect("");
	clicks.read_to_string(&mut clicks_count).expect("");
	println!("{},{},{},{}", led1_status, led2_status, led3_status, clicks_count);

	match clicks_count.trim().parse().unwrap(){
		1..=10 => {led1.write_all(b"25"); led2.write_all(b"0"); led3.write_all(b"0");},
		11..=20 => {led1.write_all(b"50"); led2.write_all(b"0"); led3.write_all(b"0");},
		21..=30 => {led1.write_all(b"75"); led2.write_all(b"0"); led3.write_all(b"0");},
		31..=40 => {led1.write_all(b"100"); led2.write_all(b"0"); led3.write_all(b"0");},
		
		41..=50 => {led1.write_all(b"100"); led2.write_all(b"25"); led3.write_all(b"0");},
		51..=60 => {led1.write_all(b"100"); led2.write_all(b"50"); led3.write_all(b"0");},
		61..=70 => {led1.write_all(b"100"); led2.write_all(b"75"); led3.write_all(b"0");},
		71..=80 => {led1.write_all(b"100"); led2.write_all(b"100"); led3.write_all(b"0");},
		
		81..=90 => {led1.write_all(b"100"); led2.write_all(b"100"); led3.write_all(b"25");},
		91..=100 => {led1.write_all(b"100"); led2.write_all(b"100"); led3.write_all(b"50");},
		101..=110 => {led1.write_all(b"100"); led2.write_all(b"100"); led3.write_all(b"75");},
		111..=120 => {led1.write_all(b"100"); led2.write_all(b"100"); led3.write_all(b"100");},

		_ => {led1.write_all(b"0"); led2.write_all(b"0"); led3.write_all(b"0");}
	}
	// Resetting Count
	clicks.write_all(b"0");
	
	// Clearing Previously stored values
	led1_status.clear();
	led2_status.clear();
	led3_status.clear();
	clicks_count.clear();

	// Closing files
	drop(led1);drop(led2);drop(led3);drop(clicks);
	// Sleeping for 10 seconds
	thread::sleep(time::Duration::from_millis(10000));
    }
}
