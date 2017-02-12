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

static u32 oricpal[] = { 0x000000ff,
                         0xff0000ff,
                         0x00ff00ff,
                         0xffff00ff,
                         0x0000ffff,
                         0xff00ffff,
                         0x00ffffff,
                         0xffffffff };


// Update the ULA for a given video frequency (1 for 50hz, 0 for 60hz)
void ula_setfreq( struct oric_ula *u, int is50hz )
{
  // PAL50 = 50Hz = 1,000,000/50 = 20000 cpu cycles/frame
  // 312 scanlines/frame, so 20000/312 = ~64 cpu cycles / scanline

  // PAL60 = 60Hz = 1,000,000/60 = 16667 cpu cycles/frame
  // 312 scanlines/frame, so 16667/312 = ~53 cpu cycles / scanline

  // NTSC = 60Hz = 1,000,000/60 = 16667 cpu cycles/frame
  // 262 scanlines/frame, so 16667/262 = ~64 cpu cycles / scanline
  if( is50hz )
  {
    // PAL50
    u->oric->cyclesperraster = 64;
    u->vid_start       = 65;
    u->vid_maxrast     = 312;
  } else {
    // NTSC
    u->oric->cyclesperraster = 64;
    u->vid_start       = 32;
    u->vid_maxrast     = 262;
  }

  u->vid_special     = u->vid_start + 200;
  u->vid_end         = u->vid_start + 224;
}

// Initialise the ULA
void ula_init( struct oric_ula *u, struct machine *oric )
{
  int i;

  // Keep a pointer to the main structure
  u->oric = oric;

  // Default to 50Hz
  ula_setfreq( u, 1 );

  u->vidbases[0] = 0xa000;
  u->vidbases[1] = 0x9800;
  u->vidbases[2] = 0xbb80;
  u->vidbases[3] = 0xb400;

  u->vid_raster = 0;

  switch( oric->type )
  {
    case MACH_ORIC1_16K:
      for( i=0; i<4; i++ )
        u->vidbases[i] &= 0x7fff;
      break;
  }

  u->vid_addr = u->vidbases[2];
  u->vid_ch_base = &u->oric->mem[u->vidbases[3]];
}

// Refresh the video base pointer
static inline void ula_refresh_charset( struct oric_ula *u )
{
  if( u->vid_textattrs & 1 )
  {
    u->vid_ch_data = &u->vid_ch_base[128*8];
  } else {
    u->vid_ch_data = u->vid_ch_base;
  }
}

// Decode an oric video attribute
static void ula_decode_attr( struct oric_ula *u, int attr, int y )
{
  switch( attr & 0x18 )
  {
    case 0x00: // Foreground colour
      u->vid_fg_col = attr & 0x07;
      break;

    case 0x08: // Text attribute
      u->vid_textattrs = attr & 0x07;
      u->vid_blinkmask = attr & 0x04 ? 0x00 : 0x3f;
      ula_refresh_charset( u );
      if( u->vid_textattrs & 0x02 )
        u->vid_chline = (y>>1) & 0x07;
      else
        u->vid_chline = y & 0x07;
      break;

    case 0x10: // Background colour
      u->vid_bg_col = attr & 0x07;
      break;

    case 0x18: // Video mode
      u->vid_mode = attr & 0x07;
      
      // Set up pointers for new mode
      if( u->vid_mode & 4 )
      {
        u->vid_addr = u->vidbases[0];
        u->vid_ch_base = &u->oric->mem[u->vidbases[1]];
      } else {
        u->vid_addr = u->vidbases[2];
        u->vid_ch_base = &u->oric->mem[u->vidbases[3]];
      }

      ula_refresh_charset( u );
      break;
  }   
}

static inline void ula_raster_default( struct oric_ula *u )
{
  u->vid_fg_col     = 7;
  u->vid_textattrs  = 0;
  u->vid_blinkmask  = 0x3f;
  u->vid_bg_col     = 0;
  ula_refresh_charset( u );
}

