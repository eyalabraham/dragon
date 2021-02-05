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

#OPT = -Wall -L/usr/local/lib -lbcm2835 -I $(INCDIR)
OPT = -Wall -I $(INCDIR)

#------------------------------------------------------------------------------------
# dependencies
#------------------------------------------------------------------------------------
DEPS = config.h mem.h cpu.h mc6809e.h trace.h uart.h
OBJEMU09 = emu09.o mem.o cpu.o rpi.o
OBJMON09 = mon09.o mem.o cpu.o rpi.o uart.o
OBJBAS09 = basic09.o mem.o cpu.o trace.o rpi.o uart.o
OBJINT09 = intr09.o mem.o cpu.o trace.o rpi.o uart.o
OBJPROF = profile.o mem.o cpu.o rpi.o
OBJDRAGON = dragon.o mem.o cpu.o rpi.o

_DEPS = $(patsubst %,$(INCDIR)/%,$(DEPS))

#------------------------------------------------------------------------------------
# build all targets
#------------------------------------------------------------------------------------
%.o: %.c $(_DEPS)
	$(CC) -c -o $@ $< $(OPT)

all: dragon

emu09: $(OBJEMU09)
	$(CC) $^ $(OPT) -o $@

mon09: $(OBJMON09)
	$(CC) $^ $(OPT) -o $@

basic09: $(OBJBAS09)
	$(CC) $^ $(OPT) -o $@

intr09: $(OBJINT09)
	$(CC) $^ $(OPT) -o $@

profile: $(OBJPROF)
	$(CC) $^ -L/usr/local/lib -lbcm2835 $(OPT) -o $@

dragon: $(OBJDRAGON)
	$(CC) $^ $(OPT) -o $@

#------------------------------------------------------------------------------------
# rsync files and run remote 'make'
# requires ssh key setup to avoid using password authentication
#------------------------------------------------------------------------------------
sync:
	rsync -vrh $(SRCDIR)/*  pi@10.0.0.13:/home/pi/Documents/dragon
#	ssh pi@10.0.0.13 "cd /home/pi/Documents/dragon && make dragon"
	ssh pi@10.0.0.13 "cd /home/pi/Documents/dragon && make emu09"
	ssh pi@10.0.0.13 "cd /home/pi/Documents/dragon && make mon09"
	ssh pi@10.0.0.13 "cd /home/pi/Documents/dragon && make basic09"
	ssh pi@10.0.0.13 "cd /home/pi/Documents/dragon && make intr09"
	ssh pi@10.0.0.13 "cd /home/pi/Documents/dragon && make profile"

rclean:
	ssh pi@10.0.0.13 "cd /home/pi/Documents/dragon && make clean"

#------------------------------------------------------------------------------------
# cleanup
#------------------------------------------------------------------------------------
.PHONY: clean

clean:
	rm -f dragon
	rm -f emu09
	rm -f mon09
	rm -f basic09
	rm -f intr09
	rm -f profile
	rm -f *.o
	rm -f *.bak

