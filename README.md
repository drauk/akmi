# akmi
akmi: Alan Kennington's Music Interpreter

This directory contains software for real-time performance of music on MIDI synthesisers, or optionally writing the output to MIDI files.

This software was originally written to run on an Atari ST, but it will also compile and run on an SLC sparc station with SunOS 4.1.3, especially if it has a serial to MIDI converter fitted to one of its serial ports. It can also be compiled and run as a MIDI file compiler on a linux PC. I might get the real-time playing functions ported to linux some day too.

This code actually compiles and works perfectly well with gcc 4.5.0 on OpenSuSE linux 11.3. But with gcc 4.8.1 on OpenSuSE linux 13.1, it doesn't yet compile successfully. I'll try to fix this some time.
