
PROGS=test

CFLAGS+=-I. -g

all: $(PROGS)

test.o: cobsC.o

%.o: %.c
	$(CC) -c $(CFLAGS) $<

$(PROGS):%:%.o cobsC.o
	$(CC) -o $@ $(CFLAGS) $^

clean:
	$(RM) *.o $(PROGS)
