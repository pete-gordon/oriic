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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <wiikeyboard/keyboard.h>

#include "6502.h"
#include "8912.h"
#include "6551.h"
#include "via.h"
#include "ula.h"
#include "disk.h"
#include "oriic.h"
#include "keyboard.h"
#include "render.h"
#include "textures.h"

static unsigned short keytab[] = { '7'        , 'n'        , '5'        , 'v'        , 61700 , '1'        , 'x'        , '3'        ,
                                   'j'        , 't'        , 'r'        , 'f'        , 0     , 27         , 'q'        , 'd'        ,
                                   'm'        , '6'        , 'b'        , '4'        , 61699 , 'z'        , '2'        , 'c'        ,
                                   'k'        , '9'        , ';'        , '-'        , '#'   , 0          , '\\'       , '\''       ,
                                   ' '        , ','        , '.'        , 62340      , 61697 , 62342      , 62341      , 62343      ,
                                   'u'        , 'i'        , 'o'        , 'p'        , 61703 , 8          , ']'        , '['        ,
                                   'y'        , 'h'        , 'g'        , 'e'        , 61704 , 'a'        , 's'        , 'w'        ,
                                   '8'        , 'l'        , '0'        , '/'        , 61698 , 13         , 0          , '=' };
static unsigned short osdkeytab[] = {  6, 46,  4, 44, -1,  0, 42,  2,
                                      34, 18, 17, 31, -1, 13, 14, 30,
                                      47,  5, 45,  3, 27, 41,  1, 43,
                                      35,  8, 37, 10, -1, 59, 12, 38,
                                      54, 48, 49, 55, 40, 52, 53, 56,
                                      20, 21, 22, 23, -1, 26, 25, 24,
                                      19, 33, 32, 16, 57, 28, 29, 15,
                                       7, 36,  9, 50, 51, 39, 58, 11 };

static struct kbdkey kbd_atmos[65], kbd_oric1[65], kbd_pravetz[65];

int kbd_wiimote_images[] = { gfx_wiimote_2, 24, 24,    // 0x00000001
                             gfx_wiimote_1, 24, 24,    // 0x00000002
                             gfx_wiimote_B, 24, 32,    // 0x00000004
                             gfx_wiimote_A, 28, 28,    // 0x00000008
                             -1, -1, -1,               // 0x00000010
                             -1, -1, -1,               // 0x00000020
                             -1, -1, -1,               // 0x00000040
                             -1, -1, -1,               // 0x00000080
                             gfx_dpad_left,  32, 32,   // 0x00000100
                             gfx_dpad_right, 32, 32,   // 0x00000200
                             gfx_dpad_down,  32, 32,   // 0x00000400
                             gfx_dpad_up,    32, 32,   // 0x00000800
                             -2 };

int kbd_nunchuck_images[] = { gfx_nunchuck_Z, 24, 24,  // 0x00010000
                              gfx_nunchuck_C, 24, 24,  // 0x00020000
                              -2 };

int kbd_classic_images[] = { gfx_classic_dpad_up,   48, 48, // 0x00010000
                             gfx_classic_dpad_left, 48, 48, // 0x00020000
                             gfx_classic_ZR,        32, 24, // 0x00040000
                             gfx_classic_x,         28, 28, // 0x00080000
                             gfx_classic_a,         28, 28, // 0x00100000
                             gfx_classic_y,         28, 28, // 0x00200000
                             gfx_classic_b,         28, 28, // 0x00400000
                             gfx_classic_ZL,        32, 24, // 0x00800000
                             -1,                    -1, -1, // 0x01000000
                             gfx_classic_R,         48, 16, // 0x02000000
                             -1,                    -1, -1, // 0x04000000
                             -1,                    -1, -1, // 0x08000000
                             -1,                    -1, -1, // 0x10000000
                             gfx_classic_L,         48, 16, // 0x20000000
                             gfx_classic_dpad_down, 48, 48, // 0x40000000
                             gfx_classic_dpad_right,48, 48, // 0x80000000
                             -2 };

