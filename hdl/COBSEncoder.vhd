library ieee;
use     ieee.std_logic_1164.all;
use     ieee.numeric_std.all;

entity COBSEncoder is
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

   constant LD_FIFO_DEPTH_C : natural := 8;

   type RegType is record
      ocnt         : unsigned(7 downto 0);
      icnt         : unsigned(7 downto 0);
      newCnt       : unsigned(7 downto 0);
      vldNewCnt    : std_logic;
   end record RegType;

   constant REG_INIT_C      : RegType := (
      ocnt         => (others => '0'),
      icnt         => (others => '0'),
      newCnt       => (others => '0'),
      vldNewCnt    => '0'
   );

   signal  r                : RegType := REG_INIT_C;
   signal  rin              : RegType;
   signal  vldOutLoc        : std_logic;
   signal  datOutLoc        : std_logic_vector(7 downto 0);
   signal  rdyOutLoc        : std_logic;
   signal  lstOutLoc        : std_logic;

   signal  stitchInp        : std_logic_vector(8 downto 0);
   signal  stitchOut        : std_logic_vector(8 downto 0);

   signal  fifoVldInp       : std_logic;
   signal  fifoRdyInp       : std_logic;
   signal  fifoDatInp       : std_logic_vector(7 downto 0);
   signal  fifoVldOut       : std_logic;
   signal  fifoRdyOut       : std_logic;
   signal  fifoDatOut       : std_logic_vector(7 downto 0);


   constant EOF_C           : std_logic_vector(7 downto 0) := x"00";
   constant CHAIN_C         : std_logic_vector(7 downto 0) := x"FF";

   shared variable memory   : Slv8Array(0 to 2**LD_FIFO_DEPTH_C - 1);
   signal memoryRen         : std_logic;
   signal memoryWen         : std_logic;

   signal wptr              : signed(LD_FIFO_DEPTH_C downto 0) := (others => '0');
   signal rptr              : signed(LD_FIFO_DEPTH_C downto 0) := (others => '0');


begin

   P_MEM_RD : process ( clk ) is
   begin
      if ( rising_edge( clk ) ) then
	 if ( memoryRen = '1' ) then
            fifoDatOut <= memory( to_integer(unsigned(rptr(LD_FIFO_DEPTH_C - 1 downto 0))) );
	 end if;
      end if;
   end process P_MEM_RD;

   P_MEM_WR : process ( clk ) is
   begin
      if ( rising_edge( clk ) ) then
	 if ( memoryWen = '1' ) then
            memory( to_integer(unsigned(wptr(LD_FIFO_DEPTH_C - 1 downto 0))) ) := fifoDatInp;
	 end if;
      end if;
   end process P_MEM_WR;

   memoryRen <= not fifoVldOut or fifoRdyOut;
   memoryWen <= fifoVldInp and fifoRdyInp;

   fifoRdyInp <= '0' when (wptr - rptr) < 0 else '1'; -- full

   P_FIFO_RD : process ( clk ) is
   begin
      if ( rising_edge( clk ) ) then
         if ( rst = '1' ) then
            rptr       <= (others => '0');
            fifoVldOut <= '0';
         else
            if ( memoryRen = '1' ) then
               if ( wptr /= rptr ) then
                  fifoVldOut <= '1';
                  rptr       <= rptr + 1;
               else
                  fifoVldOut <= '0';
               end if;
            end if;
         end if;
      end if;
   end process P_FIFO_RD;

   lstoutLoc  <= '1' when datInp = EOF_C else '0';

   P_COMB : process ( r ) is
      variable v : RegType;
   begin
      v         := r;

      fifoRdyOut <= '0';
      vldOut     <= '0';
      datOut     <= fifoDatOut;

      if ( r.ocnt /= 0 ) then
         fifoRdyOut <= rdyOut;
         vldOut     <= fifoVldOut;
         if ( (rdyOut and fifoVldOut) = '1' ) then
            v.ocnt := r.ocnt - 1;
         end if;
      else
         vldOut     <= r.vldNewCnt;
         datOut     <= std_logic_vector( r.newCnt );
         if ( (r.vldNewCnt and rdyOut) = '1' ) then
            v.ocnt       := r.newCnt;
            v.vldNewCnt := '0';
         end if;
      end if;

      fifoVldInp <= vldInp;
      fifoDatInp <= datInp;
      rdyInp     <= fifoRdyInp;

      if ( vldInp = '1' ) then
         if ( datInp = EOF_C or r.icnt = unsigned(CHAIN_C)  ) then
            fifoVldInp <= '0';
   	    rdyInp     <= not v.vldNewCnt;
	    if ( v.vldNewCnt = '0' ) then
               v.newCnt    := r.icnt;
	       v.icnt      := to_unsigned(1, v.icnt'length);
               v.vldNewCnt := '1';
            end if;
         elsif ( fifoRdyInp = '1' ) then
            v.icnt := r.icnt + 1;
         end if;
      end if;
      rin        <= v;
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

end architecture rtl;
