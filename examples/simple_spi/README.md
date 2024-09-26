

# Simple Example for an SPI-attached SD card

### Overview

This program is a simple demonstration of writing to an SD card using the
[FreeRTOS+FAT API](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/Standard_File_System_API.html).
It initializes the standard input/output, mounts an SD card, opens a file, writes to it, closes it, and then unmounts the SD card.

### Features

* Mounts an SD card
* Opens a file in append mode and writes a message
* Closes the file and unmounts the SD card
* Prints success and failure messages to the STDIO console

### Requirements
* You will need to
[customize](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico?tab=readme-ov-file#customizing-for-the-hardware-configuration)
the `hw_config.c` file to match your hardware.
* FREERTOS_KERNEL_PATH must be set in the CMake Configure Environment
