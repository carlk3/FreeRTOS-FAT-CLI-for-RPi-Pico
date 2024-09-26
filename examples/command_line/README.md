# command_line example
This is a comprehensive example demonstrating many capabilities of the FreeRTOS-FAT-CLI-for-RPi-Pico library.
It also can be used to launch some test programs.

It provides a command line interface; something like busybox or DOS.

## Operation
* Connect a terminal. Something like [PuTTY](https://www.putty.org/) or `tio` works OK. 
* For example:
  * `tio -m ODELBS /dev/ttyACM0`
* Press Enter to start the CLI. You should see a prompt like:
```
    > 
```    
* The `help` command describes the available commands:
```    
> help
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
 Changes the current directory.
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

bft:
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
