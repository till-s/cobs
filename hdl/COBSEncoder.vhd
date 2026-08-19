library ieee;
use     ieee.std_logic_1164.all;
use     ieee.numeric_std.all;

entity COBSEncoder is
   generic (
      LD_DAT_FIFO_DEPTH_G : positive range 8 to 1000 := 8;
      -- increasing
      LD_HDR_FIFO_DEPTH_G : positive                 := 2
   );
   port (
      clk          : in  std_logic;
      rst          : in  std_logic;

      datInp       : in  std_logic_vector(7 downto 0);
      vldInp       : in  std_logic;
      lstInp       : in  std_logic;
      rdyInp       : out std_logic;

      datOut       : out std_logic_vector(7 downto 0);
      vldOut       : out std_logic;
      rdyOut       : in  std_logic
   );
end entity COBSEncoder;

architecture rtl of COBSEncoder is

   subtype Slv8Type  is std_logic_vector(7 downto 0);
   type    Slv8Array is array (natural range <>) of Slv8Type;

   type HdrType is record
      cnt          : unsigned(7 downto 0);
   end record HdrType;

   constant HDR_INIT_C : HdrType := (
      cnt          => (others => '0')
   );

   subtype  HdrFifoIdxType      is signed(LD_HDR_FIFO_DEPTH_G downto 0);
   type     HdrArrayType        is array(0 to 2**LD_HDR_FIFO_DEPTH_G -1) of HdrType;

   type RegType is record
      ocnt         : unsigned(7 downto 0);
      icnt         : unsigned(7 downto 0);
      hdrs         : HdrArrayType;
      hwptr        : HdrFifoIdxType;
      hrptr        : HdrFifoIdxType;
      eof          : std_logic;
   end record RegType;

   constant REG_INIT_C      : RegType := (
      ocnt         => (others => '0'),
      icnt         => to_unsigned(1, 8),
      hdrs         => (others => HDR_INIT_C),
      hwptr        => (others => '0'),
      hrptr        => (others => '0'),
      eof          => '0'
   );

   signal  r                : RegType := REG_INIT_C;
   signal  rin              : RegType;
   signal  rdyInpLoc        : std_logic;

   signal  stitchInp        : std_logic_vector(8 downto 0);
   signal  stitchOut        : std_logic_vector(8 downto 0);

   signal  fifoVldInp       : std_logic;
   signal  fifoRdyInp       : std_logic;
   signal  fifoDatInp       : std_logic_vector(7 downto 0);
   signal  fifoVldOut       : std_logic := '0';
   signal  fifoRdyOut       : std_logic;
   signal  fifoDatOut       : std_logic_vector(7 downto 0);
   signal  fifoFull         : std_logic;
   signal  fifoEmpty        : std_logic;

   signal  hdrFifoDbg       : unsigned(7 downto 0);
   signal  hdrFifoFul       : std_logic;

   constant EOF_C           : std_logic_vector(7 downto 0) := x"00";
   constant EOF_CNT_C       : unsigned        (7 downto 0) := x"00";
   constant CHAIN_C         : unsigned        (7 downto 0) := x"FF";

   function to_std_logic(constant x : boolean) return std_logic is
   begin
      if ( x ) then return '1'; else return '0'; end if;
   end function to_std_logic;

   function ite(constant x : boolean; constant t,f : std_logic) return std_logic is
   begin
      if ( x ) then return t; else return f; end if;
   end function ite;

   function hdrFifoEmpty(constant x : in RegType) return std_logic is
   begin
      return to_std_logic( x.hwptr = x.hrptr );
   end function hdrFifoEmpty;

   function hdrFifoFull(constant x : in RegType) return std_logic is
   begin
      return to_std_logic( x.hwptr - x.hrptr < 0 );
   end function hdrFifoFull;

   function hdrFifoSpace(constant x : in RegType; constant min : in natural) return std_logic is
      constant CAPACITY_C : unsigned(x.hwptr'range) := to_unsigned(2**LD_HDR_FIFO_DEPTH_G, x.hwptr'length);
   begin
      return to_std_logic( CAPACITY_C - (unsigned(x.hwptr) - unsigned(x.hrptr)) >= min );
   end function hdrFifoSpace;

   function hdrFifoHead(constant x : in RegType) return HdrType is
   begin
      return x.hdrs(to_integer(unsigned(x.hrptr(LD_HDR_FIFO_DEPTH_G - 1 downto 0))));
   end function hdrFifoHead;

   procedure hdrFifoPop(variable x : inout RegType) is
   begin
      x       := x;
      x.hrptr := x.hrptr + 1;
   end procedure hdrFifoPop;

   procedure hdrFifoPush(variable x : inout RegType; constant v : in unsigned(7 downto 0)) is
      variable nv : HdrType := HDR_INIT_C;
   begin
      x       := x;
      nv.cnt  := v;
      x.hdrs(to_integer(unsigned(x.hwptr(LD_HDR_FIFO_DEPTH_G - 1 downto 0)))) := nv;
      x.hwptr := x.hwptr + 1;
   end procedure hdrFifoPush;