void ula_powerup_default( struct oric_ula *u )
{
  ula_decode_attr( u, 0x1a, 0 );
}
// Render a 6x1 block
static void ula_render_block( struct oric_ula *u, u8 fg, u8 bg, u8 inverted, u8 data )
{
  int i;

  if( inverted )
  {
    fg ^= 0x07;
    bg ^= 0x07;
  }

  for( i=0; i<6; i++ )
  {
//    *(u->scrpt++) = oricpal[(u->frames/25)&7];
    *(u->scrpt++) = oricpal[(data & 0x20) ? fg : bg];
    data <<= 1;
  }
}

// Draw one rasterline
int ula_doraster( struct oric_ula *u )
{
  int b, c, bitmask;
  int hires, needrender;
  u32 y, cy;
  u8 *rptr;
 
  needrender = 0;

  u->vid_raster++;
  if( u->vid_raster == u->vid_maxrast )
  {
    u->vid_raster = 0;
    u->vsync      = u->oric->cyclesperraster / 2;
    needrender    = 1;
    u->frames++;

    if( u->vid_is50hz != ((u->vid_mode&2)!=0) )
    {
      u->vid_is50hz = ((u->vid_mode&2)!=0);
      ula_setfreq( u, u->vid_is50hz );
    }
  }

  // Are we on a visible rasterline?
  if( ( u->vid_raster < u->vid_start ) ||
      ( u->vid_raster >= u->vid_end ) ) return needrender;

  y = u->vid_raster - u->vid_start;

  u->scrpt = &u->oric->odisp.rgb[y*240];
  
  cy = (y>>3) * 40;

  // Always start each scanline with white on black
  ula_raster_default( u );
  u->vid_chline = y & 0x07;

  if( y < 200 )
  {
    if( u->vid_mode & 0x04 ) // HIRES?
    {
      hires = 1;
      rptr = &u->oric->mem[u->vid_addr + y*40 -1];
    } else {
      hires = 0;
      rptr = &u->oric->mem[u->vid_addr + cy -1];
    }
  } else {
    hires = 0;
    rptr = &u->oric->mem[u->vidbases[2] + cy -1];  // bb80 = bf68 - (200/8*40)
  }
  bitmask = (u->frames&0x10)?0x3f:u->vid_blinkmask;

  for( b=0; b<40; b++ )
  {
    c = *(++rptr);

    /* if bits 6 and 5 are zero, the byte contains a serial attribute */
    if( ( c & 0x60 ) == 0 )
    {
      ula_decode_attr( u, c, y );
      ula_render_block( u, u->vid_fg_col, u->vid_bg_col, (c & 0x80)!=0, 0 );
      if( y < 200 )
      {
        if( u->vid_mode & 0x04 ) // HIRES?
        {
          hires = 1;
          rptr = &u->oric->mem[u->vid_addr + b + y*40];
        } else {
          hires = 0;
          rptr = &u->oric->mem[u->vid_addr + b + cy];
        }
      } else {
        if (hires)
        {
          hires = 0;
          rptr = &u->oric->mem[u->vidbases[2] + b + cy];   // bb80 = bf68 - (200/8*40)
        }
      }
      bitmask = (u->frames&0x10)?0x3f:u->vid_blinkmask;
    
    } else {
      if( hires )
      {
        ula_render_block( u, u->vid_fg_col, u->vid_bg_col, (c & 0x80)!=0, c & bitmask );
      } else {
        int ch_ix, ch_dat;
          
        ch_ix   = c & 0x7f;
        ch_dat = u->vid_ch_data[ (ch_ix<<3) | u->vid_chline ] & bitmask;
        
        ula_render_block( u, u->vid_fg_col, u->vid_bg_col, (c & 0x80)!=0, ch_dat  );
      }
    }
  }

  return needrender;
}

void ula_ticktock( struct oric_ula *u, int cycles )
{
  int j;

  if( u->vsync > 0 )
  {
    u->vsync -= cycles;
    if( u->vsync < 0 )
      u->vsync = 0;
  }
  
  if( u->oric->vsynchack )
  {
    j = (u->vsync == 0);
    if( u->oric->via.cb1 != j )
      via_write_CB1( &u->oric->via, j );
  }
}
