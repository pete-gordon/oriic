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

struct oric_ula
{
  struct machine *oric;

  int vid_start;           // Start drawing video
  int vid_end;             // Stop drawing video
  int vid_maxrast;         // Number of raster lines
  int vid_raster;          // Current rasterline
  int vid_special;         // After line 200, only process in text mode

  u8  vid_fg_col;
  u8  vid_bg_col;
  int vid_mode;
  int vid_is50hz;
  int vid_textattrs;
  int vid_blinkmask;
  int vid_chline;

  unsigned short vid_addr;
  unsigned char *vid_ch_data;
  unsigned char *vid_ch_base;

  u16 vidbases[4];
  u16 *vid_bitptr, *vid_inv_bitptr;

  int frames;
  int vsync;
  u32 *scrpt;
};

void ula_setfreq( struct oric_ula *u, int is50hz );
void ula_init( struct oric_ula *u, struct machine *oric );
void ula_ticktock( struct oric_ula *u, int cycles );
int ula_doraster( struct oric_ula *u );
void init_ula(void);
void ula_powerup_default( struct oric_ula *u );