begin

   P_COMB : process ( r, fifoDatOut, fifoVldOut, fifoVldInp, fifoRdyOut, rdyOut, vldInp, lstInp, datInp, fifoRdyInp, rdyInpLoc ) is
      variable v                : RegType;
      variable hdrSpaceRequired : natural;
   begin
      v         := r;

      fifoRdyOut        <= '0';
      vldOut            <= '0';
      datOut            <= fifoDatOut;
      hdrSpaceRequired  := 0;

      hdrFifoDbg        <= hdrFifoHead(r).cnt;
      hdrFifoFul        <= hdrFifoFull(r);

      -- back-end processing: get run-length out of the mailbox; append data
      if ( r.ocnt = 0 ) then
         -- icnt/ocnt == 0 serve as frame markers

         -- if mailbox data are valid, append ther run-length (or EOF marker
         -- if newCnt = 0
         vldOut     <= not hdrFifoEmpty(r);
         datOut     <= std_logic_vector( hdrFifoHead(r).cnt );
         if ( (not hdrFifoEmpty(r) and rdyOut) = '1' ) then
            if ( hdrFifoHead(r).cnt /= 0 ) then
               v.ocnt       := hdrFifoHead(r).cnt - 1;
            -- else the count can be left alone; is already 0
            end if;
	    hdrFifoPop(v);
         end if;
      else
         fifoRdyOut <= rdyOut;
         vldOut     <= fifoVldOut;
         if ( (rdyOut and fifoVldOut) = '1' ) then
            v.ocnt := r.ocnt - 1;
         end if;
      end if;

      fifoVldInp <= vldInp and not r.eof;
      fifoDatInp <= datInp;
      rdyInpLoc  <= fifoRdyInp and not r.eof;

      if ( r.eof = '1' ) then
         if ( hdrFifoSpace(v, 2) = '1' ) then
	    hdrFifoPush( v, r.icnt );
            v.icnt := to_unsigned(1, v.icnt'length);
	    hdrFifoPush( v, to_unsigned(0, v.icnt'length) );
            v.eof := '0';
         end if;
      elsif ( (vldInp  = '1') ) then
         if ( (datInp = EOF_C) or (r.icnt = CHAIN_C) ) then
            -- special processing or 0x00 and max. block length;
            -- stop writing the fifo
            fifoVldInp <= '0';
            -- consume the input if this is an EOF but hold-off
            -- if it is a max. len block

            rdyInpLoc <= to_std_logic( r.icnt /= CHAIN_C );

            if ( hdrFifoFull(v) = '0' ) then
	       hdrFifoPush( v, r.icnt );
               v.icnt    := to_unsigned(1, v.icnt'length);
            else
               -- no space in header fifo; dont' consume EOF and wait
               rdyInpLoc <= '0';
            end if;
            -- trigger EOF processing if data were consumed and lstInp
            v.eof := rdyInpLoc and lstInp;
         elsif ( (fifoVldInp and fifoRdyInp) = '1' ) then 
            v.icnt := r.icnt + 1;
            v.eof  := lstInp;
         end if;
      end if;
      rin        <= v;
      rdyInp     <= rdyInpLoc;
   end process P_COMB;

   P_SEQ : process ( clk ) is
   begin
      if ( rising_edge( clk ) ) then
         if ( rst = '1' ) then
            r <= REG_INIT_C;
         else
            r <= rin;
         end if;
      end if;
   end process P_SEQ;

   U_FIFO : entity work.COBSFifo
      generic map (
         LD_FIFO_DEPTH_G => LD_DAT_FIFO_DEPTH_G,
         DATA_WIDTH_G    => 8
      )
      port map (
         clk             => clk,
         rst             => rst,

         wrEna           => fifoVldInp,
         datInp          => fifoDatInp,
         full            => fifoFull,

         rdEna           => fifoRdyOut,
         datOut          => fifoDatOut,
         empty           => fifoEmpty
      );

   fifoVldOut <= not fifoEmpty;
   fifoRdyInp <= not fifoFull;

end architecture rtl;
