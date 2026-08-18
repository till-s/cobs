library ieee;
use     ieee.std_logic_1164.all;

-- convert EOF markers to 'lst' flag on the last payload word.
-- NOTE: empty frames (EOF, EOF) are dropped.

entity EOFTag is
   generic (
      EOF_G        : std_logic_vector := x"00";
      -- must be >= EOF_G'length; the EOF_G'length
      -- bits of data are compared against EOF_G.
      DATA_WIDTH_G : natural          := 8
   );
   port (
      clk          : in  std_logic;
      rst          : in  std_logic;

      datInp       : in  std_logic_vector(DATA_WIDTH_G - 1 downto 0);
      vldInp       : in  std_logic;
      rdyInp       : out std_logic;

      datOut       : out std_logic_vector(DATA_WIDTH_G - 1 downto 0);
      vldOut       : out std_logic;
      lstOut       : out std_logic;
      rdyOut       : in  std_logic
   );
end entity EOFTag;

architecture rtl of EOFTag is
   signal buf       : std_logic_vector(datInp'range);
   signal ful       : std_logic := '0';
   signal vldOutLoc : std_logic;
   signal rdyInpLoc : std_logic;
   signal lstOutLoc : std_logic;
begin

   vldOutLoc <= (ful and vldInp);
   rdyInpLoc <= (not ful or rdyOut);

   -- output can only be consumed when 'ful' and 'vldInp'; the
   -- 'lst' flag 'falls through' in that case.
   lstOutLoc <= '1' when datInp(EOF_G'reverse_range) = EOF_G else '0';

   P_SEQ : process ( clk ) is
   begin
      if ( rising_edge( clk ) ) then
         if ( rst = '1' ) then
            ful <= '0';
         else
            if (    ( (vldOutLoc and rdyOut) = '1' )
                 -- implies rdyInp & vldInp; input ready and can be consumed
                 or ( (not ful and vldInp)   = '1' ) ) then
               buf <= datInp;
               ful <= not lstOutLoc;
            end if;
         end if;
      end if;
   end process P_SEQ;

   datOut <= buf;
   rdyInp <= rdyInpLoc;
   lstOut <= lstOutLoc;
   vldOut <= vldOutLoc;
end architecture rtl;
