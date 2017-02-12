/*
**  Oriic
**  Copyright (C) 2009-2011 Peter Gordon
**
**  This program is free software; you can redistribute it and/or
**  modify it under the terms of the GNU General Public License
**  as published by the Free Software Foundation, version 2
**  of the License.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
**  6551 ACIA emulation
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <malloc.h>
#include <math.h>

#include <gccore.h>
#include <wiiuse/wpad.h>

#include "6502.h"
#include "ula.h"
#include "8912.h"
#include "6551.h"
#include "via.h"
#include "disk.h"
#include "oriic.h"
#include "render.h"

void acia_init( struct acia *acia, struct machine *oric )
{
  acia->oric = oric;
  acia->regs[ACIA_RXDATA]  = 0;
  acia->regs[ACIA_TXDATA]  = 0;
  acia->regs[ACIA_STATUS]  = ASTF_TXEMPTY;
  acia->regs[ACIA_COMMAND] = ACOMF_IRQDIS;
  acia->regs[ACIA_CONTROL] = 0;
}

void acia_clock( struct acia *acia, unsigned int cycles )
{
}

void acia_write( struct acia *acia, u16 addr, u8 data )
{
  switch( addr & 3 )
  {
    case ACIA_RXDATA:      // Data for TX
      acia->regs[ACIA_TXDATA] = data;
      acia->regs[ACIA_STATUS] &= ~(ASTF_TXEMPTY);
      break;
    
    case ACIA_STATUS:      // Reset
      acia->regs[ACIA_COMMAND] &= ACOMF_PARITY;
      acia->regs[ACIA_COMMAND] |= ACOMF_IRQDIS;
      acia->regs[ACIA_STATUS]  &= ~(ASTF_OVRUNERR);
      break;
    
    case ACIA_COMMAND:
      acia->regs[ACIA_COMMAND] = data;
      break;
    
    case ACIA_CONTROL:
      acia->regs[ACIA_CONTROL] = data;
      break;
  }
}

u8 acia_read( struct acia *acia, u16 addr )
{
  return acia->regs[addr&3];
}
