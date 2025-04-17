# Speed_Monitoring_LEDs
This is a simple speed monitoring system build using RUST (User Space) and C (Kernel Space)

### Block Diagram/System Overview
![Block Diagram](https://github.com/user-attachments/assets/232aee38-2b13-4dfa-be97-7c7217f2c419)

### Description
This is a system build as a course project for High End Device Development. It measures the speed at which the user/player is able to click the push buttons connected and lights up the LEDs with intensity mapping to speed. It uses the following hardware componets:
- LEDs
- Raspberry PI 4B
- Push buttons
This software design apporach seperates the functionality into two components:
- Interaction with Hardware components is done using a driver developed in **C**. User space interaction is developed in **RUST**
## Driver Functions
- Driver uses the internal hires hardware timer to generation variable intensity using pulse width modulation and memory maps to control LEDs.
- It uses GPIO to read data from push buttons and hardware interupts to calculate number of clicks.
- It also provides sysfs support to communicate with user space
## User Space functions
- It reads the data from driver using sysfs every 10s.
- Based on the data read (Speed) it calculates the intensity of each LED and writes into the sysfs so LEDs intensity can be determined by the driver.

### Hardware setup
![Hardware_Setup](https://github.com/user-attachments/assets/54628355-a3c8-4f82-bb83-38c492b69d68)

### Working Demo
https://github.com/user-attachments/assets/a2cfe656-ab33-441b-a3db-d474e044aef5
https://github.com/user-attachments/assets/e6e73f18-e1e0-4211-b075-42170f8feab9