int kbd_init( void )
{
  int i;

  for( i=0; i<62; i++ )
  {
    kbd_atmos[i].w = 43;
    kbd_atmos[i].h = 43;
    kbd_atmos[i].highlight = 0;
    kbd_atmos[i].highlightfade = 0;
    kbd_atmos[i].apressed = 0;
    kbd_atmos[i].bpressed = 0;
    kbd_atmos[i].wpadbits_wiimote = 0;
    kbd_atmos[i].wpadbits_nunchuck = 0;
    kbd_atmos[i].wpadbits_classic = 0;

    kbd_oric1[i].w = 43;
    kbd_oric1[i].h = 43;
    kbd_oric1[i].highlight = 0;
    kbd_oric1[i].highlightfade = 0;
    kbd_oric1[i].apressed = 0;
    kbd_oric1[i].bpressed = 0;
    kbd_oric1[i].wpadbits_wiimote = 0;
    kbd_oric1[i].wpadbits_nunchuck = 0;
    kbd_oric1[i].wpadbits_classic = 0;

    kbd_pravetz[i].w = 41;
    kbd_pravetz[i].h = 41;
    kbd_pravetz[i].highlight = 0;
    kbd_pravetz[i].highlightfade = 0;
    kbd_pravetz[i].apressed = 0;
    kbd_pravetz[i].bpressed = 0;
    kbd_pravetz[i].wpadbits_wiimote = 0;
    kbd_pravetz[i].wpadbits_nunchuck = 0;
    kbd_pravetz[i].wpadbits_classic = 0;
  }

  for( i=0; i<13; i++ )
  {
    kbd_atmos[i].x = i*43+47;
    kbd_atmos[i].y = 10;

    kbd_oric1[i].x = i*43+40;
    kbd_oric1[i].y = 15;

    kbd_pravetz[i].x = i*42+47;
    kbd_pravetz[i].y = 10;
  }
  for( i=13; i<27; i++ )
  {
    kbd_atmos[i].x = (i-13)*43+23;
    kbd_atmos[i].y = 53;

    kbd_oric1[i].x = (i-13)*43+16;
    kbd_oric1[i].y = 58;

    kbd_pravetz[i].x = (i-13)*42+25;
    kbd_pravetz[i].y = 51;
  }
  for( i=27; i<40; i++ )
  {
    kbd_atmos[i].x = (i-27)*43+38;
    kbd_atmos[i].y = 96;

    kbd_oric1[i].x = (i-27)*43+27;
    kbd_oric1[i].y = 101;

    kbd_pravetz[i].x = (i-27)*43+40;
    kbd_pravetz[i].y = 93;
  }
  kbd_atmos[39].w = 65;
  kbd_oric1[39].w = 87;

  kbd_atmos[40].x = 38;
  kbd_atmos[40].y = 139;
  kbd_atmos[40].w = 65;
  for( i=41; i<52; i++ )
  {
    kbd_atmos[i].x = (i-41)*43+103;
    kbd_atmos[i].y = 139;
  }
  kbd_atmos[51].w = 65;

  kbd_atmos[52].x = 60;
  kbd_atmos[52].y = 182;
  kbd_atmos[53].x = 103;
  kbd_atmos[53].y = 182;
  kbd_atmos[54].x = 146;
  kbd_atmos[54].y = 182;
  kbd_atmos[54].w = 344;
  kbd_atmos[55].x = 490;
  kbd_atmos[55].y = 182;
  kbd_atmos[56].x = 533;
  kbd_atmos[56].y = 182;
  kbd_atmos[57].x = 576;
  kbd_atmos[57].y = 182;
  kbd_atmos[58].x = -1;

  for( i=40; i<52; i++ )
  {
    kbd_oric1[i].x = (i-40)*43+49;
    kbd_oric1[i].y = 144;
  }

  kbd_oric1[52].x = 135;
  kbd_oric1[52].y = 187;
  kbd_oric1[53].x = 178;
  kbd_oric1[53].y = 187;
  kbd_oric1[54].x = 221;
  kbd_oric1[54].y = 187;
  kbd_oric1[54].w = 215;
  kbd_oric1[55].x = 436;
  kbd_oric1[55].y = 187;
  kbd_oric1[56].x = 479;
  kbd_oric1[56].y = 187;
  kbd_oric1[57].x = -1;

  for( i=41; i<52; i++ )
  {
    kbd_pravetz[i].x = (i-41)*43+103;
    kbd_pravetz[i].y = 137;
  }
  // backspace
  kbd_pravetz[26].x = 38;
  kbd_pravetz[26].y = 180;
  // enter
  kbd_pravetz[39].w = 65;
  kbd_pravetz[40].x = 38;
  kbd_pravetz[40].y = 137;
  kbd_pravetz[40].w = 65;
  kbd_pravetz[51].w = 65;

  kbd_pravetz[52].x = 82;
  kbd_pravetz[52].y = 180;
  kbd_pravetz[53].x = 125;
  kbd_pravetz[53].y = 180;
  kbd_pravetz[54].x = 168;
  kbd_pravetz[54].y = 180;
  kbd_pravetz[54].w = 215+88;
  kbd_pravetz[55].x = 436+40;
  kbd_pravetz[55].y = 180;
  kbd_pravetz[56].x = 479+40;
  kbd_pravetz[56].y = 180;
  /* kbd_pravetz[58].x = 500+44; */
  /* kbd_pravetz[58].y = 90; */
  kbd_pravetz[58].x = (13)*42+25;
  kbd_pravetz[58].y = 51;
  kbd_pravetz[59].x = 479+40 +42;
  kbd_pravetz[59].y = 180;
  kbd_pravetz[60].x = -1;

  return 1;
}

