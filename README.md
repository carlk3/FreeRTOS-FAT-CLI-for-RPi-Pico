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

## Resources Used
* At least one (depending on configuration) of the two Serial Peripheral Interface (SPI) controllers is used.
* For each SPI controller used, two DMA channels are claimed with `dma_claim_unused_channel`.
* DMA_IRQ_0 is hooked with `irq_add_shared_handler` and enabled.
* For each SPI controller used, one GPIO is needed for each of RX, TX, and SCK. Note: each SPI controller can only use a limited set of GPIOs for these functions.
* For each SD card attached to an SPI controller, a GPIO is needed for CS, and, optionally, another for CD (Card Detect).
<!--* `size simple_example.elf`
```
   text	   data	    bss	    dec	    hex	filename
  68428	     44	   4388	  72860	  11c9c	simple_example.elf
```-->

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
* Something like the [SparkFun microSD Transflash Breakout](https://www.sparkfun.com/products/544)
* Breadboard and wires
* Raspberry Pi Pico C/C++ SDK
* (Optional) A couple of ~5-10kΩ resistors for pull-ups
* (Optional) A couple of ~100 pF capacitors for decoupling

![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/IMG_1478.JPG "Prototype")

## Dependencies:
* [FreeRTOS-Kernel](https://github.com/FreeRTOS/FreeRTOS-Kernel)
* [Lab-Project-FreeRTOS-FAT](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-FAT)


![image](https://www.raspberrypi.org/documentation/microcontrollers/images/Pico-R3-SDK11-Pinout.svg "Pinout")

|       | SPI0  | GPIO  | Pin   | SPI       | MicroSD 0 | Description            | 
| ----- | ----  | ----- | ---   | --------  | --------- | ---------------------- |
| MISO  | RX    | 16    | 21    | DO        | DO        | Master In, Slave Out   |
| CS0   | CSn   | 17    | 22    | SS or CS  | CS        | Slave (or Chip) Select |
| SCK   | SCK   | 18    | 24    | SCLK      | CLK       | SPI clock              |
| MOSI  | TX    | 19    | 25    | DI        | DI        | Master Out, Slave In   |
| CD    |       | 22    | 29    |           | CD        | Card Detect            |
| GND   |       |       | 18,23 |           | GND       | Ground                 |
| 3v3   |       |       | 36    |           | 3v3       | 3.3 volt power         |

## Construction:
* The wiring is so simple that I didn't bother with a schematic. 
I just referred to the table above, wiring point-to-point from the Pin column on the Pico to the MicroSD 0 column on the Transflash.
* Card Detect is optional. Some SD card sockets have no provision for it. 
Even if it is provided by the hardware, if you have no requirement for it you can skip it and save a Pico I/O pin.
* You can choose to use either or both of the Pico's SPIs.
* Wires should be kept short and direct. SPI operates at HF radio frequencies.

### Pull Up Resistors
* The SPI MISO (**DO** on SD card, **SPI**x **RX** on Pico) is open collector (or tristate). It should be pulled up. The Pico internal gpio_pull_up is weak: around 56uA or 60kΩ. It's best to add an external pull up resistor of around 5kΩ to 3.3v. You might get away without one if you only run one SD card and don't push the SPI baud rate too high.
* The SPI Slave Select (SS), or Chip Select (CS) line enables one SPI slave of possibly multiple slaves on the bus. This is what enables the tristate buffer for Data Out (DO), among other things. It's best to pull CS up so that it doesn't float before the Pico GPIO is initialized. It is imperative to pull it up for any devices on the bus that aren't initialized. For example, if you have two SD cards on one bus but the firmware is aware of only one card (see hw_config); you can't let the CS float on the unused one. 

## Notes about prewired boards with SD card sockets:
* I don't think the [Pimoroni Pico VGA Demo Base](https://shop.pimoroni.com/products/pimoroni-pico-vga-demo-base) can work with a built in RP2040 SPI controller. It looks like RP20040 SPI0 SCK needs to be on GPIO 2, 6, or 18 (pin 4, 9, or 24, respectively), but Pimoroni wired it to GPIO 5 (pin 7).
* The [SparkFun RP2040 Thing Plus](https://learn.sparkfun.com/tutorials/rp2040-thing-plus-hookup-guide/hardware-overview) looks like it should work, on SPI1.
  * For SparkFun RP2040 Thing Plus:

    |       | SPI0  | GPIO  | Description            | 
    | ----- | ----  | ----- | ---------------------- |
    | MISO  | RX    | 12    | Master In, Slave Out   |
    | CS0   | CSn   | 09    | Slave (or Chip) Select |
    | SCK   | SCK   | 14    | SPI clock              |
    | MOSI  | TX    | 15    | Master Out, Slave In   |
    | CD    |       |       | Card Detect            |
  
* [Maker Pi Pico](https://www.cytron.io/p-maker-pi-pico) looks like it could work on SPI1. It has CS on GPIO 15, which is not a pin that the RP2040 built in SPI1 controller would drive as CS, but this driver controls CS explicitly with `gpio_put`, so it doesn't matter.

## Firmware:
* Follow instructions in [Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf) to set up the development environment.
* Install source code:
  `git clone --recurse-submodules https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico.git FreeRTOS+FAT+CLI`
* Customize:
  * Tailor `FreeRTOS+FAT+CLI/FreeRTOS+FAT+CLI/portable/RP2040/hw_config.c` to match hardware
    * Customize `pico_enable_stdio_uart` and `pico_enable_stdio_usb` in CMakeLists.txt as you prefer. 
(See *4.1. Serial input and output on Raspberry Pi Pico* in [Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf) and *2.7.1. Standard Input/Output (stdio) Support* in [Raspberry Pi Pico C/C++ SDK](https://datasheets.raspberrypi.org/pico/raspberry-pi-pico-c-sdk.pdf).) 
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
* Make sure the SD card(s) are getting enough power. Try an external supply. Try adding a decoupling capacitor between Vcc and GND. 
  * Hint: check voltage while formatting card. It must be 2.7 to 3.6 volts. 
  * Hint: If you are powering a Pico with a PicoProbe, try adding a USB cable to a wall charger to the Pico under test.
* Try another brand of SD card. Some handle the SPI interface better than others. (Most consumer devices like cameras or PCs use the SDIO interface.) I have had good luck with SanDisk.
* Tracing: Most of the source files have a couple of lines near the top of the file like:
```
#define TRACE_PRINTF(fmt, args...) // Disable tracing
//#define TRACE_PRINTF printf // Trace with printf
```
You can swap the commenting to enable tracing of what's happening in that file.
* Logic analyzer: for less than ten bucks, something like this [Comidox 1Set USB Logic Analyzer Device Set USB Cable 24MHz 8CH 24MHz 8 Channel UART IIC SPI Debug for Arduino ARM FPGA M100 Hot](https://smile.amazon.com/gp/product/B07KW445DJ/) and [PulseView - sigrok](https://sigrok.org/) make a nice combination for looking at SPI, as long as you don't run the baud rate too high. 
* Get yourself a protoboard and solder everything. So much more reliable than solderless breadboard!
![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/PXL_20211214_165648888.MP.jpg)
## Next Steps
* There is a simple example of using the API in the `simple_example` subdirectory.
* There is a demonstration data logging application in `FreeRTOS-FAT-CLI-for-RPi-Pico/src/data_log_demo.c`. 
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
When you're dealing with information storage, it's always nice to have redundancy. There are many possible combinations of SPIs and SD cards. One of these is putting multiple SD cards on the same SPI bus, at a cost of one (or two) additional Pico I/O pins (depending on whether or you care about Card Detect). I will illustrate that example here. 

To add a second SD card on the same SPI, connect it in parallel, except that it will need a unique GPIO for the Card Select/Slave Select (CSn) and another for Card Detect (CD) (optional).

Name|SPI0|GPIO|Pin |SPI|SDIO|MicroSD 0|MicroSD 1
----|----|----|----|---|----|---------|---------
CD1||14|19||||CD
CS1||15|20|SS or CS|DAT3||CS
MISO|RX|16|21|DO|DAT0|DO|DO
CS0||17|22|SS or CS|DAT3|CS|
SCK|SCK|18|24|SCLK|CLK|SCK|SCK
MOSI|TX|19|25|DI|CMD|DI|DI
CD0||22|29|||CD|
|||||||
GND|||18, 23|||GND|GND
3v3|||36|||3v3|3v3

### Wiring: 
As you can see from the table above, the only new signals are CD1 and CS1. Otherwise, the new card is wired in parallel with the first card.
### Firmware:
* `sd_driver/hw_config.c` must be edited to add a new instance to `static sd_card_t sd_cards[]`

