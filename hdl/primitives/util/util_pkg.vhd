-- This file is protected by Copyright. Please refer to the COPYRIGHT file
-- distributed with this source distribution.
--
-- This file is part of OpenCPI <http://www.opencpi.org>
--
-- OpenCPI is free software: you can redistribute it and/or modify it under the
-- terms of the GNU Lesser General Public License as published by the Free
-- Software Foundation, either version 3 of the License, or (at your option) any
-- later version.
--
-- OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
-- WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
-- A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
-- details.
--
-- You should have received a copy of the GNU Lesser General Public License
-- along with this program. If not, see <http://www.gnu.org/licenses/>.

-- package for letting VHDL access the verilogs in this library
library ieee; use ieee.std_logic_1164.all, ieee.numeric_std.all;
library ocpi; use ocpi.types.all;
package util is
component ROM
  generic (WIDTH    : natural;
           SIZE     : natural;
           INITFILE : string);
     port (CLK      : in std_logic;
           ADDR     : in  std_logic_vector(ocpi.util.width_for_max(size-1)-1 downto 0);
           DO       : out std_logic_vector(WIDTH-1 downto 0));
end component ROM;
component BRAM2
  generic (PIPELINED  : natural;
           ADDR_WIDTH : natural;
           DATA_WIDTH : natural;
           MEMSIZE    : natural);
    port  (CLKA       : in  std_logic;
           ENA        : in  std_logic;
           WEA        : in  std_logic;
           ADDRA      : in  std_logic_vector(ADDR_WIDTH-1 downto 0);
           DIA        : in  std_logic_vector(DATA_WIDTH-1 downto 0);
           DOA        : out std_logic_vector(DATA_WIDTH-1 downto 0);
           CLKB       : in  std_logic;
           ENB        : in  std_logic;
           WEB        : in  std_logic;
           ADDRB      : in  std_logic_vector(ADDR_WIDTH-1 downto 0);
           DIB        : in  std_logic_vector(DATA_WIDTH-1 downto 0);
           DOB        : out std_logic_vector(DATA_WIDTH-1 downto 0));
end component BRAM2;
-- 32 bit wide trace buffer.
component trace_buffer_ulong
  generic (depth : ulong_t);      -- expect parameter value
  port  (clk : in std_logic;
         reset : in bool_t;
         input : in std_logic_vector(ulong_t'range);  -- input values
         give  : in bool_t;       -- worker is giving new value this clock cycle
         rawin : in ocpi.wci.raw_in_t;  -- wire to raw interface
         rawout : out ocpi.wci.raw_out_t); -- wire to raw interface
end component trace_buffer_ulong;
    
--- CWD support during simulation
constant cwd_length : natural := 256;

component cwd is
   generic(length     : natural := cwd_length);
   port   (cwd        : out ocpi.types.string_t(0 to length));
end component cwd;

-- The basic verilog function for getting at the cwd= "plusarg".
component cwd_internal is
   generic(length : natural := cwd_length);
   port   (cwd    : out std_logic_vector(0 to length*8-1);
           len_err : out std_logic);
end component cwd_internal;

component TSINOUT_1 is
  port    (I  : in    std_logic;  -- OUTPUT to PIN when OE = 1
           OE : in    std_logic;                           -- output enable, 1 = enabled
           O  : out   std_logic;  -- INPUT from pin, all the time
           IO : inout std_logic); -- pin/pad
end component TSINOUT_1;

component TSINOUT_N is
  generic (width : natural);
  port    (I  : in    std_logic_vector(width-1 downto 0);  -- OUTPUT to PIN when OE = 1
           OE : in    std_logic;                           -- output enable, 1 = enabled
           O  : out   std_logic_vector(width-1 downto 0);  -- INPUT from pin, all the time
           IO : inout std_logic_vector(width-1 downto 0)); -- pin/pad
end component TSINOUT_N;

component SyncRegister is
  generic (
      width : positive := 1;
      init  : natural := 0);
    port (
      sCLK   : in std_logic;
      sRST   : in std_logic;
      dCLK   : in std_logic;
      sEN    : in std_logic;
      sRDY   : out std_logic;
      sD_IN  : in std_logic_vector(width-1 downto 0);      
      dD_OUT : out std_logic_vector(width-1 downto 0));
end component SyncRegister;

-- see Example 4/5 on page 31/32 of https://www.altera.com/content/dam/altera-www/global/en_US/pdfs/literature/an/an307.pdf
component buffer_clock_global is
  port (clk          : in  std_logic;
        clk_buffered : out std_logic);
end component buffer_clock_global;

-- A convenience function to join a CWD to a filename
function cwd_join(cwd : string_t; name : string_t) return string;

-- The basic verilog function for getting at the "plusarg".
constant plusarg_length : natural := 200;
component plusarg is
   generic(length     : natural := plusarg_length;
           name       : string);
   port   (val        : out ocpi.types.string_t(0 to length));
end component plusarg;
component plusarg_internal is
   generic(length : natural := plusarg_length;
           name : string);
   port   (val    : out std_logic_vector(0 to length*8-1));
end component plusarg_internal;

type char_file_t is file of character;
procedure open_file(file thefile : char_file_t;
                         cwd     : string_t;
                         name    : string_t;
                         mode    : file_open_kind);
procedure close_file(file thefile : char_file_t; name : string_t);

-- A module that must be written in verilog to write files with flushing
component flush_writer
  generic(nbytes       : natural;
          namelength   : natural);
  port   (clk          : in std_logic;
          reset        : in bool_t;
          name         : in string_t(0 to namelength);
          valid        : in bool_t;
          flush        : in bool_t;
          close        : in bool_t;
          data         : in std_logic_vector(nbytes*8-1 downto 0));
end component flush_writer;
component flush_writer_internal
  generic(nbytes       : natural;
          namelength   : natural);
  port   (clk          : in std_logic;
          reset        : in std_logic;
          name         : in std_logic_vector(0 to namelength*8-1);
          valid        : in std_logic;
          flush        : in std_logic;
          close        : in std_logic;
          data         : in std_logic_vector(nbytes*8-1 downto 0));
end component flush_writer_internal;

component spi
  generic(data_width    : positive := 8;
          addr_width    : positive := 7;
          clock_divisor : positive := 16;
          capture_fall  : boolean  := false);
  port   (clk           : in  std_logic;
          reset         : in  bool_t;
          renable       : in  bool_t;
          wenable       : in  bool_t;
          addr          : in  unsigned(addr_width-1 downto 0);
          wdata         : in  std_logic_vector(data_width-1 downto 0);
          rdata         : out std_logic_vector(data_width-1 downto 0);
          done          : out bool_t;
          -- The SPI signals - a 4 wire interface
          sdo           : in  std_logic;
          sclk          : out std_logic;
          sen           : out std_logic;
          sdio          : out std_logic);
end component spi;

component srl_fifo
  generic(width    : positive;
          depth    : positive := 16);
  port   (clk      : in  std_logic;
          reset    : in  bool_t;
          clear    : in  bool_t := bfalse;
          enq      : in  bool_t;
          deq      : in  bool_t;
          input    : in  std_logic_vector(width-1 downto 0);
          full     : out bool_t;
          empty    : out bool_t;
          output   : out std_logic_vector(width-1 downto 0));
end component srl_fifo;
end package util;