void kbd_set( struct machine *oric )
{
  switch( oric->type )
  {
    case MACH_ORIC1:   oric->keylayout = kbd_oric1;   break;
    case MACH_PRAVETZ: oric->keylayout = kbd_pravetz; break;
    default:           oric->keylayout = kbd_atmos;   break;
  }
}

void kbd_handler( struct machine *oric )
{
  int dontunpresskey = -1;
  int i;
  static int loverkey = -1, waitrelease = 0;
  static int assignkey = -1;
  struct kbdkey *k = (oric->overkey!=-1) ? &oric->keylayout[oric->overkey] : NULL;

  if( !oric->disp.showkbd )
  {
    for( i=0; oric->keylayout[i].x != -1; i++ )
    {
      struct kbdkey *kp = &oric->keylayout[i];
      if ((oric->inputs.wpadheld & kp->wpadbits_wiimote) != 0)
      {
        if ((!kp->apressed) &&
            (!kp->bpressed) &&
            (!kp->jpressed))
          kbd_osdkeypress( &oric->ay, i, 1 );
        kp->jpressed = 1;
      }
      else
      {
        if (kp->jpressed)
        {
          if ((!kp->apressed) &&
              (!kp->bpressed))
            kbd_osdkeypress( &oric->ay, i, 0 );
          kp->jpressed = 0;
        }
      }
    }
    return;
  }

  if ((waitrelease) && (oric->inputs.wpadheld))
    return;
 
  waitrelease = 0;

  if( assignkey != -1 )
  {
    if (oric->inputs.wpadheld)
    {
      for (i=0; kbd_wiimote_images[i]!=-2; i+=3)
      {
        if (kbd_wiimote_images[i]==-1) continue;
        if (oric->inputs.wpadheld&(1<<(i/3))) break;
      }

      if (kbd_wiimote_images[i]!=-2)
      {
        oric->keylayout[assignkey].wpadbits_wiimote = 1<<(i/3);
        assignkey = -1;
        waitrelease = 1;
      }
    }
  }

  if( assignkey != -1 )
    return;

  if( oric->overkey != loverkey )
  {
    for (i=0; i<160*224; i++)
      oric->disp.kbdinfrgb[i] = 0x000080ff;

    if( oric->overkey == -1 )
    {
      oric->disp.kbd_wiimote_show = 0;
      oric->disp.kbd_nunchuck_show = 0;
      oric->disp.kbd_classic_show = 0;
    }
    else
    {
      oric->disp.kbd_wiimote_show  = k->wpadbits_wiimote;
      oric->disp.kbd_nunchuck_show = k->wpadbits_nunchuck;
      oric->disp.kbd_classic_show  = k->wpadbits_classic;

      //                                                 12345678901234567890123456
      render_printstr( oric->disp.kbdinfrgb, 160, 0, 0, " PRESS '-' TO ASSIGN KEY", 0xffffffff );
      render_texturemangle( 160, 224, (u8 *)oric->disp.kbdinfrgb, oric->disp.kbdinftex );
    }

    DCFlushRange( oric->disp.kbdinftex, 160*224*4 );
    oric->disp.pleaseinvtexes = 1;
 
    loverkey = oric->overkey;
  }

  if( oric->overkey != -1 )
  {
    if( oric->inputs.wpadheld & WPAD_BUTTON_A )
    {
      if ((!k->apressed) &&
          (!k->bpressed) &&
          (!k->jpressed))
        kbd_osdkeypress( &oric->ay, oric->overkey, 1 );
      k->apressed = 1;
      k->bpressed = 0;
      k->highlight = 180;
      k->highlightfade = 1;
      dontunpresskey = oric->overkey;
    }
    else if( oric->inputs.wpaddown & WPAD_BUTTON_B )
    {
      if( k->bpressed )
      {
        k->apressed = 0;
        k->bpressed = 0;
        k->highlightfade = 1;
        kbd_osdkeypress( &oric->ay, oric->overkey, 0 );
      } else {
        if ((!k->apressed) &&
            (!k->jpressed))
          kbd_osdkeypress( &oric->ay, oric->overkey, 1 );
        k->apressed = 0;
        k->bpressed = 1;
        k->highlight = 180;
        k->highlightfade = 0;
      }
    }
    else if( oric->inputs.wpaddown & WPAD_BUTTON_MINUS )
    {
      for (i=0; i<160*224; i++)
        oric->disp.kbdinfrgb[i] = 0x000080ff;
      //                                                 12345678901234567890123456
      render_printstr( oric->disp.kbdinfrgb, 160, 0, 0, " PRESS BUTTON TO ASSIGN", 0xffffffff );
      render_texturemangle( 160, 224, (u8 *)oric->disp.kbdinfrgb, oric->disp.kbdinftex );
      waitrelease = 1;
      assignkey = oric->overkey;
      DCFlushRange( oric->disp.kbdinftex, 160*224*4 );
      oric->disp.pleaseinvtexes = 1;
    }
  }

  for( i=0; oric->keylayout[i].x != -1; i++ )
  {
    struct kbdkey *kp = &oric->keylayout[i];
    if( ( kp->apressed ) && ( dontunpresskey != i ) )
    {
      kp->apressed = 0;
      kp->bpressed = 0;
      kp->highlightfade = 1;
      if (!kp->jpressed)
        kbd_osdkeypress( &oric->ay, i, 0 );
    }
  }
}

