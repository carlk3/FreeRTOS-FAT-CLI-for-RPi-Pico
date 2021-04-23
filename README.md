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

## Performance
Writing and reading a file of 0x10000000 (268,435,456) bytes (1/4 GiB) on a SanDisk 32GB card with SPI baud rate 12,500,000:
* Writing
  * Elapsed seconds 376
  * Transfer rate 697 KiB/s
* Reading
  * Elapsed seconds 315
  * Transfer rate 832 KiB/s

Surprisingly (to me), I have been able to push the SPI baud rate as far as 20,833,333 with a SanDisk Class 4 16 GB card:
* Writing
  * Elapsed seconds 226
  * Transfer rate 1159 KiB/s
* Reading
  * Elapsed seconds 232
  * Transfer rate 1129 KiB/s

 [but SDIO would be faster!]

## Prerequisites:
* Raspberry Pi Pico
* Breadboard and wires
* Raspberry Pi Pico C/C++ SDK
* Something like the [SparkFun microSD Transflash Breakout](https://www.sparkfun.com/products/544)

Note: avoid modules like these: [DAOKI 5Pcs TF Micro SD Card Module Memory Shield Module Micro SD Storage Expansion Board Mini Micro SD TF Card with Pins for Arduino ARM AVR with Dupo](https://www.amazon.com/gp/product/B07XF4RJSL/). They appear to be designed for 5v signals and won't work with the 3.3v Pico.

* (Optional) A couple of ~5-10kΩ resistors

## Dependencies:
* [FreeRTOS-Kernel](https://github.com/FreeRTOS/FreeRTOS-Kernel)
* [Lab-Project-FreeRTOS-FAT](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-FAT)

![image](https://www.raspberrypi.org/documentation/rp2040/getting-started/static/64b50c4316a7aefef66290dcdecda8be/Pico-R3-SDK11-Pinout.svg "Pinout")

|       | SPI0  | GPIO  | Pin   | SPI       | MicroSD 0 | Description            | 
| ----- | ----  | ----- | ---   | --------  | --------- | ---------------------- |
| MOSI  | TX    | 16    | 21    | DI        | DI        | Master Out, Slave In   |
| CS0   | CSn   | 17    | 22    | SS or CS  | CS        | Slave (or Chip) Select |
| SCK   | SCK   | 18    | 24    | SCLK      | CLK       | SPI clock              |
| MISO  | RX    | 19    | 25    | DO        | DO        | Master In, Slave Out   |
| CD    |       | 22    | 29    |           | CD        | Card Detect            |
| GND   |       |       | 18,23 |           | GND       | Ground                 |
| 3v3   |       |       | 36    |           | 3v3       | 3.3 volt power         |

## Construction:
* The wiring is so simple that I didn't bother with a schematic. 
I just referred to the table above, wiring point-to-point from the Pin column on the Pico to the MicroSD 0 column on the Transflash.
* Card Detect is optional. Some SD card sockets have no provision for it. 
Even if it is provided by the hardware, if you have no requirement for it you can skip it and save a Pico I/O pin.
* You can choose to use either or both of the Pico's SPIs.
* To add a second SD card on the same SPI, connect it in parallel, except that it will need a unique GPIO for the Card Select/Slave Select (CSn) and another for Card Detect (CD).
* Wires should be kept short and direct. SPI operates at HF radio frequencies.

### Pull Up Resistors
* The SPI MISO (DO on SD card, SPIx RX on Pico) is open collector (or tristate). It should be pulled up. The Pico internal gpio_pull_up is weak: around 56uA or 60kΩ. You might need to add an external pull up resistor of around ~5-10kΩ to 3.3v, depending on the SD card and the SPI baud rate.
* The SPI Slave Select (SS), or Chip Select (CS) line enables one SPI slave of possibly multiple slaves on the bus. It's best to pull CS up so that it doesn't float before the Pico GPIO is initialized.
* If resistors are used, trim the leads to keep the connections short and direct.

## Firmware:
* Follow instructions in [Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf) to set up the development environment.
* Install source code:
  `git clone --recurse-submodules https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico.git FreeRTOS+FAT+CLI`
* Customize:
  * Tailor `FreeRTOS+FAT+CLI/FreeRTOS+FAT+CLI/portable/RP2040/hw_config.c` to match hardware
  * Customize `FreeRTOS+FAT+CLI/example/CMakeLists.txt` to `pico_enable_stdio_uart` or `pico_enable_stdio_usb`
  * Build:
```  
   cd FreeRTOS+FAT+CLI/example
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
     
    data_log_demo:
     Launch data logging task
     
```
![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/IMG_1481.JPG "Prototype")

## Troubleshooting
* The first thing to try is lowering the SPI baud rate (see hw_config.c). This will also make it easier to use things like logic analyzers.
* Try another brand of SD card. Some handle the SPI interface better than others. (Most consumer devices like cameras or PCs use the SDIO interface.) I have had good luck with SanDisk.
* Tracing: Most of the source files have a couple of lines near the top of the file like:
```
#define TRACE_PRINTF(fmt, args...) // Disable tracing
//#define TRACE_PRINTF printf // Trace with printf
```
You can swap the commenting to enable tracing of what's happening in that file.
* Logic analyzer: for less than ten bucks, something like this [Comidox 1Set USB Logic Analyzer Device Set USB Cable 24MHz 8CH 24MHz 8 Channel UART IIC SPI Debug for Arduino ARM FPGA M100 Hot](https://smile.amazon.com/gp/product/B07KW445DJ/) and [PulseView - sigrok](https://sigrok.org/) make a nice combination for looking at SPI, as long as you don't run the baud rate too high. 
* 
## Next Steps
There is a demonstration data logging application in `FreeRTOS-FAT-CLI-for-RPi-Pico/src/data_log_demo.c`. 
It runs as a separate task, and can be launched from the CLI with the `data_log_demo` command.
(Stop it with the `die` command.)
It records the temperature as reported by the RP2040 internal Temperature Sensor once per second 
in files named something like `/sd0/data/2021-02-27/21.csv`.
Use this as a starting point for your own data logging application!

If you want to use FreeRTOS+FAT+CLI as a library embedded in another project, use something like:
  ```
  git submodule add git@github.com:carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico.git
  ```
  or
  ```
  git submodule add https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico.git
  ```
You will need to pick up the library in CMakeLists.txt:
```
add_subdirectory(FreeRTOS-FAT-CLI-for-RPi-Pico/FreeRTOS+FAT+CLI build)
target_link_libraries(_my_app_ FreeRTOS+FAT+CLI)
```  

Happy hacking!
![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/IMG_20210322_201928116.jpg "Prototype")

## Appendix: Adding Additional Cards
When you're dealing with information storage, it's always nice to have redundancy. There are many possible combinations of SPIs and SD cards. One of these is putting multiple SD cards on the same SPI bus, at a cost of one (or two) Pico I/O pins (depending on whether or you care about Card Detect). I will illustrate that example here. 

Name|SPI0|GPIO|Pin |SPI|MicroSD 0|MicroSD 1
----|----|----|----|---|---------|---------
CD1||14|19|||CD
CS1||15|20|SS or CS||CS
MISO|RX|16|21|DO|DO|DO
CS0||17|22|SS or CS|CS|CS
SCK|SCK|18|24|SCLK|SCK|SCK
MOSI|TX|19|25|DI|DI|DI
CD0||22|29||CD|
||||||
GND|||18, 23||GND|GND
3v3|||36||3v3|3v3

### Wiring: 
As you can see from the table above, the only new signals are CD1 and CS1. Otherwise, the new card is wired in parallel with the first card.
### Firmware:
* `sd_driver/hw_config.c` must be edited to add a new instance to `static sd_card_t sd_cards[]`

## Acknowledgement:
I would like to thank PicoCPP for [RPI-pico-FreeRTOS](https://github.com/PicoCPP/RPI-pico-FreeRTOS), which was my starting point for this project.
