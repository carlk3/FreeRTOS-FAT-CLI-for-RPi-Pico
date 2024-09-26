# FreeRTOS-FAT-CLI-for-RPi-Pico  
# v2.13.0
=============================
## C/C++ Library for SD Cards on the Pico

This project is essentially a 
[FreeRTOS+FAT](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/index.html)
[Media Driver](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/Creating_a_file_system_media_driver.html)
for the 
[Raspberry Pi Pico](https://www.raspberrypi.org/products/raspberry-pi-pico/),
using Serial Peripheral Interface (SPI),
based on 
[SDBlockDevice from Mbed OS 5](https://os.mbed.com/docs/mbed-os/v5.15/apis/sdblockdevice.html),
and/or a 4-bit Secure Digital Input Output (SDIO) driver derived from 
[ZuluSCSI-firmware](https://github.com/ZuluSCSI/ZuluSCSI-firmware). 
It is wrapped up in a complete runnable project, with a little command line interface, some self tests, and an example data logging application.

## What's new
### v2.13.0
Ported to SDK 2 and Pico 2.
### v2.12.0
Add support for running without Chip Select (CS) (formerly Slave Select [SS]).
### v2.11.0
* Add `spi_mode` to the hardware configuration.
For SPI attached cards, SPI Mode 3 can significantly improve performance.
See [SPI Controller Configuration](#spi-controller-configuration).
* Additional performance improvements in SPI driver.
### v2.10.0
* Make timeouts configurable. See [Timeouts](#timeouts).
### v2.9.0
* Add retries in SPI driver `sd_read_blocks`
### v2.8.0
* Migrate to **Raspberry Pi Pico SDK 2.0.0**
* CRC performance improvements for SPI
* Clean up `sd_write_blocks` in `sd_card_spi.c`
### v2.7.0
* Refactor SPI sd_write_blocks
* Drop support for SD Standard Capacity Memory Card (up to and including 2 GB). 
SDSC Card uses byte unit address and SDHC and SDXC Cards (CCS=1) use block unit address (512 Bytes unit).
### v2.6.2
Pick up Lab-Project-FreeRTOS-FAT bug fix: [Add missing error check #68](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-FAT/pull/68). 
The bug could cause a memory overwrite if the Media Driver returned an error.
### v2.6.1
Fix bug in SPI sd_write_blocks that caused single block writes to be sent with CMD25 WRITE_MULTIPLE_BLOCK
instead of CMD24 WRITE_BLOCK.
### v2.6.0
* Added
[include/file_stream.h](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/src/FreeRTOS%2BFAT%2BCLI/include/file_stream.h)
and
[src/file_stream.c](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/src/FreeRTOS%2BFAT%2BCLI/src/file_stream.c)
which use the C library's
[fopencookie—open a stream with custom callbacks](https://sourceware.org/newlib/libc.html#fopencookie)
API to put a buffered Standard Input/Output (stdio) wrapper around the
[FreeRTOS-Plus-FAT Standard API](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/Standard_File_System_API.html).
* Added
[examples/stdio_buffering/](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/tree/master/examples/stdio_buffering)
which shows how to use the C library's Standard Input/Output (stdio) buffering to achieve significant
(up to 4X) speedups in many applications.
* See [Appendix D: Performance Tuning Tips](#appendix-d-performance-tuning-tips).
### v2.5.2
* Fixed initialization problem when multiple SD cards share an SPI bus.
* Performance improvement for writing large contiguous blocks of data to SPI-attached SD cards. This is accomplished by avoiding sending "stop transmission" for as long as possible.
### v2.5.1
* Fixed locking bug in `sd_sync`. 
* Performance tweaks.
### v2.5.0 
* Added new example: [examples/wifi_httpd](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/tree/master/examples/wifi_httpd) demonstrates a Pico W WiFi web server serving files from an SD card.
* Substantial (~15%) performance improvement for writing large contiguous blocks of data to SDIO-attached SD cards. This is accomplished by avoiding sending "stop transmission" for as long as possible.
### v2.4.3
* Bug fix: Fix miscalculation in `get_num_sectors`.
This error was visible in `bench` in the reported disk capacity.
*  `command_line` example: Run `big_file_test` in separate task.
This frees up the CLI to run commands like `run-time-stats`.
### v2.4.2 
* Bug fix: `rp2040_sdio_tx_poll` called the DMA IRQ handler for any exception to "Verify that IRQ handler gets called even if we are in hardfault handler". 
However, it was using a mask for *all* exceptions, including PendSV and SysTick which are normal in FreeRTOS. This occasionally caused writes to fail with a CRC error.
* Unified DMA IRQ handling: merge SPI and SDIO DMA IRQ handling. 
Only add IRQ handler once. 
Fix bug where `rp2040_sdio_stop` always disabled the channel on DMA_IRQ_1.
* `command_line` example: Use the RTOS Daemon (Timer Service) Task instead of a separate task to execute unmount request from card detect interrupt
### v2.4.1
Pick up [Lab-Project-FreeRTOS-FAT](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-FAT) [Fix dynamic FAT variant detection](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-FAT/pull/56) 
### v2.4.0
Implement `ACMD42_SET_CLR_CARD_DETECT`: 
At power up the CS/DAT3 line has a 50KOhm pull up enabled in the SD card. 
This resistor serves two functions: Card detection and Mode Selection. 
This pull-up should be disconnected by the user, during regular data transfer, 
with SET_CLR_CARD_DETECT (ACMD42) command.
### v2.3.0
* `command_line` example enhancements:
  * `info` enhanced to report SD card Allocation Unit (AU_SIZE) or "segment" (available only on SDIO-attached cards)
  * `format` now creates a single primary partition and attempts to align this on an SD card segment.
### v2.2.0
* `FreeRTOS-Kernel` removed as a submodule of this library. 
It should be up to the application to manage the FreeRTOS library.
There is some increased risk of incompatibilities with FreeRTOS versions with this change.
See [Dependencies](#dependencies).
* Similarly, the `FreeRTOSFATConfig.h` and `FreeRTOSFATConfig.h` configuration files 
have been removed from this library. 
These are for application specific customization and should be provided by the application, 
not the library.
However, examples are provided in the example applications in the `examples` directory.
See [Other Application-Specific Customization](#other-application-specific-customization).
### v2.1.0
* Symetrical Multi Processing (SMP) enabled. 
`configNUM_CORES` got renamed to `configNUMBER_OF_CORES` in FreeRTOS,
so SMP was not working in v2.0.0.
* Multi Task Big File Test: like Big File Test, but using multiple tasks to write multiple files
### v2.0.0
* 4-wire SDIO support
* Rewritten Command Line Interface (CLI)

For required migration actions, see [Appendix A: Migration actions](#appendix-a-migration-actions).

*Note:* Release 1 remains available on the [v1.0.0 branch](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/tree/v1.0.0).

## Features
* Supports multiple SD cards, all in a common file system
* Supports desktop compatible SD card formats
* Supports 4-bit wide SDIO by PIO, or SPI using built in SPI controllers, or both
* Supports multiple SPIs
* Supports multiple SD Cards per SPI
* Supports multiple SDIO buses
* Supports Real Time Clock for maintaining file and directory time stamps
* Supports Cyclic Redundancy Check (CRC) for data integrity
* Compatible with Pico W
* Plus all the neat features provided by
[FreeRTOS+FAT](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/index.html)

## Limitations
* exFAT is not supported. Generally, if an SD card is formatted for exFAT you can reformat for FAT32. This library has the 
[facilities to do that](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/native_API/FF_Format.html), 
or you can use something like 
[SD Memory Card Formatter](https://www.sdcard.org/downloads/formatter/).
* This library currently does not support multiple partitions on an SD card. Neither does Windows.

## Resources Used
* SPI attached cards:
  * One or two Serial Peripheral Interface (SPI) controllers may be used.
  * For each SPI controller used, two DMA channels are claimed with `dma_claim_unused_channel`.
  * A configurable DMA IRQ is hooked with `irq_add_shared_handler` or `irq_set_exclusive_handler` (configurable) and enabled.
  * For each SPI controller used, one GPIO is needed for each of RX, TX, and SCK. Note: each SPI controller can only use a limited set of GPIOs for these functions.
  * For each SD card attached to an SPI controller:
      * (Optional, if there's only one SD card) A GPIO for slave (or "chip") select (SS or "CS"). (See [Running without Chip Select (CS) (formerly Slave Select [SS])](#running-without-chip-select-cs-formerly-slave-select-ss).)
      * (Optional) A GPIO for Card Detect (CD or "DET"). (See [Notes about Card Detect](#notes-about-card-detect).)
* SDIO attached cards:
  * A PIO block
  * Two DMA channels claimed with `dma_claim_unused_channel`
  * A configurable DMA IRQ is hooked with `irq_add_shared_handler` or `irq_set_exclusive_handler` (configurable) and enabled.
  * Six GPIOs for signal pins, and, optionally, another for CD (Card Detect). Four pins must be at fixed offsets from D0 (which itself can be anywhere):
    * CLK_gpio = D0_gpio - 2.
    * D1_gpio = D0_gpio + 1;
    * D2_gpio = D0_gpio + 2;
    * D3_gpio = D0_gpio + 3;

SPI and SDIO can share the same DMA IRQ.

For the complete 
[examples/command_line](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/tree/master/examples/command_line) application, 
configured for oneSDIO-attached card, `MinSizeRel` build, 
as reported by link flag `-Wl,--print-memory-usage`:
```
[build] Memory region         Used Size  Region Size  %age Used
[build]            FLASH:      160400 B         2 MB      7.65%
[build]              RAM:      221584 B       256 KB     84.53%
```
The high RAM consumption is because I chose to devote 192 kB to the FreeRTOS Heap4:
```
  #define configTOTAL_HEAP_SIZE                   192 * 1024
```
in [FreeRTOSConfig.h](https://www.freertos.org/a00110.html) on the theory that if you're running FreeRTOS, you're more likely to use [pvPortMalloc()](https://www.freertos.org/a00111.html) than [malloc()](https://man7.org/linux/man-pages/man3/malloc.3.html). `mount`ing the SD card takes 2504 bytes of heap. After running the `cvef` (Create and Verify Example Files) test:
```
> heap-stats
Configured total heap size:     196608
Free bytes in the heap now:     193480
Minimum number of unallocated bytes that have ever existed in the heap: 192424
```
so the maximum heap utilization was 4184 bytes, or about 1.6 % of the Pico's RAM.

## Performance
Writing and reading a file of 200 MiB of psuedorandom data on the same 
[Silicon Power 3D NAND U1 32GB microSD card](https://www.amazon.com/gp/product/B07RSXSYJC/) inserted into a 
[Pico Stackable, Plug & Play SD Card Expansion Module](https://forums.raspberrypi.com/viewtopic.php?t=356864)
at the default Pico system clock frequency (`clk_sys`) of 125 MHz, `MinSizeRel` build, using the command
[big_file_test bf 200 ](#appendix-b-operation-of-command_line-example)*x*.
once on SPI and one on SDIO.

* SDIO, baud rate 31,250,000 Hz:
  * Writing...
    * Elapsed seconds 20.7
    * Transfer rate 9.68 MiB/s (10.1 MB/s), or 9910 KiB/s (10148 kB/s) (81183 kbit/s)
  * Reading...
    * Elapsed seconds 16.0
    * Transfer rate 12.5 MiB/s (13.1 MB/s), or 12784 KiB/s (13091 kB/s) (104729 kbit/s)

* SPI, baud rate, baud rate 31,250,000 Hz:
  * Writing...
    * Elapsed seconds 86.5
    * Transfer rate 2.31 MiB/s (2.42 MB/s), or 2366 KiB/s (2423 kB/s) (19386 kbit/s)
  * Reading...
    * Elapsed seconds 90.4
    * Transfer rate 2.21 MiB/s (2.32 MB/s), or 2265 KiB/s (2320 kB/s) (18557 kbit/s)

Results from a
[port](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/examples/command_line/tests/bench.c)
of
[SdFat's bench](https://github.com/greiman/SdFat/blob/master/examples/bench/bench.ino):

* SDIO, baud rate 31,250,000 Hz:
  ```
  ...
  write speed and latency
  speed,max,min,avg
  KB/Sec,usec,usec,usec
  10922.7,12672,5723,5992
  11397.6,6009,5704,5752
  ...
  read speed and latency
  speed,max,min,avg
  KB/Sec,usec,usec,usec
  13273.1,4964,4907,4932
  13306.8,4940,4907,4918
  ...
  ```
* SPI, baud rate 31,250,000 Hz:
  ```
  ...
  write speed and latency
  speed,max,min,avg
  KB/Sec,usec,usec,usec
  2463.8,41867,26204,26594
  2491.9,26558,26175,26296
  ...
  read speed and latency
  speed,max,min,avg
  KB/Sec,usec,usec,usec
  2359.5,27882,27664,27766
  2361.7,27847,27675,27758
  ...
  ```
### Data Striping
For high data rate applications, it is possible to obtain higher write and read speeds 
by writing or reading to multiple SD cards simultaneously.

For example, using the command 
[mtbft 80 /sd0/bf](#appendix-b-operation-of-command_line-example) 
to write a 80 MiB file to a single SDIO-attached SD card, I got a transfer rate of 6.46 MiB/s.

Using the command 
[mtbft 40 /sd0/bf /sd3/bf](#appendix-b-operation-of-command_line-example) 
to write 40 MiB files on two SDIO-attached SD cards, I got a transfer rate of 12.4 MiB/s.

(This test includes the time to fill or check the buffer 
in the transfer rate calculation, 
so the actual write or read performance is higher.)

This gives a speedup of about 1.9 X for two cards vs a single card.

## Choosing the Interface Type(s)
The main reason to use SDIO is for the much greater speed that the 4-bit wide interface gets you. 
However, you pay for that in pins. 
SPI can get by with four GPIOs for the first card and one more for each additional card.
SDIO needs at least six GPIOs, and the 4 bits of the data bus have to be on consecutive GPIOs.
It is possible to put more than one card on an SDIO bus (each card has an address in the protocol), but at the higher speeds (higher than this implementation can do) the tight timing requirements don't allow it. I haven't tried it.
Running multiple SD cards on multiple SDIO buses works, but it does require a lot of pins and PIO resources.

You can mix and match the attachment types.
One strategy: use SDIO for cache and SPI for backing store. 
A similar strategy that I have used: SDIO for fast, interactive use, and SPI to offload data.

## Hardware
### My boards
* [Pico SD Card Development Board](https://forums.raspberrypi.com/viewtopic.php?p=2123146#p2123146)
![PXL_20230726_200951753a](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/assets/50121841/986ff919-e39e-40ef-adfb-78407f6e1e41)

* [Pico Stackable, Plug & Play SD Card Expansion Module](https://forums.raspberrypi.com/viewtopic.php?t=356864)
![PXL_20230926_212422091](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/assets/50121841/7edfea8c-59b0-491c-8321-45487bce9693)

### Prewired boards with SD card sockets
There are a variety of RP2040 boards on the market that provide an integrated µSD socket. As far as I know, most are useable with this library.
* [Maker Pi Pico](https://www.cytron.io/p-maker-pi-pico) works on SPI1. Looks fine for 4-bit wide SDIO.
* I don't think the [Pimoroni Pico VGA Demo Base](https://shop.pimoroni.com/products/pimoroni-pico-vga-demo-base) can work with a built in RP2040 SPI controller. It looks like RP20040 SPI0 SCK needs to be on GPIO 2, 6, or 18 (pin 4, 9, or 24, respectively), but Pimoroni wired it to GPIO 5 (pin 7). SDIO? For sure it could work with one bit SDIO, but I don't know about 4-bit. It looks like it *can* work, depending on what other functions you need on the board.
* The [SparkFun RP2040 Thing Plus](https://learn.sparkfun.com/tutorials/rp2040-thing-plus-hookup-guide/hardware-overview) works well on SPI1. For SDIO, the data lines are consecutive, but in the reverse order! I think that it could be made to work, but you might have to do some bit twiddling. A downside to this board is that it's difficult to access the signal lines if you want to look at them with, say, a logic analyzer or an oscilloscope.
* [Challenger RP2040 SD/RTC](https://ilabs.se/challenger-rp2040-sd-rtc-datasheet/) looks usable for SPI only. 
* [RP2040-GEEK](https://www.waveshare.com/wiki/RP2040-GEEK) This looks capable of 4 bit wide SDIO.
* Here is one list of RP2040 boards: [earlephilhower/arduino-pico: Raspberry Pi Pico Arduino core, for all RP2040 boards](https://github.com/earlephilhower/arduino-pico) Only a fraction of them have an SD card socket.
  
### Rolling your own
Prerequisites:
* Raspberry Pi Pico or some other kind of RP2040 board
* Something like the [Adafruit Micro SD SPI or SDIO Card Breakout Board](https://www.adafruit.com/product/4682)[^3] or [SparkFun microSD Transflash Breakout](https://www.sparkfun.com/products/544)

  ***Warning***: Avoid Aduino breakout boards like these: [Micro SD Storage Board Micro SD Card Modules](https://smile.amazon.com/gp/product/B07XF4Q1TT/). 
  They are designed for 5 V Arduino signals. 
  Many use simple resistor dividers to drop the signal voltage, and will not work properly with the 3.3 V Raspberry Pi Pico. 
  However, see [The 5V Arduino SD modules might work with a simple trick](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico/issues/83).
  
* Breadboard and wires
* Raspberry Pi Pico C/C++ SDK
* (Optional) A couple of ~10 kΩ - 50 kΩ resistors for pull-ups
* (Optional) 100 nF, 1 µF, and 10 µF capacitors for decoupling
* (Optional) 22 µH inductor for decoupling

![image](https://www.raspberrypi.com/documentation/microcontrollers/images/pico-pinout.svg "Pinout")
<!--
|       | SPI0  | GPIO  | Pin   | SPI       | MicroSD 0 | Description            | 
| ----- | ----  | ----- | ---   | --------  | --------- | ---------------------- |
| MISO  | RX    | 16    | 21    | DO        | DO        | Master In, Slave Out   |
| CS0   | CSn   | 17    | 22    | SS or CS  | CS        | Slave (or Chip) Select |
| SCK   | SCK   | 18    | 24    | SCLK      | CLK       | SPI clock              |
| MOSI  | TX    | 19    | 25    | DI        | DI        | Master Out, Slave In   |
| CD    |       | 22    | 29    |           | CD        | Card Detect            |
| GND   |       |       | 18,23 |           | GND       | Ground                 |
| 3v3   |       |       | 36    |           | 3v3       | 3.3 volt power         |
-->

Please see [here](https://docs.google.com/spreadsheets/d/1BrzLWTyifongf_VQCc2IpJqXWtsrjmG7KnIbSBy-CPU/edit?usp=sharing) for an example wiring table for an SPI attached card and an SDIO attached card on the same Pico. 
SPI and SDIO at 31.5 MHz are pretty demanding electrically. You need good, solid wiring, especially for grounds. A printed circuit board with a ground plane would be nice!

<!--
![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/IMG_1473.JPG "Prototype")
-->
![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/PXL_20230201_232043568.jpg "Protoboard, top")
![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/PXL_20230201_232026240_3.jpg "Protoboard, bottom")

### Construction
* The wiring is so simple that I didn't bother with a schematic. 
I just referred to the table above, wiring point-to-point from the Pin column on the Pico to the MicroSD 0 column on the Transflash.
* Card Detect is optional. Some SD card sockets have no provision for it. 
Even if it is provided by the hardware, if you have no requirement for it you can skip it and save a Pico I/O pin.
* You can choose to use none, either or both of the Pico's SPIs.
* You can choose to use zero or more PIO SDIO interfaces. [However, currently, the library has only been tested with zero or one.]
I don't know that there's much call for it.
* It's possible to put more than one card on an SDIO bus, but there is currently no support in this library for it.
* For SDIO, data lines D0 - D3 must be on consecutive GPIOs, with D0 being the lowest numbered GPIO.
Furthermore, the CMD signal must be on GPIO D0 GPIO number - 2, modulo 32. (This can be changed in the PIO code.)
* Wires should be kept short and direct. SPI operates at HF radio frequencies.

### Pull Up Resistors and other electrical considerations
* The SPI MISO (**DO** on SD card, **SPI**x **RX** on Pico) is open collector or tristateable push-pull, depending on the type of card.
[MMC](https://en.wikipedia.org/wiki/MultiMediaCard)s use an open collector bus, so it is imperative to pull this up if you want compatibility with MMCs.
However, modern SD cards use strong push-pull tristateable outputs and shouldn't need this pull up.
On some SD cards, you can configure the card's output drivers using the Driver Stage Register (DSR).[^4]).
The Pico internal `gpio_pull_up` is weak: around 56uA or 60kΩ.
If a pull up is needed, it's best to add an external pull up resistor of around 5-50 kΩ to 3.3v.
The internal `gpio_pull_up` can be disabled in the hardware configuration by setting the `no_miso_gpio_pull_up` attribute of the `spi_t` object.
* The SPI Slave Select (SS), or Chip Select (CS) line enables one SPI slave of possibly multiple slaves on the bus. This is what enables the tristate buffer for Data Out (DO), among other things. It's best to pull CS up so that it doesn't float before the Pico GPIO is initialized. It is imperative to pull it up for any devices on the bus that aren't initialized. For example, if you have two SD cards on one bus but the firmware is aware of only one card (see hw_config), don't let the CS float on the unused one. At power up the CS/DAT3 line has a 50 kΩ pull up enabled in the SD card, but I wouldn't necessarily count on that. It will be disabled if the card is initialized,
and it won't be enabled again until the card is power cycled. Also, the RP2040 defaults GPIO pins to pull down, which might override the SD card's pull up.
* Driving the SD card directly with the GPIOs is not ideal. Take a look at the [CM1624](https://www.onsemi.com/pdf/datasheet/cm1624-d.pdf). Unfortunately, it's a tiny little surface mount part -- not so easy to work with, but the schematic in the data sheet is still instructive. Besides the pull up resistors, it's a good idea to have 25 - 100 Ω series source termination resistors in each of the signal lines. 
This gives a cleaner signal, allowing higher baud rates. Even if you don't care about speed, it also helps to control the slew rate and current, which can reduce EMI and noise in general. (This can be important in audio applications, for example.) Ideally, the resistor should be as close as possible to the driving end of the line. That would be the Pico end for CS, SCK, MOSI, and the SD card end for MISO. For SDIO, the data lines are bidirectional, so, ideally, you'd have a source termination resistor at each end. Practically speaking, the clock is by far the most important to terminate, because each edge is significant. The other lines probably have time to bounce around before being clocked. Ideally, the resistance should be towards the low end for fat PCB traces, and towards the high end for flying wires, but if you have a drawer full of 47 Ω resistors they'll probably work well enough.
* It can be helpful to add a decoupling capacitor or three (e.g., 100 nF, 1 µF, and 10 µF) between 3.3 V and GND on the SD card. ChaN also [recommends](http://elm-chan.org/docs/mmc/mmc_e.html#hotplug) putting a 22 µH inductor in series with the Vcc (or "Vdd") line to the SD card.
* Good grounds are very important.
Remember, the current for all of the signal lines will flow back through the grounds.
There is a reason that the Pico devotes eight pins to GND.
* If your system allows hot removal and insertion of an SD card,
remember to allow for floating lines when the card is removed
and inrush current when the card is inserted. See [Cosideration to Bus Floating and Hot Insertion](http://elm-chan.org/docs/mmc/mmc_e.html#hotplug).
* Note: the [Adafruit Breakout Board](https://learn.adafruit.com/assets/93596) takes care of the pull ups and decoupling caps, but the Sparkfun one doesn't. And, you can never have too many decoupling caps.

## Notes about Card Detect
* There is one case in which Card Detect can be important: when the user can hot swap the physical card while the file system is mounted. In this case, the file system might have no way of knowing that the card was swapped, and so it will continue to assume that its prior knowledge of the FATs and directories is still valid. File system corruption and data loss are the likely results.
* If Card Detect is used, in order to detect a card swap there needs to be a way for the application to be made aware of a change in state when the card is removed. This could take the form of a GPIO interrupt (see
[examples/command_line](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/783e3dce97e65d65e592eb4e7f01e8033c2ade5a/examples/command_line/src/main.cpp#L21)), 
or polling.
* Some workarounds for absence of Card Detect:
  * If you don't care much about performance or battery life, you could mount the card before each access and unmount it after. This might be a good strategy for a slow data logging application, for example.
  * Some other form of polling: if the card is periodically accessed at rate faster than the user can swap cards, then the temporary absence of a card will be noticed, so a swap will be detected. For example, if a data logging application writes a log record to the card once per second, it is unlikely that the user could swap cards between accesses.

## Running without Chip Select (CS) (formerly Slave Select [SS])
If you have only one SD card, and you are short on GPIOs, you may be able to run without CS/SS.
I know of no guarantee that this will work for all SD cards.
The [Physical Layer Simplified Specification](https://www.sdcard.org/downloads/pls/) says
> Every command or data block is
> built of 8-bit bytes and is byte aligned with the CS signal...
> The card starts to count SPI bus clock cycle at the assertion of the CS signal...
> The host
> starts every bus transaction by asserting the CS signal low.

It doesn't say what happens if the CS signal is always asserted.
However, it worked for me with:
* [Silicon Power 3D NAND U1 32GB microSD card](https://www.amazon.com/gp/product/B07RSXSYJC/)
* [SanDisk 16GB Ultra microSDHC UHS-I Memory Card ](https://www.amazon.com/gp/product/B089DPCJS1/ref=ppx_yo_dt_b_search_asin_title?th=1)
* [PNY 16GB Elite Class 10 U1 microSDHC Flash Memory Card](https://www.amazon.com/gp/product/B08QDN7CVN/ref=ppx_yo_dt_b_search_asin_title)

You will need to pull down the CS/SS line on the SD card with hardware. (I.e., connect CS to GND. CS is active low.)

In the hardware configuration definition, set `ss_gpio` to -1.
See [An instance of `sd_spi_if_t` describes the configuration of one SPI to SD card interface.](#an-instance-of-sd_spi_if_t-describes-the-configuration-of-one-spi-to-sd-card-interface).

## Firmware
### Dependencies
* [FreeRTOS-Kernel](https://github.com/FreeRTOS/FreeRTOS-Kernel)
You will need to set FREERTOS_KERNEL_PATH in "Cmake: Configure Environment setting"
or via `-DFREERTOS_KERNEL_PATH=/path/to/FreeRTOS-Kernel` on the `CMake` command line
to point to the installation. 
See [FreeRTOS-Kernel/portable/ThirdParty/GCC/RP2040/](https://github.com/FreeRTOS/FreeRTOS-Kernel/tree/main/portable/ThirdParty/GCC/RP2040#using-this-port).
* [Lab-Project-FreeRTOS-FAT](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-FAT)
This will be picked up automatically as a submodule when you git clone this library if you specify the option 
`--recurse-submodules`.

### Procedure
* Follow instructions in [Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf) to set up the development environment.
* Install source code:
  ```bash
  git clone --recurse-submodules https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico.git FreeRTOS+FAT+CLI
  ```
* Customize:
  * Configure the code to match the hardware: see section 
  [Customizing for the Hardware Configuration](#customizing-for-the-hardware-configuration), below.
  * Customize `pico_enable_stdio_uart` and `pico_enable_stdio_usb` in CMakeLists.txt as you prefer. 
(See *4.1. Serial input and output on Raspberry Pi Pico* in [Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf) and *2.7.1. Standard Input/Output (stdio) Support* in [Raspberry Pi Pico C/C++ SDK](https://datasheets.raspberrypi.org/pico/raspberry-pi-pico-c-sdk.pdf).) 
* Build:
```bash
   cd FreeRTOS+FAT+CLI/examples/command_line
   mkdir build
   cd build
   cmake ..
   make
```   
  * Program the device
  * See [Appendix B: Operation of `command_line` example](#appendix-b-operation-of-command_line-example) for operation.
<!--
![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/IMG_1481.JPG "Prototype")
-->

## Customizing for the Hardware Configuration 
This library can support many different hardware configurations. 
Therefore, the hardware configuration is not defined in the library. 
Instead, the application must provide it. 
The configuration is defined in "objects" of type `spi_t` (see `sd_driver/spi.h`), 
`sd_spi_if_t`, `sd_sdio_if_t`, and `sd_card_t` (see `sd_driver/sd_card.h`). 
* Instances of `sd_card_t` describe the configuration of SD card sockets.
* Each instance of `sd_card_t` is associated (one to one) with an `sd_spi_if_t` or `sd_sdio_if_t` interface object, 
and points to it with `spi_if_p` or `sdio_if_p`[^5].
* Instances of `sdio_if_p` specify the configuration of an SDIO/PIO interface.
* Each instance of `sd_spi_if_t` is assocated (many to one) with an instance of `spi_t` and points to it with `spi_t *spi`. (It is a many to one relationship because multiple SD cards can share a single SPI bus, as long as each has a unique slave (or "chip") select (SS, or "CS") line.) It describes the configuration of a specific SD card's interface to a specific SPI hardware component.
* Instances of `spi_t` describe the configuration of the RP2040 SPI hardware components used.
There can be multiple objects (or "instances") of all three types.
Attributes (or "fields", or "members") of these objects specify which pins to use for what, baud rates, features like Card Detect, etc.
* Generally, anything not specified will default to `0` or `false`. (This is the user's responsibility if using Dynamic Configuration, but in a Static Configuration 
[see [Static vs. Dynamic Configuration](#static-vs-dynamic-configuration)], 
the C runtime initializes static memory to 0.)

![Illustration of the configuration dev_brd.hw_config.c](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/assets/50121841/0eedadea-f6cf-44cb-9b76-544ec74287d2)

Illustration of the configuration 
[dev_brd.hw_config.c](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/examples/command_line/config/dev_brd.hw_config.c)

### An instance of `sd_card_t` describes the configuration of one SD card socket
  ```C
struct sd_card_t {
    const char *device_name;
    const char *mount_point; // Must be a directory off the file system's root directory and must be an absolute path that starts with a forward slash (/)
    sd_if_t type;
    union {
        sd_spi_if_t *spi_if_p;
        sd_sdio_if_t *sdio_if_p;
    };
    bool use_card_detect;
    uint card_detect_gpio;    // Card detect; ignored if !use_card_detect
    uint card_detected_true;  // Varies with card socket; ignored if !use_card_detect
    bool card_detect_use_pull;
    bool card_detect_pull_hi;
//...
}
```
* `device_name` Device name. This is arbitrary, but if the string contains spaces the `command_line` example will have problems with it. This is the name that you pass to the `mount` command or the `FF_SDDiskInit` API call.
* `mount_point` An absolute path that specifies a directory off the file system's root directory where the SD card will appear after it is mounted and added
* `type` Type of interface: either `SD_IF_SPI` or `SD_IF_SDIO`
* `spi_if_p` or `sdio_if_p` Pointer to the instance `sd_spi_if_t` or `sd_sdio_if_t` that drives this SD card
* `use_card_detect` Whether or not to use Card Detect, meaning the hardware switch featured on some SD card sockets. This requires a GPIO pin.
* `card_detect_gpio` Ignored if not `use_card_detect`. GPIO number of the Card Detect, connected to the SD card socket's Card Detect switch (sometimes marked DET)
* `card_detected_true` Ignored if not `use_card_detect`. What the GPIO read returns when a card is present (Some sockets use active high, some low)
* `card_detect_use_pull` Ignored if not `use_card_detect`. If true, use the `card_detect_gpio`'s pad's Pull Up / Pull Down resistors; 
if false, no pull resistor is applied. 
Often, a Card Detect Switch is just a switch to GND or Vdd, 
and you need a resistor to pull it one way or the other to make logic levels.
* `card_detect_pull_hi` Ignored if not `use_card_detect`. Ignored if not `card_detect_use_pull`. Otherwise, if true, pull up; if false, pull down.

### An instance of `sd_sdio_if_t` describes the configuration of one SDIO to SD card interface.
  ```C
typedef struct sd_sdio_if_t {
    // See sd_driver\SDIO\rp2040_sdio.pio for SDIO_CLK_PIN_D0_OFFSET
    uint CLK_gpio;  // Must be (D0_gpio + SDIO_CLK_PIN_D0_OFFSET) % 32
    uint CMD_gpio;
    uint D0_gpio;      // D0
    uint D1_gpio;      // Must be D0 + 1
    uint D2_gpio;      // Must be D0 + 2
    uint D3_gpio;      // Must be D0 + 3
    PIO SDIO_PIO;      // either pio0 or pio1
    uint DMA_IRQ_num;  // DMA_IRQ_0 or DMA_IRQ_1
    bool use_exclusive_DMA_IRQ_handler;
    uint baud_rate;
    // Drive strength levels for GPIO outputs:
    // GPIO_DRIVE_STRENGTH_2MA 
    // GPIO_DRIVE_STRENGTH_4MA
    // GPIO_DRIVE_STRENGTH_8MA 
    // GPIO_DRIVE_STRENGTH_12MA
    bool set_drive_strength;
    enum gpio_drive_strength CLK_gpio_drive_strength;
    enum gpio_drive_strength CMD_gpio_drive_strength;
    enum gpio_drive_strength D0_gpio_drive_strength;
    enum gpio_drive_strength D1_gpio_drive_strength;
    enum gpio_drive_strength D2_gpio_drive_strength;
    enum gpio_drive_strength D3_gpio_drive_strength;
//...
} sd_sdio_t;
```
Specify `D0_gpio`, but pins `CLK_gpio`, `D1_gpio`, `D2_gpio`, and `D3_gpio` are at offsets from pin `D0_gpio` and are set implicitly.
The offsets are determined by `sd_driver\SDIO\rp2040_sdio.pio`.
  As of this writing, `SDIO_CLK_PIN_D0_OFFSET` is 30,
    which is -2 in mod32 arithmetic, so:
    
  * CLK_gpio = D0_gpio - 2
  * D1_gpio = D0_gpio + 1
  * D2_gpio = D0_gpio + 2
  * D3_gpio = D0_gpio + 3 

These pin assignments are set implicitly and must not be set explicitly.
* `CLK_gpio` RP2040 GPIO to use for Clock (CLK).
Implicitly set to `(D0_gpio + SDIO_CLK_PIN_D0_OFFSET) % 32` where `SDIO_CLK_PIN_D0_OFFSET` is defined in `sd_driver/SDIO/rp2040_sdio.pio`.
As of this writing, `SDIO_CLK_PIN_D0_OFFSET` is 30, which is -2 in mod32 arithmetic, so:
  * CLK_gpio = D0_gpio - 2
* `CMD_gpio` RP2040 GPIO to use for Command/Response (CMD)
* `D0_gpio` RP2040 GPIO to use for Data Line [Bit 0]. The PIO code requires D0 - D3 to be on consecutive GPIOs, with D0 being the lowest numbered GPIO.
* `D1_gpio` RP2040 GPIO to use for Data Line [Bit 1]. Implicitly set to D0_gpio + 1.
* `D2_gpio` RP2040 GPIO to use for Data Line [Bit 2]. Implicitly set to D0_gpio + 2.
* `D3_gpio` RP2040 GPIO to use for Card Detect/Data Line [Bit 3]. Implicitly set to D0_gpio + 3.
* `SDIO_PIO` Which PIO block to use. Defaults to `pio0`. Can be changed to avoid conflicts. 
If you try to use multiple SDIO-attached SD cards simultaneously on the same PIO block,
contention might lead to timeouts.
* `DMA_IRQ_num` Which IRQ to use for DMA. Defaults to DMA_IRQ_0. Set this to avoid conflicts with any exclusive DMA IRQ handlers that might be elsewhere in the system.
* `use_exclusive_DMA_IRQ_handler` If true, the IRQ handler is added with the SDK's `irq_set_exclusive_handler`. The default is to add the handler with `irq_add_shared_handler`, so it's not exclusive. 
* `baud_rate` The frequency of the SDIO clock in Hertz. This may be no higher than the system clock frequency divided by `CLKDIV` in `sd_driver\SDIO\rp2040_sdio.pio`, which is currently four. For example, if the system clock frequency is 125 MHz, `baud_rate` cannot exceed 31250000 (31.25 MHz). The default is 10 MHz.
This is used to divide the system clock frequency (`clk_sys`) to get a ratio to pass to the SDK's [sm_config_set_clkdiv](https://www.raspberrypi.com/documentation//pico-sdk/hardware.html#gae8c09c7a4372da95ad777faae51c5a24). As it says there, "An integer clock divisor of n will cause the state machine to run 1 cycle in every n. Note that for small n, the jitter introduced by a fractional divider (e.g. 2.5) may be unacceptable although it will depend on the use case."
In this case, n can be as little as four (which I would consider small).
The fractional divider essentially causes the frequency to vary in a range,
with the average being the requested frequency.
If the hardware is capable of running at the high end of the range,
you might as well run at that frequency all the time.
Therefore, I recommend choosing a baud rate that is some factor of the system clock frequency.
For example, if the system clock frequency is the default 125 MHz:
  ```C
      .baud_rate = 125 * 1000 * 1000 / 10,  // 12500000 Hz
  ```
  or
  ```C
      .baud_rate = 125 * 1000 * 1000 / 4  // 31250000 Hz
  ```
  The higher the baud rate, the faster the data transfer.
  However, the hardware might limit the usable baud rate.
  See [Pull Up Resistors and other electrical considerations](#pull-up-resistors-and-other-electrical-considerations).
* `set_drive_strength` If true, enable explicit specification of output drive strengths on `CLK_gpio`, `CMD_gpio`, and `D0_gpio` - `D3_gpio`. 
The GPIOs on RP2040 have four different output drive strengths, which are nominally 2, 4, 8 and 12mA modes.
If `set_drive_strength` is false, all will be implicitly set to 4 mA.
If `set_drive_strength` is true, each GPIO's drive strength can be set individually. Note that if it is not explicitly set, it will default to 0, which equates to `GPIO_DRIVE_STRENGTH_2MA` (2 mA nominal drive strength).

* ```
  CLK_gpio_drive_strength
  CMD_gpio_drive_strength 
  D0_gpio_drive_strength 
  D1_gpio_drive_strength 
  D2_gpio_drive_strength 
  D3_gpio_drive_strength 
  ``` 
  Ignored if `set_drive_strength` is false. Otherwise, these can be set to one of the following:
  ```
  GPIO_DRIVE_STRENGTH_2MA 
  GPIO_DRIVE_STRENGTH_4MA
  GPIO_DRIVE_STRENGTH_8MA 
  GPIO_DRIVE_STRENGTH_12MA
  ```
  You might want to do this for electrical tuning. A low drive strength can give a cleaner signal, with less overshoot and undershoot. 
  In some cases, this allows operation at higher baud rates.
  In other cases, the signal lines might have a lot of capacitance to overcome.
  Then, a higher drive strength might allow operation at higher baud rates.
  A low drive strength generates less noise. This might be important in, say, audio applications.

### An instance of `sd_spi_if_t` describes the configuration of one SPI to SD card interface.
```C
typedef struct sd_spi_if_t {
    spi_t *spi;
    // Slave select is here instead of in spi_t because multiple SDs can share an SPI.
    uint ss_gpio;                   // Slave select for this SD card
    // Drive strength levels for GPIO outputs:
    // GPIO_DRIVE_STRENGTH_2MA 
    // GPIO_DRIVE_STRENGTH_4MA
    // GPIO_DRIVE_STRENGTH_8MA 
    // GPIO_DRIVE_STRENGTH_12MA
    bool set_drive_strength;
    enum gpio_drive_strength ss_gpio_drive_strength;
} sd_spi_if_t;
```
* `spi` Points to the instance of `spi_t` that is to be used as the SPI to drive this interface
* `ss_gpio` Slave Select (SS) (or "Chip Select [CS]") GPIO for the SD card socket associated with this interface.
Set this to -1 to disable it.
(See [Running without Chip Select (CS) (formerly Slave Select [SS])](#running-without-chip-select-cs-formerly-slave-select-ss).)
*Note:* 0 is a valid GPIO number, so you must explicitly set it to -1 to disable it.
* `set_drive_strength` Enable explicit specification of output drive strength of `ss_gpio_drive_strength`. 
If false, the GPIO's drive strength will be implicitly set to 4 mA.
* `ss_gpio_drive_strength` Drive strength for the SS (or CS).
  Ignored if `set_drive_strength` is false. Otherwise, it can be set to one of the following:
  ```
  GPIO_DRIVE_STRENGTH_2MA 
  GPIO_DRIVE_STRENGTH_4MA
  GPIO_DRIVE_STRENGTH_8MA 
  GPIO_DRIVE_STRENGTH_12MA
  ```
### SPI Controller Configuration
An instance of `spi_t` describes the configuration of one RP2040 SPI controller.
```C
typedef struct spi_t {
    spi_inst_t *hw_inst;  // SPI HW
    uint miso_gpio;  // SPI MISO GPIO number (not pin number)
    uint mosi_gpio;
    uint sck_gpio;
    uint baud_rate;

    /* The different modes of the Motorola SPI protocol are:
    - Mode 0: When CPOL and CPHA are both 0, data sampled at the leading rising edge of the
    clock pulse and shifted out on the falling edge. This is the most common mode for SPI bus
    communication.
    - Mode 1: When CPOL is 0 and CPHA is 1, data sampled at the trailing falling edge and
    shifted out on the rising edge.
    - Mode 2: When CPOL is 1 and CPHA is 0, data sampled at the leading falling edge
    and shifted out on the rising edge.
    - Mode 3: When CPOL is 1 and CPHA is 1, data sampled at the trailing rising edge and
    shifted out on the falling edge. */
    uint spi_mode;

    uint DMA_IRQ_num; // DMA_IRQ_0 or DMA_IRQ_1
    bool use_exclusive_DMA_IRQ_handler;
    bool no_miso_gpio_pull_up;

    /* Drive strength levels for GPIO outputs:
        GPIO_DRIVE_STRENGTH_2MA, 
        GPIO_DRIVE_STRENGTH_4MA, 
        GPIO_DRIVE_STRENGTH_8MA,
        GPIO_DRIVE_STRENGTH_12MA */
    bool set_drive_strength;
    enum gpio_drive_strength mosi_gpio_drive_strength;
    enum gpio_drive_strength sck_gpio_drive_strength;

    // State variables:
// ...
} spi_t;
```
* `hw_inst` Identifier for the hardware SPI instance (for use in SPI functions). e.g. `spi0`, `spi1`, declared in `pico-sdk\src\rp2_common\hardware_spi\include\hardware\spi.h`
* `miso_gpio` SPI Master In, Slave Out (MISO) (also called "CIPO" or "Peripheral's SDO") GPIO number. This is connected to the SD card's Data Out (DO).
* `mosi_gpio` SPI Master Out, Slave In (MOSI) (also called "COPI", or "Peripheral's SDI") GPIO number. This is connected to the SD card's Data In (DI).
* `sck_gpio` SPI Serial Clock GPIO number. This is connected to the SD card's Serial Clock (SCK).
* `baud_rate` Frequency of the SPI Serial Clock, in Hertz. The default is `clk_sys` / 12.
  This is ultimately passed to the SDK's [spi_set_baudrate](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#ga37f4c04ce4165ac8c129226336a0b66c). This applies a hardware prescale and a post-divide to the *Peripheral clock* (`clk_peri`) (see section **4.4.2.3.** *Clock prescaler* in [RP2040 Datasheet](https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf)). 
  The *Peripheral clock* typically,
  but not necessarily, runs from `clk_sys`.
  Practically, the hardware limits the choices for the SPI frequency to `clk_peri` divided by an even number.
  For example, if `clk_peri` is `clk_sys` and `clk_sys` is running at the default 125 MHz,
  ```C
      .baud_rate = 125 * 1000 * 1000 / 10,  // 12500000 Hz
  ```
  or
  ```C
      .baud_rate = 125 * 1000 * 1000 / 4  // 31250000 Hz
  ```
  If you ask for 14,000,000 Hz, you'll actually get 12,500,000 Hz.
  The actual baud rate will be printed out if `USE_DBG_PRINTF` (see [Messages from the SD card driver](#messages-from-the-sd-card-driver)) is defined at compile time.
  The higher the baud rate, the faster the data transfer.
  At the maximum `clk_peri` frequency on RP2040 of 133MHz, the maximum peak bit rate in master mode is 62.5Mbps.
  However, the hardware (including the SD card) might limit the usable baud rate.
  See [Pull Up Resistors and other electrical considerations](#pull-up-resistors-and-other-electrical-considerations).
* `spi_mode` 0, 1, 2, or 3. 0 is the most common mode for SPI bus slave communication.
This controls the Motorola SPI frame format CPOL, clock polarity; and CPHA, clock phase.
SPI mode 0 (CPOL=0, CPHA=0) is the proper setting to control MMC/SDC, but mode 3 (CPOL=1, CPHA=1) also works as well in most cases[^6].
Mode 3 can be around 15% faster than mode 0, probably due to quirks of the ARM PrimeCell Synchronous Serial Port in the RP2040.
* `DMA_IRQ_num` Which IRQ to use for DMA. Defaults to DMA_IRQ_0. Set this to avoid conflicts with any exclusive DMA IRQ handlers that might be elsewhere in the system.
* `use_exclusive_DMA_IRQ_handler` If true, the IRQ handler is added with the SDK's `irq_set_exclusive_handler`. The default is to add the handler with `irq_add_shared_handler`, so it's not exclusive. 
* `no_miso_gpio_pull_up` According to the standard, an SD card's DO MUST be pulled up (at least for the old MMC cards). 
However, it might be done externally. If `no_miso_gpio_pull_up` is false, the library will set the RP2040 GPIO internal pull up.
* `set_drive_strength` Specifies whether or not to set the RP2040 GPIO pin drive strength.
If `set_drive_strength` is false, all will be implicitly set to 4 mA. 
If `set_drive_strength` is true, each GPIO's drive strength can be set individually. Note that if it is not explicitly set, it will default to 0, which equates to `GPIO_DRIVE_STRENGTH_2MA` (2 mA nominal drive strength).
* `mosi_gpio_drive_strength` SPI Master Out, Slave In (MOSI) drive strength, 
* and `sck_gpio_drive_strength` SPI Serial Clock (SCK) drive strength:
  Ignored if `set_drive_strength` is false. Otherwise, these can be set to one of the following:
  ```
  GPIO_DRIVE_STRENGTH_2MA 
  GPIO_DRIVE_STRENGTH_4MA
  GPIO_DRIVE_STRENGTH_8MA 
  GPIO_DRIVE_STRENGTH_12MA
  ```
  You might want to do this for electrical tuning. A low drive strength can give a cleaner signal, with less overshoot and undershoot.
  In some cases, this allows operation at higher baud rates.
  In other cases, the signal lines might have a lot of capacitance to overcome.
  Then, a higher drive strength might allow operation at higher baud rates.
  A low drive strength generates less noise. This might be important in, say, audio applications.

### You must provide a definition for the functions declared in `sd_driver/hw_config.h`
* `size_t sd_get_num()` Returns the number of SD cards  
* `sd_card_t *sd_get_by_num(size_t num)` Returns a pointer to the SD card "object" at the given (zero origin) index.  

### Static vs. Dynamic Configuration
The definition of the hardware configuration can either be built in at build time, which I'm calling "static configuration", or supplied at run time, which I call "dynamic configuration". 
In either case, the application simply provides an implementation of the functions declared in `sd_driver/hw_config.h`. 
* See 
[FreeRTOS-FAT-CLI-for-RPi-Pico/examples/simple_sdio/hw_config.c](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/examples/simple_sdio/hw_config.c)
 or 
 [FreeRTOS-FAT-CLI-for-RPi-Pico/examples/command_line/config/hw_config.c](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/examples/command_line/config/hw_config.c) for examples of static configuration.
* See 
[FreeRTOS-FAT-CLI-for-RPi-Pico/examples/dynamic_config/](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/tree/master/examples/dynamic_config)
for an example of dynamic configuration.
* One advantage of static configuration is that the fantastic GNU Linker (ld) strips out anything that you don't use.

## Other Application-Specific Customization
Two other files contain definitions that should be adjusted for your particular hardware and application requirements:
* `FreeRTOSConfig.h` FreeRTOS is customised using a configuration file called `FreeRTOSConfig.h`.
Every FreeRTOS application must have a `FreeRTOSConfig.h` header file in its pre-processor include path. See [Customisation](https://www.freertos.org/a00110.html).
* `FreeRTOSFATConfig.h` Applications that use FreeRTOS-Plus-FAT must provide a `FreeRTOSFATConfig.h` header file. 
See [FreeRTOS-Plus-FAT Configuration](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/Embedded_File_System_Configuration.html).

For examples of these files, see `examples/commmand_line/include`.

### Timeouts
Indefinite timeouts are normally bad practice, because they make it difficult to recover from an error.
Therefore, we have timeouts all over the place.
To make these configurable, they are collected in `sd_timeouts_t sd_timeouts` in `sd_timeouts.c`.
The definition has the `weak` attribute, so it can be overridden by user code.
For example, in `hw_config.c` you could have:
```C
sd_timeouts_t sd_timeouts = {
    .sd_command = 2000, // Timeout in ms for response
    .sd_command_retries = 3, // Times SPI cmd is retried when there is no response
//...
    .sd_sdio_begin = 1000, // Timeout in ms for response
    .sd_sdio_stopTransmission = 200, // Timeout in ms for response
};
```

### Messages
Sometimes problems arise when attempting to use SD cards. At the 
[FreeRTOS-Plus-FAT API](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/Standard_File_System_API.html)
level, it can be difficult to diagnose problems. You get an
[error number](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/stdio_API/errno.html), 
but it might just tell you `pdFREERTOS_ERRNO_EIO` ("I/O error"),
for example, without telling you what you need to know in order to fix the problem.
The library generates messages that might help. 
These are classed into Error, Informational, and Debug messages. 

#### Messages from the SD card driver
Two compile definitions control how these are handled in the SD card driver (or "
[media driver](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/Creating_a_file_system_media_driver.html)
"):
* `USE_PRINTF` If this is defined and not zero, 
these message output functions will use the Pico SDK's Standard Output (`stdout`).
* `USE_DBG_PRINTF` If this is not defined or is zero or `NDEBUG` is defined, 
`DBG_PRINTF` statements will be effectively stripped from the code.

Messages are sent using `EMSG_PRINTF`, `IMSG_PRINTF`, and `DBG_PRINTF` macros, which can be redefined (see 
[my_debug.h](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/src/FreeRTOS%2BFAT%2BCLI/include/my_debug.h)).
By default, these call `error_message_printf`, `info_message_printf`, and `debug_message_printf`, 
which are implemented as
[weak](https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html) functions, 
meaning that they can be overridden by strongly implementing them in user code. 
If `USE_PRINTF` is defined and not zero, the weak implementations will write to the Pico SDK's stdout. Otherwise, they will format the messages into strings and forward to `put_out_error_message`, `put_out_info_message`, and `put_out_debug_message`. These are implemented as weak functions that do nothing. You can override these to send the output somewhere.

#### Messages from FreeRTOS-Plus-FAT
FreeRTOS-Plus-FAT uses a macro called `FF_PRINTF`, which is defined in the 
[FreeRTOS-Plus-FAT Configuration file](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/Embedded_File_System_Configuration.html).
See [Other Application-Specific Customization](#other-application-specific-customization).

## Using the Application Programming Interface
In general, you use the [FreeRTOS-Plus-FAT APIs](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/Standard_File_System_API.html) in your application. One function that is not documented as part of the standard API but is conventional in FreeRTOS-Plus-FAT:

  `FF_Disk_t *FF_SDDiskInit( const char *pcName )` Initializes the "disk" (SD card) and returns a pointer to an 
  [FF_Disk_t](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/File_System_Media_Driver/FF_Disk_t.html)
  structure. This can then be passed to other functions in the [FreeRTOS-Plus-FAT Native API](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/Standard_File_System_API.html#native) such as `FF_Mount` and `FF_FS_Add`. The parameter `pcName` is the Device Name; `device_name` in 
  [struct sd_card_t](#an-instance-of-sd_card_t-describes-the-configuration-of-one-sd-card-socket).

A typical sequence would be:
* `FF_SDDiskInit`
* `FF_SDDiskMount`
* `FF_FS_Add`
* `ff_fopen`
* `ff_fwrite`
* `ff_fread`
* `ff_fclose`
* `FF_FS_Remove`
* `FF_Unmount`
* `FF_SDDiskDelete`

See [FreeRTOS-FAT-CLI-for-RPi-Pico/examples/simple_sdio/](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/tree/master/examples/simple_sdio) for an example.

You may call `sd_init_driver()` to explicitly initialize the block device driver.
It is called implicitly by `FF_SDDiskInit`,
but you might want to call it sooner.
For example, you might want to get the GPIOs configured before setting up a card detect interrupt handler.
(See [examples/command_line/src/unmounter.c](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/examples/command_line/src/unmounter.c).)
You might want to call it to get the SD cards into SPI mode so that they can share an SPI bus with other devices.
(See [Cosideration on Multi-slave Configuration](http://elm-chan.org/docs/mmc/mmc_e.html#spibus).)
`sd_init_driver()` must be called from a FreeRTOS task.

## Next Steps
* There is a simple example of using the API in the 
[FreeRTOS-FAT-CLI-for-RPi-Pico/examples/simple_sdio/](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/tree/master/examples/simple_sdio)
subdirectory.
* There is a demonstration data logging application in 
[FreeRTOS-FAT-CLI-for-RPi-Pico/examples/command_line/src/data_log_demo.c](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/examples/command_line/src/data_log_demo.c).
It runs as a separate task, and can be launched from the CLI with the `data_log_demo` command.
(Stop it with the `die` command.)
It records the temperature as reported by the RP2040 internal Temperature Sensor once per second 
in files named something like `/sd0/data/2021-02-27/21.csv`.
Use this as a starting point for your own data logging application!

If you want to use FreeRTOS+FAT+CLI as a library embedded in another project, use something like:
  ```bash
  git submodule add git@github.com:carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico.git
  ```
  or
  ```bash
  git submodule add https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico.git
  ```
  
You will need to pick up the library in CMakeLists.txt:
```CMake
add_subdirectory(FreeRTOS-FAT-CLI-for-RPi-Pico/FreeRTOS+FAT+CLI build)
target_link_libraries(_my_app_ FreeRTOS+FAT+CLI)
```  

Happy hacking!

## Future Directions
You are welcome to contribute to this project! Just submit a Pull Request in GitHub. Here are some ideas for future enhancements:
* Battery saving: at least stop the SDIO clock when it is not needed
* Support 1-bit SDIO
* Try multiple cards on a single SDIO bus
* [RP2040: Enable up to 42 MHz SDIO bus speed](https://github.com/ZuluSCSI/ZuluSCSI-firmware/tree/rp2040_highspeed_sdio)
* SD UHS Double Data Rate (DDR): clock data on both edges of the clock
<!--
![image](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/assets/50121841/8a28782e-84c4-40c8-8757-a063a4b83292)
-->
## Appendix A: Migration actions

### Migrating from Release [1.0.0](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/tree/v1.0.0)
* Directory restructuring: 
    * Examples have been moved to subdirectory `examples`.
    * Libraries `FreeRTOS+FAT+CLI`, `FreeRTOS-Kernel`, and `Lab-Project-FreeRTOS-FAT` have been moved to subdirectory `src`.
* The example previously called `example` is renamed `command_line`. The names and syntax of some CLI commands have changed, and new ones added. See [Appendix B: Operation of `command_line` example](#appendix-b-operation-of-command_line-example).
* `sd_card_t` attribute (or "field" or "member") `pcName` has been removed and replaced by `device_name` and `mount_point`. 
`device_name` is equivalent to the old `pcName`. `mount_point` specifies the directory name for the mount point in the root directory.
* The object model for hardware configuration has changed.
 If you are migrating a project from 
 [Release 1.0.0](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/tree/v1.0.0), 
 you will have to change the hardware  configuration customization. The `sd_card_t` now contains a new object that specifies the configuration of either an SPI interface or an SDIO interface. See the 
 [Customizing for the Hardware Configuration](#customizing-for-the-hardware-configuration) 
 section.
 
 For example, if you were using a `hw_config.c` containing 
 ```C
 static sd_card_t sd_cards[] = {  // One for each SD card
     {
         .pcName = "sd0",   // Name used to mount device
         .spi = &spis[0],  // Pointer to the SPI driving this card
         .ss_gpio = 17,    // The SPI slave select GPIO for this SD card//...
 ```        
 that would now become
 ```C
 static sd_spi_if_t spi_ifs[] = {
     { 
         .spi = &spis[0],          // Pointer to the SPI driving this card
         .ss_gpio = 17,             // The SPI slave select GPIO for this SD card
 //...
 static sd_card_t sd_cards[] = {  // One for each SD card
     {
         .device_name = "sd0",           // Name used to mount device
         .mount_point = "/sd0",
         .type = SD_IF_SPI,
         .spi_if_p = &spi_ifs[0],  // Pointer to the SPI interface driving this card
 //...
 ```


## Appendix B: Operation of `command_line` example
* Connect a terminal. [PuTTY](https://www.putty.org/) or `tio` work OK. For example:
  * `tio -m ODELBS /dev/ttyACM0`
* Press Enter to start the CLI. You should see a prompt like:
```
    > 
```
* The `help` command describes the available commands:
```
setrtc <DD> <MM> <YY> <hh> <mm> <ss>:
 Set Real Time Clock
 Parameters: new date (DD MM YY) new time in 24-hour format (hh mm ss)
        e.g.:setrtc 16 3 21 0 4 0

date:
 Print current date and time

format <device name>:
 Creates an FAT/exFAT volume on the device name.
        e.g.: format sd0

mount <device name> [device_name...]:
 Makes the specified device available at its mount point in the directory tree.
        e.g.: mount sd0

unmount <device name>:
 Unregister the work area of the volume

info <device name>:
 Print information about an SD card

cd <path>:
 Changes the current directory of the device name.
 <path> Specifies the directory to be set as current directory.
        e.g.: cd /dir1

mkdir <path>:
 Make a new directory.
 <path> Specifies the name of the directory to be created.
        e.g.: mkdir /dir1

rm [options] <pathname>:
 Removes (deletes) a file or directory
 <pathname> Specifies the path to the file or directory to be removed
 Options:
  -d Remove an empty directory
  -r Recursively remove a directory and its contents

cp <source file> <dest file>:
 Copies <source file> to <dest file>

mv <source file> <dest file>:
 Moves (renames) <source file> to <dest file>

pwd:
 Print Working Directory

ls [pathname]:
 List directory

cat <filename>:
 Type file contents

simple:
 Run simple FS tests

lliot <device name>
 !DESTRUCTIVE! Low Level I/O Driver Test
The SD card will need to be reformatted after this test.
        e.g.: lliot sd0

bench <device name>:
 A simple binary write/read benchmark

big_file_test <pathname> <size in MiB> <seed>:
 Writes random data to file <pathname>.
 Specify <size in MiB> in units of mebibytes (2^20, or 1024*1024 bytes)
        e.g.: big_file_test /sd0/bf 1 1
        or: big_file_test /sd1/big3G-3 3072 3

Alias for big_file_test

mtbft <size in MiB> <pathname 0> [pathname 1...]
Multi Task Big File Test
 pathname: Absolute path to a file (must begin with '/' and end with file name)

cvef:
 Create and Verify Example Files
Expects card to be already formatted and mounted

swcwdt:
 Stdio With CWD Test
Expects card to be already formatted and mounted.
Note: run cvef first!

loop_swcwdt:
 Run Create Disk and Example Files and Stdio With CWD Test in a loop.
Expects card to be already formatted and mounted.
Note: Stop with "die".

mtswcwdt:
 MultiTask Stdio With CWD Test
        e.g.: mtswcwdt

start_logger:
 Start Data Log Demo

die:
 Kill background tasks

undie:
 Allow background tasks to live again

task-stats:
 Show task statistics

heap-stats:
 Show heap statistics

run-time-stats:
 Displays a table showing how much processing time each FreeRTOS task has used

help:
 Shows this command help.
```    

![image](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/assets/50121841/8a28782e-84c4-40c8-8757-a063a4b83292)

## Appendix C: Adding Additional Cards
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

### Wiring
As you can see from the table above, the only new signals are CD1 and CS1. Otherwise, the new card is wired in parallel with the first card.
### Firmware
* [The hardware configuration](#customizing-for-the-hardware-configuration)
must be edited to add a new instance of 
[sd_card_t](#an-instance-of-sd_card_t-describes-the-configuration-of-one-sd-card-socket)
and its interface
[sd_sdio_if_t](#an-instance-of-sd_sdio_if_t-describes-the-configuration-of-one-sdio-to-sd-card-interface)
or
[sd_spi_if_t](#an-instance-of-sd_spi_if_t-describes-the-configuration-of-one-spi-to-sd-card-interface).

## Appendix D: Performance Tuning Tips
Obviously, if possible, use 4-bit SDIO instead of 1-bit SPI.
(See [Choosing the Interface Type(s)](#choosing-the-interface-types).)

Obviously, set the baud rate as high as you can. (See
[Customizing for the Hardware Configuration](#customizing-for-the-hardware-configuration)).

If you are using SPI, try SPI mode 3 (CPOL=1, CPHA=1) instead of 0 (CPOL=0, CPHA=0). (See
[SPI Controller Configuration](#spi-controller-configuration).) This could buy a 15% speed boost.

TL;DR: In general, it is much faster to transfer a given number of bytes in one large write (or read) 
than to transfer the same number of bytes in multiple smaller writes (or reads). 

One quick and easy way to speed up many applications is to take advantage of the buffering built into the C library for standard I/O streams.
(See 
[fopencookie—open a stream with custom callbacks](https://sourceware.org/newlib/libc.html#fopencookie) and 
[setvbuf—specify file or stream buffering](https://sourceware.org/newlib/libc.html#setvbuf)). 
The application would use [fprintf](https://sourceware.org/newlib/libc.html#sprintf) instead of 
[ff_fprintf](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/stdio_API/ff_fprintf.html), 
or 
[fwrite](https://sourceware.org/newlib/libc.html#fwrite) instead of 
[ff_fwrite](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/stdio_API/ff_fwrite.html),
for example.
If you are using SDIO, it is critically important for performance to use `setvbuf` 
to set the buffer to an `aligned` buffer. 
Also, the buffer should be a multiple of the SD block size, 512 bytes, in size.
For example:
```C
    static char vbuf[1024] __attribute__((aligned));
    int err = setvbuf(file_p, vbuf, _IOFBF, sizeof vbuf);
```
If you have a record-oriented application, 
and the records are multiples of 512 bytes in size,
you might not see a significant speedup.
However, if, for example, you are writing text files with 
no fixed record length, the speedup can be great.
See
[examples/stdio_buffering/](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/tree/master/examples/stdio_buffering).

Now, for the details:
The modern SD card is a block device, meaning that the smallest addressable unit is a a block (or "sector") of 512 bytes. So, it helps performance if your write size is a multiple of 512. If it isn't, partial block writes involve reading the existing block, modifying it in memory, and writing it back out. With all the space in SD cards these days, it can be well worth it to pad a record length to a multiple of 512.

Generally, flash memory has to be erased before it can be written, and the minimum erase size is the "allocation unit" or "segment":

> AU (Allocation Unit): is a physical boundary of the card and consists of one or more blocks and its
size depends on each card. The maximum AU size is defined for memory capacity. Furthermore AU
is the minimal unit in which the card guarantees its performance for devices which complies with
Speed Class Specification. The information about the size and the Speed Class are stored in the
SD Status.

> -- SD Card Association; Physical Layer Specification Version 3.01

There is a controller in each SD card running all kinds of internal processes. When an amount of data to be written is smaller than a segment, the segment is read, modified in memory, and then written again. SD cards use various strategies to speed this up. Most implement a "translation layer". For any I/O operation, a translation from virtual to physical address is carried out by the controller. If data inside a segment is to be overwritten, the translation layer remaps the virtual address of the segment to another erased physical address. The old physical segment is marked dirty and queued for an erase. Later, when it is erased, it can be reused. Usually, SD cards have a cache of one or more segments for increasing the performance of read and write operations. The SD card is a "black box": much of this is invisible to the user, except as revealed in the Card-Specific Data register (CSD), SD_STATUS, and the observable performance characteristics. So, the write times are far from deterministic.

The Allocation Unit is typically 4 MiB for a 16 or 32 GB card, for example. Of course, nobody is going to be using 4 MiB write buffers on a Pico, but the AU is still important. For good performance and wear tolerance, it is recommended that the "disk partition" be aligned to an AU boundary. [SD Memory Card Formatter](https://www.sdcard.org/downloads/formatter/) makes this happen. For my 16 GB card, it set "Partition Starting Offset	4,194,304 bytes". This accomplished by inserting "hidden sectors" between the actual start of the physical media and the start of the volume. Also, it might be helpful to have your write size be some factor of the segment size.

There are more variables at the file system level. The FAT "allocation unit" (not to be confused with the SD card "allocation unit"), also known as "cluster", is a unit of "disk" space allocation for files. These are identically sized small blocks of contiguous space that are indexed by the File Allocation Table. When the size of the allocation unit is 32768 bytes, a file with 100 bytes in size occupies 32768 bytes of disk space. The space efficiency of disk usage gets worse with increasing size of allocation unit, but, on the other hand, the read/write performance increases. Therefore the size of allocation unit is a trade-off between space efficiency and performance. This is something you can change by formatting the SD card. See 
[FF_Format](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/native_API/FF_Format.html) 
and 
[Description of Default Cluster Sizes for FAT32 File System](https://support.microsoft.com/en-us/topic/description-of-default-cluster-sizes-for-fat32-file-system-905ea1b1-5c4e-a03f-3863-e4846a878d31). 
Again, there might be some advantage to making your write size be some factor or multiple of the FAT allocation unit.
The `info` command in [examples/command_line](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/tree/master/examples/command_line) reports the allocation unit.

[File fragmentation](https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system#Fragmentation) can lead to long access times. 
Fragmented files can result from multiple files being incrementally extended in an interleaved fashion. 
One strategy to avoid fragmentation is to pre-allocate files to their maximum expected size, 
then reuse these files at run time. 
Since a flash memory erase block is typically filled with 0xFF after an erase
(although some cards use 0x00), 
you could write a file full of 0xFF bytes (chosen to avoid flash memory "wear") ahead of time. 
(Also, see [FAT Volume Image Creator](http://elm-chan.org/fsw/ff/res/mkfatimg.zip) (Pre-creating built-in FAT volume).)
Then 
[ff_fopen](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/stdio_API/ff_fopen.html) 
it in mode "r+" at run time.
Obviously, you will need some way to keep track of how much valid data is in the file. 
You could use a file header.
Alternatively, if the file contains text, you could write an End-Of-File (EOF) character. 
In DOS, this is the character 26, which is the Control-Z character.
Alternatively, if the file contains records, each record could contain a magic number or checksum, so you can easily tell when you've reached the end of the valid records.
(This might be an obvious choice if you're padding the record length to a multiple of 512 bytes.)

For SDIO-attached cards, alignment of the read or write buffer is quite important for performance.
This library uses DMA with `DMA_SIZE_32`, and the read and write addresses must always be aligned to the current transfer size,
i.e., four bytes.
(For example, you could specify that the buffer has [\_\_attribute\_\_ ((aligned (4))](https://gcc.gnu.org/onlinedocs/gcc-3.1.1/gcc/Type-Attributes.html).)
If the buffer address is not aligned, the library copies each block into a temporary buffer that is aligned and then writes it out, one block at a time.
(The SPI driver uses `DMA_SIZE_8` so the alignment isn't important.)

For a logging type of application, opening and closing a file for each update is hugely inefficient,
but if you can afford the time it can be a good way to minimize data loss in the event
of an unexpected power loss or that kind of thing.
You can also try to find a middle ground by periodically closing
and reopening a file, or switching to a new file.
A well designed directory structure can act as a sort of
hierarchical database for rapid retrieval of records
distributed across many small files.

## Appendix E: Troubleshooting
* **Check your grounds!** Maybe add some more if you were skimpy with them. The Pico has six of them.
* Turn on `DBG_PRINTF`. (See #messages-from-the-sd-card-driver.) For example, in `CMakeLists.txt`, 
  ```CMake
  add_compile_definitions(USE_PRINTF USE_DBG_PRINTF)
  ```
  You might see a clue in the messages.
* Power cycle the SD card. Once an SD card is in SPI mode, the only way to get it back to SD mode is to power cycle it.
At power up, an SD card's CS/DAT3 line has a 50 kΩ pull up enabled in the card, but it will be disabled if the card is initialized,
and it won't be enabled again until the card is power cycled.
* Try lowering the SPI or SDIO baud rate (e.g., in `hw_config.c`). This will also make it easier to use things like logic analyzers.
  * For SPI, this is in the
  [spi_t](#an-instance-of-spi_t-describes-the-configuration-of-one-rp2040-spi-controller) instance.
  * For SDIO, this is in the 
  [sd_sdio_if_t](#an-instance-of-sd_sdio_if_t-describes-the-configuration-of-one-sdio-to-sd-card-interface) instance.
* Make sure the SD card(s) are getting enough power. Try an external supply. Try adding a decoupling capacitor between Vcc and GND. 
  * Hint: check voltage while formatting card. It must be 2.7 to 3.6 volts. 
  * Hint: If you are powering a Pico with a PicoProbe, try adding a USB cable to a wall charger to the Pico under test.
* Try another brand of SD card. Some handle the SPI interface better than others. (Most consumer devices like cameras or PCs use the SDIO interface.) I have had good luck with SanDisk, PNY, and  Silicon Power.
* Tracing: Most of the source files have a couple of lines near the top of the file like:
```C
#define TRACE_PRINTF(fmt, args...) // Disable tracing
//#define TRACE_PRINTF printf // Trace with printf
```
You can swap the commenting to enable tracing of what's happening in that file.
* Logic analyzer: for less than ten bucks, something like this [Comidox 1Set USB Logic Analyzer Device Set USB Cable 24MHz 8CH 24MHz 8 Channel UART IIC SPI Debug for Arduino ARM FPGA M100 Hot](https://smile.amazon.com/gp/product/B07KW445DJ/) and [PulseView - sigrok](https://sigrok.org/) make a nice combination for looking at SPI, as long as you don't run the baud rate too high. 
* Get yourself a protoboard and solder everything. So much more reliable than solderless breadboard!
* Better yet, go to somwhere like [JLCPCB](https://jlcpcb.com/) and get a printed circuit board!


[^3]: In my experience, the Card Detect switch on these doesn't work worth a damn. This might not be such a big deal, because according to [Physical Layer Simplified Specification](https://www.sdcard.org/downloads/pls/) the Chip Select (CS) line can be used for Card Detection: "At power up this line has a 50KOhm pull up enabled in the card... For Card detection, the host detects that the line is pulled high."
However, the Adafruit card has it's own 47 kΩ pull up on CS - Card Detect / Data Line [Bit 3], rendering it useless for Card Detection.

[^4]: [Physical Layer Simplified Specification](https://www.sdcard.org/downloads/pls/)

[^5]: Rationale: Instances of `sd_spi_if_t` or `sd_sdio_if_t` are separate objects instead of being embedded in `sd_card_t` objects because `sd_sdio_if_t` carries a lot of state information with it (including things like data buffers). The union of the two types has the size of the largest type, which would result in a lot of wasted space in instances of `sd_spi_if_t`. I had another solution using `malloc`, but some people are frightened of `malloc` in embedded systems.

[^6]: *SPI Mode* in [How to Use MMC/SDC](http://elm-chan.org/docs/mmc/mmc_e.html#spimode)
