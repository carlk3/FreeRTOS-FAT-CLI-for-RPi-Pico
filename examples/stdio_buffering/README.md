# stdio_buffering

This example illustrates the speedup that can be provided by taking advantage of the 
C library's Standard Input/Output (stdio) buffering.

Typical output:
```
SimpleTask: Hello, world!
Time without buffering: 1374 ms
Time with buffering: 322 ms
Goodbye, world!
```
showing a speedup of greater than 4X.

If you have a record-oriented application, 
and the records are multiples of 512 bytes in size,
you might not see a significant speedup.
However, if, for example, you are writing text files with 
no fixed record length, the speedup can be great.
