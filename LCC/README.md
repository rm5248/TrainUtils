# LibLCC

LibLCC is a C library used for communicating with the LCC(Layout Command and Control)
protocol.

The library is designed to run on any system with a reasonable C compiler.  It has
been tested to run on Linux and microcontrollers.

## Arduino Microcontrollers

LibLCC has been validated to work with Arduino boards(Uno).  When run on an Arduino,
almost all memory is statically allocated.  The library requires approx. 1k of RAM
in order to run.

# Simple Usage

The first thing to do is to create an `lcc_context` that keeps track of our state.
Next, various features can be enabled by creating other contexts, such as
memory, datagram support, event support.

See the examples/ folder for Arduino examples.  These examples can easily
be adapted to Linux, and likely Windows as well.

# Bugs / feedback

The offical source is a monorepo here: https://github.com/rm5248/TrainUtils/tree/master/LCC
Arduino sources are located here: https://github.com/rm5248/liblcc-arduino
Note that the Arduino sources are a manually-updated mirror of what is in the TrainUtils
project, and as such any and all pull requests/bug reports should be filed against
the TrainUtils project.

# License

GPL-2.0