void kbd_queuekeys( struct ay8912 *ay, char *str )
{
  ay->keyqueue   = str;
  ay->keysqueued = strlen( str );
  ay->kqoffs     = 0;
}

/*
** Handle a key press
*/
void kbd_keypress( struct ay8912 *ay, unsigned short key, int down )
{
  int i;

  // No key?
  if( key == 0 ) return;

  // Does this key exist on the Oric?
  for( i=0; i<64; i++ )
    if( keytab[i] == key ) break;

  // No...
  if( i == 64 ) return;

  // Key down event, or key up event?
  if( down )
    ay->keystates[i>>3] |= (1<<(i&7));          // Down, so set the corresponding bit
  else
    ay->keystates[i>>3] &= ~(1<<(i&7));         // Up, so clear it

  // Maybe update the VIA
  if( ay->currkeyoffs == (i>>3) )
  {
    if( ay->keystates[ay->currkeyoffs] & (ay->eregs[AY_PORT_A]^0xff) )
      ay->oric->via.write_port_b( &ay->oric->via, 0x08, 0x08 );
    else
      ay->oric->via.write_port_b( &ay->oric->via, 0x08, 0x00 );
  }
}

void kbd_osdkeypress( struct ay8912 *ay, int key, int down )
{
  int i;

  // No key?
  if( key == -1 ) return;

  // Valid key?
  for( i=0; i<64; i++ )
    if( osdkeytab[i] == key ) break;

  // No...
  if( i == 64 ) return;

  // Key down event, or key up event?
  if( down )
    ay->keystates[i>>3] |= (1<<(i&7));          // Down, so set the corresponding bit
  else
    ay->keystates[i>>3] &= ~(1<<(i&7));         // Up, so clear it

  // Maybe update the VIA
  if( ay->currkeyoffs == (i>>3) )
  {
    if( ay->keystates[ay->currkeyoffs] & (ay->eregs[AY_PORT_A]^0xff) )
      ay->oric->via.write_port_b( &ay->oric->via, 0x08, 0x08 );
    else
      ay->oric->via.write_port_b( &ay->oric->via, 0x08, 0x00 );
  }
}
