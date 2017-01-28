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

 License:
 
 GPL v2 ONLY