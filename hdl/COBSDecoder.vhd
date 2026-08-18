library ieee;
use     ieee.std_logic_1164.all;
use     ieee.numeric_std.all;

entity COBSDecoder is
   port (
      clk          : in  std_logic;
      rst          : in  std_logic;

      datInp       : in  std_logic_vector(7 downto 0);
      vldInp       : in  std_logic;
      rdyInp       : out std_logic;

      datOut       : out std_logic_vector(7 downto 0);
      vldOut       : out std_logic;
      lstOut       : out std_logic;
      rdyOut       : in  std_logic
   );
end entity COBSDecoder;

architecture rtl of COBSDecoder is
   type RegType is record
      cnt          : unsigned(7 downto 0);
      sendZero     : std_logic;
   end record RegType;

   constant REG_INIT_C      : RegType := (
      cnt          => (others => '0'),
      sendZero     => '0'
   );

   signal  r                : RegType := REG_INIT_C;
   signal  rin              : RegType;
   signal  vldOutLoc        : std_logic;
   signal  datOutLoc        : std_logic_vector(7 downto 0);
   signal  rdyOutLoc        : std_logic;
   signal  lstOutLoc        : std_logic;

   signal  stitchInp        : std_logic_vector(8 downto 0);
   signal  stitchOut        : std_logic_vector(8 downto 0);

   constant EOF_C           : std_logic_vector(7 downto 0) := x"00";
   constant CHAIN_C         : std_logic_vector(7 downto 0) := x"FF";

begin

   lstoutLoc <= '1' when datInp = EOF_C else '0';

   P_COMB : process ( r, datInp, vldInp, rdyOutLoc, lstOutLoc ) is
      variable v : RegType;
   begin
      v         := r;
      vldOutLoc <= vldInp;
      datOutLoc <= datInp;
      rdyInp    <= rdyOutLoc;
      if ( (vldInp and rdyOutLoc) = '1' ) then
         if ( lstOutLoc = '1' ) then
            v.cnt      := (others => '0');
            v.sendZero := '0';
         elsif ( r.cnt = 0 ) then
            vldOutLoc  <= r.sendZero;
            if ( datInp = CHAIN_C ) then
               v.sendZero := '0';
            else
               v.sendZero := '1';
            end if;
            v.cnt      := unsigned(datInp) - 1; -- datInp > 0 since lstOutLoc = '0'
            datOutLoc  <= (others => '0');
         else
            v.cnt      := r.cnt - 1;
         end if;
      end if;
      rin       <= v;
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

   stitchInp <= datOutLoc & lstoutLoc;

   U_EOF : entity work.EOFTag
      generic map (
         EOF_G        => "1",
         DATA_WIDTH_G => 9
      )
      port map (
         clk          => clk,
         rst          => rst,
         vldInp       => vldOutLoc,
         rdyInp       => rdyOutLoc,
         datInp       => stitchInp,
         vldOut       => vldOut,
         lstOut       => lstOut,
         rdyOut       => rdyOut,
         datOut       => stitchOut
      );

   datOut    <= stitchOut(8 downto 1);

end architecture rtl;
