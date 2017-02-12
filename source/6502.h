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
**  6502 emulation header
*/

// 6502 flag bits
#define FB_C 0
#define FF_C (1<<FB_C)
#define FB_Z 1
#define FF_Z (1<<FB_Z)
#define FB_I 2
#define FF_I (1<<FB_I)
#define FB_D 3
#define FF_D (1<<FB_D)
#define FB_B 4
#define FF_B (1<<FB_B)
#define FB_R 5
#define FF_R (1<<FB_R)
#define FB_V 6
#define FF_V (1<<FB_V)
#define FB_N 7
#define FF_N (1<<FB_N)

// IRQ sources
#define IRQB_VIA 0
#define IRQF_VIA (1<<IRQB_VIA)
#define IRQB_DISK 1
#define IRQF_DISK (1<<IRQB_DISK)
#define IRQB_VIA2 2
#define IRQF_VIA2 (1<<IRQB_VIA2)

// Merge the seperate flag stores into a 6502 status register form
#define MAKEFLAGS ((cpu->f_n<<7)|(cpu->f_v<<6)|(1<<5)|(cpu->f_b<<4)|(cpu->f_d<<3)|(cpu->f_i<<2)|(cpu->f_z<<1)|cpu->f_c)
#define MAKEFLAGSBC ((cpu->f_n<<7)|(cpu->f_v<<6)|(1<<5)|(cpu->f_d<<3)|(cpu->f_i<<2)|(cpu->f_z<<1)|cpu->f_c)

// Set the seperate flag stores from a 6502-format mask
#define SETFLAGS(n) cpu->f_n=(n&0x80)>>7;\
                    cpu->f_v=(n&0x40)>>6;\
                    cpu->f_b=(n&0x10)>>4;\
                    cpu->f_d=(n&0x08)>>3;\
                    cpu->f_i=(n&0x04)>>2;\
                    cpu->f_z=(n&0x02)>>1;\
                    cpu->f_c=n&0x01

struct m6502
{
  int   rastercycles;
  int   icycles;
  u32   cycles;
  u8    a, x, y, sp;
  u8    f_c, f_z, f_i, f_d, f_b, f_v, f_n;
  u16   pc, lastpc, calcpc, calcint, baddr;
  int   nmi;
  u8    irq, nmicount;
  void (*write)(struct m6502 *,u16,u8);
  unsigned char (*read)(struct m6502 *,u16);
  u8    calcop;
  void    *userdata;
};

void m6502_init( struct m6502 *cpu, void *userdata );
void m6502_reset( struct m6502 *cpu );
void m6502_inst( struct m6502 *cpu );
void m6502_set_icycles( struct m6502 *cpu );
