#
# Created by gmakemake Unknown (Jul 27 2014) on Thu Oct  2 16:14:13 2014
#

#
# Definitions
#

.SUFFIXES:
.SUFFIXES:	.a .o .c .C .cpp .s .S
.c.o:
		$(COMPILE.c) $< -o $@
.C.o:
		$(COMPILE.cc) $< -o $@
.cpp.o:
		$(COMPILE.cc) $< -o $@
.S.s:
		$(CPP) -o $*.s $<
.s.o:
		$(COMPILE.cc) $< -o $@
.c.a:
		$(COMPILE.c) -o $% $<
		$(AR) $(ARFLAGS) $@ $%
.C.a:
		$(COMPILE.cc) -o $% $<
		$(AR) $(ARFLAGS) $@ $%
.cpp.a:
		$(COMPILE.cc) -o $% $<
		$(AR) $(ARFLAGS) $@ $%

CC = 		gcc
CXX = 		g++

RM = rm -f
AR = ar
LINK.c = $(CC) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS)
LINK.cc = $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS)
COMPILE.c = $(CC) $(CFLAGS) $(CPPFLAGS) -c
COMPILE.cc = $(CXX) $(CXXFLAGS) $(CPPFLAGS) -c


########## Default flags (redefine these with a header.mak file if desired)
CPPFLAGS = 	-I../CabBusCommunications -I../LocoNetCommunications -I../common
CFLAGS = 	-ggdb
CXXFLAGS = 	-ggdb
CLIBFLAGS = 	-lm -lpopt -lpthread -lrt
CCLIBFLAGS = 	
########## End of default flags


CPP_FILES =	
C_FILES =	LoconetSniffer.c
H_FILES =	
SOURCEFILES =	$(H_FILES) $(CPP_FILES) $(C_FILES) $(S_FILES)
.PRECIOUS:	$(SOURCEFILES)
OBJFILES =	../LocoNetCommunications/loconet_buffer.o ../common/loconet_print.o

#
# Main targets
#

all:	LoconetSniffer 

LoconetSniffer:	LoconetSniffer.o $(OBJFILES)
	$(CC) $(CFLAGS) -o LoconetSniffer LoconetSniffer.o $(OBJFILES) $(CLIBFLAGS)

#
# Dependencies
#


#
# Housekeeping
#

Archive:	archive.tgz

archive.tgz:	$(SOURCEFILES) Makefile
	tar cf - $(SOURCEFILES) Makefile | gzip > archive.tgz

clean:
	$(RM) $(OBJFILES) LoconetSniffer.o core 2> /dev/null

realclean:        clean
	$(RM) -rf LoconetSniffer 
