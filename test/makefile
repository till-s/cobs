all: EOFTagTb COBSDecoderTb

%.o: %.vhd
	ghdl -a $<

EOFTagTb.o: EOFTag.o

COBSDecoderTb.o: COBSDecoder.o
COBSDecoder.o: EOFTag.o

%Tb: %Tb.o
	ghdl -e -o $@ $@
