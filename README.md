# FreeRTOS-FAT-CLI-for-RPi-Pico

## SD Cards on the Pico

This project is essentially a [FreeRTOS+FAT](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/index.html)
[Media Driver](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/Creating_a_file_system_media_driver.html)
for the [Raspberry Pi Pico](https://www.raspberrypi.org/products/raspberry-pi-pico/),
using Serial Peripheral Interface (SPI),
based on [SDBlockDevice from Mbed OS 5](https://os.mbed.com/docs/mbed-os/v5.15/apis/sdblockdevice.html). 
It is wrapped up in a complete runnable project, with a command line interface provided by 
[FreeRTOS+CLI](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_CLI/FreeRTOS_Plus_Command_Line_Interface.html).

![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/IMG_1473.JPG "Prototype")

## Features:
* Supports multiple SD Cards
* Supports multiple SPIs
* Supports multiple SD Cards per SPI
* Supports Real Time Clock for maintaining file and directory time stamps
* Supports Cyclic Redundancy Check (CRC)

## Prerequisites:
* Raspberry Pi Pico
* Something like the [SparkFun microSD Transflash Breakout](https://www.sparkfun.com/products/544)
* Breadboard and wires
* Raspberry Pi Pico C/C++ SDK
* [FreeRTOS-Kernel](https://github.com/FreeRTOS/FreeRTOS-Kernel)
* [Lab-Project-FreeRTOS-FAT](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-FAT)

![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/IMG_1478.JPG "Prototype")

![image](https://www.raspberrypi.org/documentation/rp2040/getting-started/static/15243f1ffd3b8ee646a1708bf4c0e866/Pico-R3-Pinout.svg "Pinout")

|	    | SPI0 | GPIO | Pin   | SPI	     | MicroSD 0 |
| --- | ---- | ---- | ---   | -------- | --------- |
| MOSI|	TX	 | 16	  | 25	  | DI	     | DI        |
| CS0	| CSn	 | 17	  |	22	  |	SS or CS | CS        |
| SCK	| SCK	 | 18	  |	24	  | SCLK	   | CLK       |
| MISO| RX   | 19	  |	21	  | DO	     | DO        |
| CD	|      | 22	  | 29		|	         | CD 			 |
| GND	|      |      | 18,23	|	         | GND       |
| 3v3	|      |      | 36		|	         | 3v3       |


## Construction:
* The wiring is so simple that I didn't bother with a schematic. 
I just referred to the table above, wiring point-to-point from the Pin column on the Pico to the MicroSD 0 column on the Transflash.
* You can choose to use either or both of the Pico's SPIs.
* To add a second SD card on the same SPI, connect it in parallel, except that it will need a unique GPIO for the Card Select/Slave Select (CSn).
* Wires should be kept short and direct. SPI operates at HF radio frequencies.

## Firmware:
* Install source code:
  `git clone --recurse-submodules https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico.git`
* Customize:
  * Tailor `hw_config.c` to match hardware
  * Build:
```  
   mkdir build
   cd build
   cmake ..
   make
```   
  * Program the device
  
## Operation:
* By default, this project has serial input (stdin) and output (stdout) directed to USB CDC (USB serial). 
* Connect a terminal. [PuTTY](https://www.putty.org/) or `tio` work OK. For example:
  * `tio -m ODELBS /dev/ttyACM0`
* Press Enter to start the CLI. You should see a prompt like:
```
    Sunday 24 February 9:04:23 2021 
    FreeRTOS+CLI> 
    
    > 
```    
* The `help` command describes the available commands:
```    
    help:
     Lists all the registered commands.
    
    Prefix command with "time " to time execution.
    
    task-stats:
     Displays a table showing the state of each FreeRTOS task
    
    run-time-stats:
     Displays a table showing how much processing time each FreeRTOS task has used
    
    cls:
     Clear screen
    
    diskinfo <device name>:
     Print information about mounted partitions
    	e.g.: "diskinfo SDCard"
    
    setrtc <DD> <MM> <YY> <hh> <mm> <ss>:
     Set Real Time Clock
     Parameters: new date (DD MM YY) new time in 24-hour format (hh mm ss)
    
    date:
     Print current date and time
    die:
     Kill background tasks
    
    undie:
     Allow background tasks to live again
    
    test <number>:
     Development test
    
    reset:
     Soft system reset
    
    format <device name>:
     Format <device name>
    	e.g.: "format sd0"
    
    mount <device name>:
     Mount <device name> at /<device name>
    	e.g.: "mount sd0"
    
    eject <device name>:
     Eject <device name>
    	e.g.: "eject sd0"
    
    lliot <device name>:
     !DESTRUCTIVE! Low Level I/O Driver Test
    	e.g.: "lliot sd0"
    
    simple <device name>:
     Run simple FS tests
    Expects card to already be formatted but not mounted.
    	e.g.: "simple sd0"
    
    cdef <device name>:
     Create Disk and Example Files
    Expects card to be already formatted but not mounted.
    	e.g.: "cdef sd0"
    
    swcwdt <device name>:
     Stdio With CWD Test
    Expects card to be already formatted but not mounted.
    Note: run cdef first!	e.g.: "swcwdt sd0"
    
    mtswcwdt <device name>:
     MultiTask Stdio With CWD Test
    Expects card to be already formatted but not mounted.
    	e.g.: "mtswcwdt sd0"
    
    big_file_test <pathname> <size in bytes> <seed>:
     Writes random data to file <pathname>.
     <size in bytes> must be multiple of 512.
    	e.g.: big_file_test sd0/bf 1048576 1
    
    dir:
     Lists the files in the current directory
    
    cd <dir name>:
     Changes the working directory
    
    type <filename>:
     Prints file contents to the terminal
    
    del <filename>:
     deletes a file (use rmdir to delete a directory)
    
    rmdir <directory name>:
     deletes a directory
    
    copy <source file> <dest file>:
     Copies <source file> to <dest file>
    
    ren <source file> <dest file>:
     Moves <source file> to <dest file>
    
    pwd:
     Print Working Directory
    
    > 
```
![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/IMG_1481.JPG "Prototype")
Happy hacking!
