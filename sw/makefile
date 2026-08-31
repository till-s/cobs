
PROGS=test rtest

CFLAGS+=-I. -g

all: $(PROGS)

test.o cobsC.o rtest.o: cobsC.h

%.o: %.cc
	$(CXX) -c $(CFLAGS) $<

%.o: %.c
	$(CC) -c $(CFLAGS) $<

$(PROGS):%:%.o cobsC.o
	$(CXX) -o $@ $(CFLAGS) $^

clean:
	$(RM) *.o $(PROGS)
