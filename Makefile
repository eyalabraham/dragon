#####################################################################################
#
#  This make file is for compiling the Dragon 32 emulator
#  on Raspberry Pi with Raspbian Linux OS.
#  Make will rsync files and run compilation on the RPi.
#
#  Use:
#    clean      - clean environment
#    all        - build all outputs
#
#####################################################################################


#------------------------------------------------------------------------------------
# project directories
#------------------------------------------------------------------------------------
INCDIR = include
SRCDIR = .
BINDIR = .

#------------------------------------------------------------------------------------
# build tool and options
#------------------------------------------------------------------------------------
CC = gcc
LST2H = awk -f $(INCDIR)/test/lst2h.awk
#OPT = -Wall -L/usr/local/lib -lbcm2835 -lxml2 -I $(INCDIR) -I/usr/include/libxml2
OPT = -Wall -L/usr/local/lib -lbcm2835 -I $(INCDIR)

#------------------------------------------------------------------------------------
# dependencies
#------------------------------------------------------------------------------------
DEPS = config.h mem.h cpu.h mc6809e.h
OBJSEMU09 = emu09.o mem.o cpu.o rpi.o
OBJSDRAGON = dragon.o mem.o cpu.o rpi.o

_DEPS = $(patsubst %,$(INCDIR)/%,$(DEPS))

#------------------------------------------------------------------------------------
# build all targets
#------------------------------------------------------------------------------------
%.o: %.c $(_DEPS)
	$(CC) -c -o $@ $< $(OPT)

all: dragon

emu09: $(OBJSEMU09)
	$(CC) $^ $(OPT) -o $@

dragon: $(OBJSDRAGON)
	$(CC) $^ $(OPT) -o $@

#------------------------------------------------------------------------------------
# rsync files and run remote 'make'
# requires ssh key setup to avoid using password authentication
#------------------------------------------------------------------------------------
sync:
	rsync -vrh $(SRCDIR)/*  pi@10.0.0.13:/home/pi/Documents/dragon
#	ssh pi@10.0.0.13 "cd /home/pi/Documents/dragon && make dragon"
	ssh pi@10.0.0.13 "cd /home/pi/Documents/dragon && make emu09"
	

rclean:
	ssh pi@10.0.0.13 "cd /home/pi/Documents/dragon && make clean"

#------------------------------------------------------------------------------------
# cleanup
#------------------------------------------------------------------------------------
.PHONY: clean

clean:
	rm -f dragon
	rm -f emu09
	rm -f *.o
	rm -f *.bak

