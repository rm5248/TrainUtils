# Cab bus / loconet utilities

Directories here:

CabBusCommunications - Talks to the CAB bus.  Handles the protocol conversions.
  All in C code.
LocoNetCommunications - Talks to LocoNet.  Handles all the protocol
 conversions that need to happen.
CabBusToLoconet.X - MPLABX project for PIC32.  Uses both CabBusCommunications
 and LocoNetCommunications to talk to both sides.
CabBusToLoconetComputer - A Linux/Unix program which uses both 
 CabBusCommunications and LocoNetCommunications to do the conversions.
Protocol - folder which contains protocol information for both LocoNet and the CabBus

## Notes:
* If you are using CabBusToLoconetComputer, make sure to set your latency timer to 1
for the USB to serial connection!  It won't work well otherwise:
`echo 1 > /sys/devices/pci0000:00/0000:00:14.0/usb1/1-3/1-3:1.0/ttyUSB0/latency_timer`

## License:
 
 GPL v2 ONLY
