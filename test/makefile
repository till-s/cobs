all: EOFTagTb COBSDecoderTb COBSEncoderTb

VPATH=../hdl

%.o: %.vhd
	ghdl -a $<

EOFTagTb.o: EOFTag.o

COBSEncoderTb.o: COBSDecoder.o COBSEncoder.o COBSFifo.o
COBSDecoderTb.o: COBSDecoder.o
COBSDecoder.o: EOFTag.o
COBSEncoder.o: COBSFifo.o

%Tb: %Tb.o
	ghdl -e -o $@ $@
