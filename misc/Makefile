SHELL = /bin/sh

IDIR = /usr/src/cfscc/src/

CFLAGS = -fPIC --std=gnu99 -Wall -Wextra -m64 -I$(IDIR)
LDFLAGS = -lcfscc

CC = gcc -g3

TRANSPARENT = fscc_acq_transparent

SYNCHR = fscc_acq_synchronous

TRACK_INTERRUPT_CHECK = fscc_interrupt_check

SLEEPER = fscc_acq_transparent_sleeper

DISABLE_RECEIVE = disable_port_receive

PROGS = $(SLEEPER) $(TRANSPARENT) $(SYNCHR) $(TRACK_INTERRUPT_CHECK) $(DISABLE_RECEIVE)

SRC = $(TRANSPARENT).c $(TRACK_INTERRUPT_CHECK).c $(SYNCHR).c $(SLEEPER).c $(DISABLE_RECEIVE).c
OBJ = $(SRC:.c=.o)

all: $(TRANSPARENT) $(TRACK_INTERRUPT_CHECK) $(SYNCHR) $(SLEEPER) $(DISABLE_RECEIVE)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(TRANSPARENT): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(@).o $(LDFLAGS)

$(SLEEPER): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(@).o $(LDFLAGS)

$(TRACK_INTERRUPT_CHECK): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(@).o $(LDFLAGS)

$(SYNCHR): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(@).o $(LDFLAGS)

$(DISABLE_RECEIVE): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(@).o $(LDFLAGS)

.PHONY: clean

clean:
	rm -f *.o $(PROGS)
