CC = gcc

IDIR = /usr/src/cfscc/src/

#CFLAGS = -fPIC -std=gnu99 -O2 -Wall -g -I$(IDIR)
#Full debug with -g3
CFLAGS = -fPIC -std=gnu99 -Wall -g3 -I$(IDIR)
LDFLAGS = -pipe -Wall -lm -pthread -lcfscc -g3

EXEC = fscc_acq

SRC = fscc_acq.c fscc_helpers.c
OBJ = $(SRC:.c=.o)

HEADERS = defaults.h fscc_errors.h fscc_helpers.h fscc_struct.h $(EXEC).h

all: $(SRC) $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

.c.o: $(HEADERS)
	$(CC) -o $@ $(CFLAGS) -c $<

.PHONY: clean

clean:
	rm -f *.o $(EXEC)
