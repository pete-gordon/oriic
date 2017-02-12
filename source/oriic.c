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

#include <ogcsys.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <fat.h>
#include <dirent.h>
#include <unistd.h>

#include "6502.h"
#include "ula.h"
#include "8912.h"
#include "6551.h"
#include "via.h"
#include "tape.h"
#include "disk.h"
#include "oriic.h"
#include "render.h"
#include "keyboard.h"

struct machine *oric;

extern u32 rom_basic10[];
extern u32 rom_basic11b[];
extern u32 rom_microdisc[];
extern u32 rom_jasmin[];
extern u8 rom_pravetzo[];

char tapepath[2048], tapefile[2048+128];
char diskpath[2048], diskfile[2048+128];
char tdiskpath[2048], tdiskfile[2048+128];

// Main settings panel                       Type             X      Y       W       ID                  S  STR
static struct sp_button spmain_btns[]  = { { BTYP_BUTTON,     32.0f,  60.0f, 220.0f, BTN_INSERTTAPE,     0, "Insert tape" },
                                           { BTYP_BUTTON,     32.0f,  84.0f, 220.0f, BTN_INSERTDISK0,    0, "Insert disk 0" },
                                           { BTYP_BUTTON,     32.0f, 108.0f, 220.0f, BTN_INSERTDISK1,    0, "Insert disk 1" },
                                           { BTYP_BUTTON,     32.0f, 132.0f, 220.0f, BTN_INSERTDISK2,    0, "Insert disk 2" },
                                           { BTYP_BUTTON,     32.0f, 156.0f, 220.0f, BTN_INSERTDISK3,    0, "Insert disk 3" },

                                           { BTYP_BUTTON,    268.0f,  60.0f, 220.0f, BTN_HWSETTINGS,     0, "Hardware settings" },
                                           { BTYP_BUTTON,    268.0f,  84.0f, 220.0f, BTN_VIDEOSETTINGS,  0, "Video settings" },
                                           { BTYP_BUTTON,    268.0f, 302.0f, 220.0f, BTN_RESET,          0, "Reset oric" },

                                           { BTYP_BUTTON,     32.0f, 350.0f, 220.0f, BTN_CLOSESETTINGS,  0, "Return to Emulator" },
                                           { BTYP_BUTTON,    268.0f, 350.0f, 220.0f, BTN_QUIT,           0, "Quit Oriic" },
                                           { BTYP_BUTTON,    268.0f, 108.0f, 220.0f, BTN_CONFIG_JOYSTICK_KEYS, 0, "Configure JoyKyes" },
                                           { BTYP_BUTTON,      0.0f,   0.0f,   0.0f, -1,                 0, NULL } };

static struct sp_button sphw_btns[]    = { { BTYP_RADIO,     150.0f,  60.0f, 220.0f, BTN_SYSTEM,         2, "System|Oric-1|Oric-1 16K|Atmos" },
                                           { BTYP_RADIO,     150.0f, 176.0f, 220.0f, BTN_DISK,           0, "Disk Drive|None|Microdisc|Jasmin" },
                                           { BTYP_BUTTON,    150.0f, 350.0f, 220.0f, BTN_MAINSETTINGS,   0, "Back" },
                                           { BTYP_BUTTON,      0.0f,   0.0f,   0.0f, -1,                 0, NULL } };

static struct sp_button spvideo_btns[] = { { BTYP_CHECKBOX,  150.0f,  60.0f, 220.0f, BTN_VIDEOSTRETCH,   1, "Horizontal stretch" },
                                           { BTYP_CHECKBOX,  150.0f,  84.0f, 220.0f, BTN_VIDEOLINEAR,    1, "Smooth scaling" },
                                           { BTYP_BUTTON,    150.0f, 350.0f, 220.0f, BTN_MAINSETTINGS,   0, "Back" },
                                           { BTYP_BUTTON,      0.0f,   0.0f,   0.0f, -1,                 0, NULL } };

static struct sp_button spjoy_btns[]    = {{ BTYP_BUTTON,    32.0f,  60.0f, 120.0f, BTN_SAVEKEYJOY1,   0, "Save 1" },
                                           { BTYP_BUTTON,    32.0f,  84.0f, 120.0f, BTN_SAVEKEYJOY2,   0, "Save 2" },
                                           { BTYP_BUTTON,    32.0f, 108.0f, 120.0f, BTN_SAVEKEYJOY3,   0, "Save 3" },
                                           { BTYP_BUTTON,    32.0f, 132.0f, 120.0f, BTN_SAVEKEYJOY4,   0, "Save 4" },
                                           { BTYP_BUTTON,    150.0f,  60.0f, 120.0f, BTN_LOADKEYJOY1,   0, "Load 1" },
                                           { BTYP_BUTTON,    150.0f,  84.0f, 120.0f, BTN_LOADKEYJOY2,   0, "Load 2" },
                                           { BTYP_BUTTON,    150.0f, 108.0f, 120.0f, BTN_LOADKEYJOY3,   0, "Load 3" },
                                           { BTYP_BUTTON,    150.0f, 132.0f, 120.0f, BTN_LOADKEYJOY4,   0, "Load 4" },
                                           { BTYP_BUTTON,    150.0f, 156.0f, 120.0f, BTN_LOADKEYJOY5,   0, "Clear" },
                                           { BTYP_BUTTON,    150.0f, 350.0f, 220.0f, BTN_MAINSETTINGS,   0, "Back" },
                                           { BTYP_BUTTON,      0.0f,   0.0f,   0.0f, -1,                 0, NULL } };

// Main settings panel                       Type             X      Y       W       ID                  S  STR      H
static struct sp_button spfreq_btns[]  = { { BTYP_LIST,       32.0f,  60.0f, 452.0f, BTN_FILELIST,       0, "",      240.0f },
                                           { BTYP_HIDDEN,    150.0f, 308.0f, 452.0f, BTN_FILEOPTION,     0, "" },
                                           { BTYP_BUTTON,    150.0f, 350.0f, 220.0f, BTN_MAINSETTINGS,   0, "Cancel" },
                                           { BTYP_BUTTON,      0.0f,   0.0f,   0.0f, -1,                 0, NULL } };

static struct settingspanel settingspanels[] = { { "Oriic WIP",         spmain_btns },
                                                 { "Filereq",           spfreq_btns },
                                                 { "Hardware settings", sphw_btns },
                                                 { "Video settings",    spvideo_btns },
                                                 { "JoyKey settings",   spjoy_btns },
                                                 { NULL,                NULL } };


// Set up the RAM to one of two known power-on states
static void blank_ram( int how, u8 *mem, int size )
{
  int i, j;

  switch( how )
  {
    case 0:
      for( i=0; i<size; i+=256 )
      {
        for( j=0; j<128; j++ )
        {
          mem[i+j    ] = 0;
          mem[i+j+128] = 255;
        }
      }
      break;
    
    default:
      for( i=0; i<size; i+=2 )
      {
        mem[i  ] = 0xff;
        mem[i+1] = 0x00;
      }
      break;
  }
}

// Oric Atmos CPU write
void atmoswrite( struct m6502 *cpu, unsigned short addr, unsigned char data )
{
  struct machine *oric = (struct machine *)cpu->userdata;
  if( ( !oric->romdis ) && ( addr >= 0xc000 ) ) return;  // Can't write to ROM!
  if( ( addr & 0xff00 ) == 0x0300 )
  {
    via_write( &oric->via, addr, data );
    return;
  }
  oric->mem[addr] = data;
}

// 16k oric-1 CPU write
void o16kwrite( struct m6502 *cpu, unsigned short addr, unsigned char data )
{
  struct machine *oric = (struct machine *)cpu->userdata;
  if( ( !oric->romdis ) && ( addr >= 0xc000 ) ) return;  // Can't write to ROM!
  if( ( addr & 0xff00 ) == 0x0300 )
  {
    via_write( &oric->via, addr, data );
    return;
  }

  oric->mem[addr&0x3fff] = data;
}

/*
// Oric Telestrat CPU write
void telestratwrite( struct m6502 *cpu, unsigned short addr, unsigned char data )
{
  struct machine *oric = (struct machine *)cpu->userdata;

  if( addr >= 0xc000 )
  {
//    if( oric->romdis )
//    {
//      if( ( oric->md.diskrom ) && ( addr >= 0xe000 ) ) return; // Can't write to ROM!
//    } else {
      switch( oric->tele_banktype )
      {
        case TELEBANK_HALFNHALF:
          if( addr >= 0xe000 ) break;
        case TELEBANK_RAM:
          oric->rom[addr-0xc000] = data;
          break;
      }
      return;
//    }
  }

  if( ( addr & 0xff00 ) == 0x0300 )
  {
    switch( addr & 0x0f0 )
    {
      case 0x20:
        via_write( &oric->tele_via, addr, data );
        break;
      
      case 0x10:
        if( addr >= 0x31c )
          acia_write( &oric->tele_acia, addr, data );
        else
          microdisc_write( &oric->md, addr, data );
        break;
      
      default:
        via_write( &oric->via, addr, data );
        break;
    }
    return;
  }

  oric->mem[addr] = data;
}
*/

// Atmos + jasmin
void jasmin_atmoswrite( struct m6502 *cpu, unsigned short addr, unsigned char data )
{
  struct machine *oric = (struct machine *)cpu->userdata;

  if( oric->jasmin.olay == 0 )
  {
    if( oric->romdis )
    {
      if( addr >= 0xf800 ) return; // Can't write to jasmin rom
    } else {
      if( addr >= 0xc000 ) return; // Can't write to BASIC rom
    }
  }
  
  if( ( addr & 0xff00 ) == 0x0300 )
  {
    if( ( addr >= 0x3f4 ) && ( addr < 0x400 ) )
    {
      jasmin_write( &oric->jasmin, addr, data );
      return;
    }

    via_write( &oric->via, addr, data );
    return;
  }
  oric->mem[addr] = data;
}

// 16k + jasmin
void jasmin_o16kwrite( struct m6502 *cpu, unsigned short addr, unsigned char data )
{
  struct machine *oric = (struct machine *)cpu->userdata;

  if( oric->jasmin.olay == 0 )
  {
    if( oric->romdis )
    {
      if( addr >= 0xf800 ) return; // Can't write to jasmin rom
    } else {
      if( addr >= 0xc000 ) return; // Can't write to BASIC rom
    }
  }

  if( ( addr & 0xff00 ) == 0x0300 )
  {
    if( ( addr >= 0x3f4 ) && ( addr < 0x400 ) )
    {
      jasmin_write( &oric->jasmin, addr, data );
      return;
    }

    via_write( &oric->via, addr, data );
    return;
  }

  oric->mem[addr&0x3fff] = data;
}

// Atmos + microdisc
void microdisc_atmoswrite( struct m6502 *cpu, unsigned short addr, unsigned char data )
{
  struct machine *oric = (struct machine *)cpu->userdata;
  if( oric->romdis )
  {
    if( ( oric->md.diskrom ) && ( addr >= 0xe000 ) ) return; // Can't write to ROM!
  } else {
    if( addr >= 0xc000 ) return;
  }

  if( ( addr & 0xff00 ) == 0x0300 )
  {
    if( ( addr >= 0x310 ) && ( addr < 0x31c ) )
    {
      microdisc_write( &oric->md, addr, data );
    } else {
      via_write( &oric->via, addr, data );
    }
    return;
  }
  oric->mem[addr] = data;
}

// Atmos + microdisc
void microdisc_o16kwrite( struct m6502 *cpu, unsigned short addr, unsigned char data )
{
  struct machine *oric = (struct machine *)cpu->userdata;
  if( oric->romdis )
  {
    if( ( oric->md.diskrom ) && ( addr >= 0xe000 ) ) return; // Can't write to ROM!
  } else {
    if( addr >= 0xc000 ) return;
  }

  if( ( addr & 0xff00 ) == 0x0300 )
  {
    if( ( addr >= 0x310 ) && ( addr < 0x31c ) )
    {
      microdisc_write( &oric->md, addr, data );
    } else {
      via_write( &oric->via, addr, data );
    }
    return;
  }
  oric->mem[addr&0x3fff] = data;
}

// VIA is returned as RAM since it isn't ROM
int isram( struct machine *oric, unsigned short addr )
{
  if( addr < 0xc000 ) return 1;

/*
  if( oric->type == MACH_TELESTRAT )
  {
    return oric->tele_banktype == TELEBANK_RAM;
  }
*/
  if( !oric->romdis ) return 0;

  switch( oric->drivetype )
  {
    case DRV_MICRODISC:
      if( !oric->md.diskrom ) return 1;
      if( addr >= 0xe000 ) return 0;
      break;
    
    case DRV_JASMIN:
      if( oric->jasmin.olay ) return 1;
      if( addr >= 0xf800 ) return 0;
      break;
  }

  return 1;
}

// Oric Atmos CPU read
unsigned char atmosread( struct m6502 *cpu, unsigned short addr )
{
  struct machine *oric = (struct machine *)cpu->userdata;

//  if( oric->lightpen )
//  {
//    if( addr == 0x3e0 ) return oric->lightpenx;
//    if( addr == 0x3e1 ) return oric->lightpeny;
//  }

  if( ( addr & 0xff00 ) == 0x0300 )
    return via_read( &oric->via, addr );

  if( ( !oric->romdis ) && ( addr >= 0xc000 ) )
    return oric->rom[addr-0xc000];

  return oric->mem[addr];
}

// Oric-1 16K CPU read
unsigned char o16kread( struct m6502 *cpu, unsigned short addr )
{
  struct machine *oric = (struct machine *)cpu->userdata;

//  if( oric->lightpen )
//  {
//    if( addr == 0x3e0 ) return oric->lightpenx;
//    if( addr == 0x3e1 ) return oric->lightpeny;
//  }

  if( ( addr & 0xff00 ) == 0x0300 )
    return via_read( &oric->via, addr );

  if( ( !oric->romdis ) && ( addr >= 0xc000 ) )
    return oric->rom[addr-0xc000];

  return oric->mem[addr&0x3fff];
}

/*
// Oric Telestrat CPU read
unsigned char telestratread( struct m6502 *cpu, unsigned short addr )
{
  struct machine *oric = (struct machine *)cpu->userdata;

  if( oric->lightpen )
  {
    if( addr == 0x3e0 ) return oric->lightpenx;
    if( addr == 0x3e1 ) return oric->lightpeny;
  }

  if( ( addr & 0xff00 ) == 0x0300 )
  {
    switch( addr & 0x0f0 )
    {
      case 0x010:
        if( addr >= 0x31c )
        {
          return acia_read( &oric->tele_acia, addr );
        }

        return microdisc_read( &oric->md, addr );

      case 0x020:
        return via_read( &oric->tele_via, addr );
    }

    return via_read( &oric->via, addr );
  }

  if( addr >= 0xc000 )
  {
    if( oric->romdis )
    {
      if( ( oric->md.diskrom ) && ( addr >= 0xe000 ) )
        return ((u8 *)rom_microdisc)[addr-0xe000];
    } else {
      return oric->rom[addr-0xc000];
    }
  }

  return oric->mem[addr];
}
*/

// Atmos + jasmin
unsigned char jasmin_atmosread( struct m6502 *cpu, unsigned short addr )
{
  struct machine *oric = (struct machine *)cpu->userdata;

//  if( oric->lightpen )
//  {
//    if( addr == 0x3e0 ) return oric->lightpenx;
//    if( addr == 0x3e1 ) return oric->lightpeny;
//  }

  if( oric->jasmin.olay == 0 )
  {
    if( oric->romdis )
    {
      if( addr >= 0xf800 ) return ((u8 *)rom_jasmin)[addr-0xf800];
    } else {
      if( addr >= 0xc000 ) return oric->rom[addr-0xc000];
    }
  }
  
  if( ( addr & 0xff00 ) == 0x0300 )
  {
    if( ( addr >= 0x3f4 ) && ( addr < 0x400 ) )
      return jasmin_read( &oric->jasmin, addr );

    return via_read( &oric->via, addr );
  }

  return oric->mem[addr];
}

// 16k + jasmin
unsigned char jasmin_o16kread( struct m6502 *cpu, unsigned short addr )
{
  struct machine *oric = (struct machine *)cpu->userdata;

//  if( oric->lightpen )
//  {
//    if( addr == 0x3e0 ) return oric->lightpenx;
//    if( addr == 0x3e1 ) return oric->lightpeny;
//  }

  if( oric->jasmin.olay == 0 )
  {
    if( oric->romdis )
    {
      if( addr >= 0xf800 ) return ((u8 *)rom_jasmin)[addr-0xf800];
    } else {
      if( addr >= 0xc000 ) return oric->rom[addr-0xc000];
    }
  }

  if( ( addr & 0xff00 ) == 0x0300 )
  {
    if( ( addr >= 0x3f4 ) && ( addr < 0x400 ) )
      return jasmin_read( &oric->jasmin, addr );

    return via_read( &oric->via, addr );
  }

  return oric->mem[addr&0x3fff];
}

// Atmos + microdisc
unsigned char microdisc_atmosread( struct m6502 *cpu, unsigned short addr )
{
  struct machine *oric = (struct machine *)cpu->userdata;

//  if( oric->lightpen )
//  {
//    if( addr == 0x3e0 ) return oric->lightpenx;
//    if( addr == 0x3e1 ) return oric->lightpeny;
//  }

  if( oric->romdis )
  {
    if( ( oric->md.diskrom ) && ( addr >= 0xe000 ) )
      return ((u8 *)rom_microdisc)[addr-0xe000];
  } else {
    if( addr >= 0xc000 )
      return oric->rom[addr-0xc000];
  }

  if( ( addr & 0xff00 ) == 0x0300 )
  {
    if( ( addr >= 0x310 ) && ( addr < 0x31c ) )
      return microdisc_read( &oric->md, addr );

    return via_read( &oric->via, addr );
  }

  return oric->mem[addr];
}

// Atmos + microdisc
unsigned char microdisc_o16kread( struct m6502 *cpu, unsigned short addr )
{
  struct machine *oric = (struct machine *)cpu->userdata;

//  if( oric->lightpen )
//  {
//    if( addr == 0x3e0 ) return oric->lightpenx;
//    if( addr == 0x3e1 ) return oric->lightpeny;
//  }

  if( oric->romdis )
  {
    if( ( oric->md.diskrom ) && ( addr >= 0xe000 ) )
      return ((u8 *)rom_microdisc)[addr-0xe000];
  } else {
    if( addr >= 0xc000 )
      return oric->rom[addr-0xc000];
  }

  if( ( addr & 0xff00 ) == 0x0300 )
  {
    if( ( addr >= 0x310 ) && ( addr < 0x31c ) )
      return microdisc_read( &oric->md, addr );

    return via_read( &oric->via, addr );
  }

  return oric->mem[addr&0x3fff];
}

void setromon( struct machine *oric )
{
  // Determine if the ROM is currently active
  oric->romon = 1;
  if( oric->drivetype == DRV_JASMIN )
  {
    if( oric->jasmin.olay == 0 )
    {
      oric->romon = !oric->romdis;
    } else {
      oric->romon = 0;
    }
  } else {
    oric->romon = !oric->romdis;
  }
}

void set_buttons( struct machine *oric )
{
  int i, j;
  struct settingspanel *sp;

  for( i=0; i<SP_LAST; i++ )
  {
    sp = &settingspanels[i];
    for( j=0; sp->btns[j].w!=0; j++ )
    {
      switch( sp->btns[j].id )
      {
        case BTN_SYSTEM:       sp->btns[j].state = oric->type;              break;
        case BTN_DISK:         sp->btns[j].state = oric->drivetype;         break;
        case BTN_VIDEOSTRETCH: sp->btns[j].state = oric->odisp.hstretch;    break;
        case BTN_VIDEOLINEAR:  sp->btns[j].state = oric->odisp.linearscale; break;
      }
    }
  }
}


// Initialise the machiiiiineeee!!!!111
void init_machine( struct machine *oric, int type )
{
  oric->type = oric->settype = type;

  oric->tapeturbo_forceoff = 0;

  ula_powerup_default( &oric->ula );
  m6502_init( &oric->cpu, (void*)oric );
  blank_ram( oric->rampattern, oric->mem, 65536 );

  oric->tapeturbo_syncstack = -1;

  oric->porta_ay = 0;
  oric->porta_is_ay = 1;

  kbd_set( oric );

  switch( type )
  {
    case MACH_ORIC1_16K:
    case MACH_ORIC1:
      // Filename decoding patch addresses
      oric->pch_fd_cload_getname_pc = 0xe4af;
      oric->pch_fd_recall_getname_pc = -1;
      oric->pch_fd_getname_addr = 0x0035;
      oric->pch_fd_available = 1;

      // Turbo tape patch addresses
      oric->pch_tt_getsync_pc = 0xe696;
      oric->pch_tt_getsync_end_pc = 0xe6b9;
      oric->pch_tt_getsync_loop_pc = 0xe681;
      oric->pch_tt_readbyte_pc = 0xe6c9;
      oric->pch_tt_readbyte_end_pc = 0xe65b;
      oric->pch_tt_readbyte_storebyte_addr = 0x002f;
      oric->pch_tt_readbyte_storezero_addr = -1;
      oric->pch_tt_readbyte_setcarry = 0;
      oric->pch_tt_available = 1;

      oric->rom = (u8 *)rom_basic10;
      switch( oric->drivetype )
      {
        case DRV_MICRODISC:
          oric->cpu.read = (type==MACH_ORIC1) ? microdisc_atmosread : microdisc_o16kread;
          oric->cpu.write = (type==MACH_ORIC1) ? microdisc_atmoswrite : microdisc_o16kwrite;
          oric->romdis = 1;
          microdisc_init( &oric->md, &oric->wddisk, oric );
          break;
        
        case DRV_JASMIN:
          oric->cpu.read = (type==MACH_ORIC1) ? jasmin_atmosread : jasmin_o16kread;
          oric->cpu.write = (type==MACH_ORIC1) ? jasmin_atmoswrite : jasmin_o16kwrite;
          oric->romdis = 1;
          jasmin_init( &oric->jasmin, &oric->wddisk, oric );
          break;

        case DRV_PRAVETZ:
        default:
          oric->drivetype = DRV_NONE;
          oric->cpu.read = (type==MACH_ORIC1) ? atmosread : o16kread;
          oric->cpu.write = (type==MACH_ORIC1) ? atmoswrite : o16kwrite;
          oric->romdis = 0;
          break;
      }
      break;

    case MACH_PRAVETZ:
      oric->pch_fd_cload_getname_pc = 0xe4ac;
      oric->pch_fd_recall_getname_pc = 0xe9d8;
      oric->pch_fd_getname_addr = 0x027f;
      oric->pch_fd_available = 1;

      // Turbo tape patch addresses
      oric->pch_tt_getsync_pc = 0xe735;
      oric->pch_tt_getsync_end_pc = 0xe759;
      oric->pch_tt_getsync_loop_pc = 0xe720;
      oric->pch_tt_readbyte_pc = 0xe6c9;
      oric->pch_tt_readbyte_end_pc = 0xe6fb;
      oric->pch_tt_readbyte_storebyte_addr = 0x002f;
      oric->pch_tt_readbyte_storezero_addr = 0x02b1;
      oric->pch_tt_readbyte_setcarry = 1;
      oric->pch_tt_available = 1;

      oric->rom = (u8 *)rom_pravetzo;
      switch( oric->drivetype )
      {
        case DRV_MICRODISC:
          oric->cpu.read = microdisc_atmosread;
          oric->cpu.write = microdisc_atmoswrite;
          oric->romdis = 1;
          microdisc_init( &oric->md, &oric->wddisk, oric );
          break;
        
        case DRV_JASMIN:
          oric->cpu.read = jasmin_atmosread;
          oric->cpu.write = jasmin_atmoswrite;
          oric->romdis = 0;
          jasmin_init( &oric->jasmin, &oric->wddisk, oric );
          break;

        case DRV_PRAVETZ:
        default:
          oric->drivetype = DRV_NONE;
          oric->cpu.read = atmosread;
          oric->cpu.write = atmoswrite;
          oric->romdis = 0;
          break;
      }
      break;
    
    default:
      // Filename decoding patch addresses
      oric->pch_fd_cload_getname_pc = 0xe4ac;
      oric->pch_fd_recall_getname_pc = 0xe9d8;
      oric->pch_fd_getname_addr = 0x027f;
      oric->pch_fd_available = 1;

      // Turbo tape patch addresses
      oric->pch_tt_getsync_pc = 0xe735;
      oric->pch_tt_getsync_end_pc = 0xe759;
      oric->pch_tt_getsync_loop_pc = 0xe720;
      oric->pch_tt_readbyte_pc = 0xe6c9;
      oric->pch_tt_readbyte_end_pc = 0xe6fb;
      oric->pch_tt_readbyte_storebyte_addr = 0x002f;
      oric->pch_tt_readbyte_storezero_addr = 0x02b1;
      oric->pch_tt_readbyte_setcarry = 1;
      oric->pch_tt_available = 1;

      oric->rom = (u8 *)rom_basic11b;
      switch( oric->drivetype )
      {
        case DRV_MICRODISC:
          oric->cpu.read = microdisc_atmosread;
          oric->cpu.write = microdisc_atmoswrite;
          oric->romdis = 1;
          microdisc_init( &oric->md, &oric->wddisk, oric );
          break;
        
        case DRV_JASMIN:
          oric->cpu.read = jasmin_atmosread;
          oric->cpu.write = jasmin_atmoswrite;
          oric->romdis = 0;
          jasmin_init( &oric->jasmin, &oric->wddisk, oric );
          break;

        case DRV_PRAVETZ:
        default:
          oric->drivetype = DRV_NONE;
          oric->cpu.read = atmosread;
          oric->cpu.write = atmoswrite;
          oric->romdis = 0;
          break;
      }
      break;
  }

  oric->overkey = -1;

  setromon( oric );
  ay_init( &oric->ay, oric );
  ula_init( &oric->ula, oric );
  via_init( &oric->via, oric, VIA_MAIN);
  m6502_reset( &oric->cpu );

  set_buttons( oric );
}

void emulate_frame( struct machine *oric )
{
  int needrender;

  do {
    oric->cpu.rastercycles += oric->cyclesperraster;
    while( oric->cpu.rastercycles > 0 )
    {
      m6502_set_icycles( &oric->cpu );
      tape_patches( oric );
      ay_ticktock( &oric->ay, oric->cpu.icycles );
      via_clock( &oric->via, oric->cpu.icycles );
      ula_ticktock( &oric->ula, oric->cpu.icycles );
      wd17xx_ticktock( &oric->wddisk, oric->cpu.cycles );
      m6502_inst( &oric->cpu );
    }
    needrender = ula_doraster( &oric->ula );
  } while( !needrender );
}

void refreshpanel( struct machine *oric )
{
  int i, j, k;
  struct sp_button *spb;

  memset( &oric->disp.pnlrgb, 0, 520*400*4 );

  render_dblprintstr( oric->disp.pnlrgb, 520, 33, 25, oric->disp.currpanel->title, 0x000000a0 );
  render_dblprintstr( oric->disp.pnlrgb, 520, 32, 24, oric->disp.currpanel->title, 0xffffffff );

  for( i=0; oric->disp.currpanel->btns[i].str; i++ )
  {
    spb = &oric->disp.currpanel->btns[i];

    switch( spb->type )
    {
      case BTYP_RADIO:
        render_printstr( oric->disp.pnlrgb, 520, spb->x+21, spb->y-13, spb->str, 0x000000a0 );
        render_printstr( oric->disp.pnlrgb, 520, spb->x+20, spb->y-14, spb->str, 0xffff88ff );
        k = 0;
        while( ( spb->str[k] != 0 ) && ( spb->str[k] != '|' ) ) k++;
        if ( spb->str[k] != 0 ) k++;
        for( j=0; spb->str[k]; j+=24 )
        {
          render_printstr( oric->disp.pnlrgb, 520, spb->x+31, spb->y+j+9, &spb->str[k], 0x000000a0 );
          render_printstr( oric->disp.pnlrgb, 520, spb->x+30, spb->y+j+8, &spb->str[k], 0xffffffff );

          while( ( spb->str[k] != 0 ) && ( spb->str[k] != '|' ) ) k++;
          if ( spb->str[k] != 0 ) k++;
        }
        break;

      case BTYP_CHECKBOX:
        render_printstr( oric->disp.pnlrgb, 520, spb->x+35, spb->y+9, spb->str, 0x000000a0 );
        render_printstr( oric->disp.pnlrgb, 520, spb->x+34, spb->y+8, spb->str, 0xffffffff );
        break;

      case BTYP_BUTTON:
        render_printstr( oric->disp.pnlrgb, 520, (spb->x+spb->w/2+1)-(strlen(spb->str)*3), spb->y+9, spb->str, 0x000000a0 );
        render_printstr( oric->disp.pnlrgb, 520, (spb->x+spb->w/2)-(strlen(spb->str)*3), spb->y+8, spb->str, 0xffffffff );
        break;
      
      case BTYP_LIST:
        {
          struct listdata *data;
          struct listitem *item;

          data = (struct listdata *)spb->data;
          if( !data ) break;

          for( item = data->head, j=0, k=0; item; item = item->next, j++ )
          {
            if( j < data->top ) continue;

            render_printstr( oric->disp.pnlrgb, 520, spb->x+2, spb->y+k, item->display, 0xffccccff );

            k += 12;

            if( k > (spb->h-12) ) break;
          }
        }
        break;
    }
  }

  oric->disp.pleaseinvtexes = 1;
  render_texturemangle( 520, 400, (u8 *)oric->disp.pnlrgb, oric->disp.pnltex );
  DCFlushRange( oric->disp.pnltex, 520*400*4 );
}

void setpanel( struct machine *oric, int whichpanel )
{
  oric->disp.currpanelid = whichpanel;
  oric->disp.currpanel = &settingspanels[whichpanel];
  oric->disp.currbutton = -1;

  refreshpanel( oric );
}

void setemustate( struct machine *oric, int newstate )
{
  switch( newstate )
  {
    case ES_RUNNING:
      sobj_zoom( &oric->disp.sobj[SO_SETTINGS], 0.6f,   0, 8 );
      start_audio();
      break;
    
    case ES_SETTINGS:
      sobj_zoom( &oric->disp.sobj[SO_SETTINGS], 1.0f, 255, 8 );
      stop_audio();
      break;
  }

  oric->emustate = newstate;
}

void init_defaults( struct machine *oric )
{
  oric->odisp.hstretch    = 1;
  oric->odisp.linearscale = 1;

  oric->tapeturbo = 1;
  oric->tapeautorun = 1;

  oric->type = MACH_ATMOS;

  oric->rom = NULL;

  oric->drivetype = DRV_NONE;

  oric->tapebuf = NULL;
  oric->tapelen = 0;
  oric->tapemotor = 0;
  oric->vsynchack = 0;
  oric->tapeturbo = 1;
  oric->tapeturbo_forceoff = 0;
  oric->tapeautorewind = 0;
  oric->tapeautoinsert = 1;
  oric->tapename[0] = 0;
  oric->lasttapefile[0] = 0;
  oric->rampattern = 0;

  oric->porta_ay  = 0;
  oric->porta_is_ay = 1;

  strcpy( tapepath, "/apps/Oriic/tapes" );
  strcpy( diskpath, "/apps/Oriic/disks" );
  strcpy( tdiskpath, "/apps/Oriic/teledisks" );
}

struct listitem *alloc_listitem( struct sp_button *parent, char *content )
{
  int maxchars, clen, dlen;
  struct listitem *litem = NULL, *fitem, *previtem;
  struct listdata *ldata = NULL;

  maxchars = (parent->w-32)/6;

  clen = strlen( content );
  if( clen > maxchars )
  {
    dlen = maxchars;
  } else {
    dlen = clen;
  }

  ldata = (struct listdata *)parent->data;
  if( !ldata )
  {
    ldata = malloc( sizeof( struct listdata ) );
    if (!ldata ) return NULL;

    ldata->head = NULL;
    ldata->top = 0;
    ldata->total = 0;
    ldata->knobsize = 0.0f;
    ldata->visible = parent->h / 12;
    ldata->hover = -1;
    ldata->sel = -1;

    parent->data = ldata;
  }

  litem = malloc(sizeof(struct listitem) + clen + dlen + 2);
  if( !litem ) return NULL;

  litem->content = (char *)&litem[1];
  litem->display = &litem->content[clen+1];

  strcpy( litem->content, content );

  if( clen > maxchars )
  {
    strcpy( litem->display, "..." );
    strcat( &litem->display[3], &content[clen-(maxchars-3)] );
  } else {
    strcpy( litem->display, content );
  }

  ldata->total++;
  if( ldata->total > ldata->visible )
  {
    ldata->knobsize = ((f32)(ldata->visible) * parent->h) / ((f32)ldata->total);
    if( ldata->knobsize < 8.0f ) ldata->knobsize = 8.0f;
  }
  else
  {
    ldata->knobsize = 0.0f;
  }

  fitem = ldata->head;
  previtem = NULL;

  while( fitem )
  {
    if( strcasecmp(litem->display, fitem->display) < 0 ) break;
    previtem = fitem;
    fitem = fitem->next;
  }

  litem->next = fitem;
  litem->prev = previtem;

  if( fitem ) fitem->prev = litem;
  if( previtem )
    previtem->next = litem;
  else
    ldata->head = litem;

  return litem;
}

void free_list( struct sp_button *parent )
{
  struct listitem *litem, *nitem;

  if (!parent->data) return;

  litem = ((struct listdata *)parent->data)->head;
  while (litem)
  {
    nitem = litem->next;
    free(litem);
    litem = nitem;
  }

  ((struct listdata *)parent->data)->head = NULL;
  ((struct listdata *)parent->data)->total = 0;
  ((struct listdata *)parent->data)->knobsize = 0.0f;
  ((struct listdata *)parent->data)->hover = -1;
  ((struct listdata *)parent->data)->sel = -1;
}

int scan_dir( struct sp_button *listobject, char *path )
{
  DIR *dp = NULL;
  struct dirent *dent;
//  struct stat st;

  free_list( listobject );

  dp = opendir( path );
  if( !dp ) { return 0; }

  while( ( dent = readdir( dp ) ) != NULL )
  {
    alloc_listitem( listobject, dent->d_name );
  }

  closedir( dp );
  return 1;
}

// Initialise the application
int init( void )
{
  int i, j;

  for( i=0; settingspanels[i].title; i++ )
  {
    struct sp_button *spb = settingspanels[i].btns;
    for( j=0; spb[j].str; j++ )
    {
      spb[j].data = NULL;
    }
  }

  if( !kbd_init() )
    return 0;

  // Allocate our emulator state structure
  oric = malloc( sizeof( struct machine ) );
  if( !oric ) return 0;

  memset( oric, 0, sizeof( struct machine ) );

  init_defaults( oric );

  fatInitDefault();

  // Initialise the audio subsystem
  if( !audio_init( oric ) ) return 0;

  // Initialise the rendering subsystem
  if( !render_init( oric ) ) return 0;

  // Initialise the WiiMotes
  WPAD_Init();
  WPAD_SetDataFormat( 0, WPAD_FMT_BTNS_ACC_IR );
  WPAD_SetVRes( 0, 640, 480 );

  set_buttons( oric );

  init_machine( oric, oric->type );

  // Lets go!
  oric->emustate = ES_RUNNING;

  emulate_frame( oric );
  render( oric );
  VIDEO_SetBlack( FALSE );

//  setpanel( oric, SP_VIDEO );
//  setemustate( oric, ES_SETTINGS );

  // Lets do this!
  return 1;
}

// Shut down the application
void shut( void )
{
  int i, j;

  // Shut down the rendering subsystem
  render_shut( oric );

  // Shut down the audio subsystem
  audio_shut( oric );

  for( i=0; settingspanels[i].title; i++ )
  {
    struct sp_button *spb = settingspanels[i].btns;
    for( j=0; spb[j].str; j++ )
    {
      switch( spb[j].type )
      {
        case BTYP_LIST: free_list( &spb[j] ); break;
      }
    }
  }

  // Free the oric!
  if( oric ) free( oric );
}

void refresh_settings( struct machine *oric )
{
  if(( oric->type != oric->settype ) ||
     ( oric->drivetype != oric->setdrivetype))
  {
    oric->drivetype = oric->setdrivetype;
    init_machine( oric, oric->settype );
  }
}

int do_frame_running( void )
{
  int done = 0;

  if( oric->inputs.wpaddown & WPAD_BUTTON_HOME )
  {
    setpanel( oric, SP_MAIN );
    setemustate( oric, ES_SETTINGS );
  }

  if( oric->inputs.wpaddown & WPAD_BUTTON_PLUS )
    render_togglekbd( oric );

  kbd_handler( oric );

  if( ( oric->disp.is50hz ) && ( !oric->ula.vid_is50hz ) )
  {
    // Wii video is 50hz, Oric video is 60hz
    // Emulate 6 oric frames for every 5 wii frames
    oric->disp.framecount = (oric->disp.framecount+1)%5;
    if( oric->disp.framecount == 0 ) emulate_frame( oric );
    emulate_frame( oric );
  } else if( ( !oric->disp.is50hz ) && ( oric->ula.vid_is50hz ) ) {
    // Wii video is 60hz, Oric video is 50hz
    // Emulate 5 oric frames in every 6 wii frames
    oric->disp.framecount = (oric->disp.framecount+1)%6;
    if( oric->disp.framecount != 5 ) emulate_frame( oric );
  } else {
    // Wii video and Oric video match
    emulate_frame( oric );
  }

  return done;
}

/* Handle a new press over a button */
void button_press( void )
{
  int i;
  struct sp_button *spb;

  if( !oric->disp.ir.smooth_valid )
    return;

  for( i=0; oric->disp.currpanel->btns[i].w; i++ )
  {
    spb = &oric->disp.currpanel->btns[i];
    if( ( oric->disp.ir.sx >= spb->x+60 ) &&
        ( oric->disp.ir.sx < spb->x+spb->w+60 ) &&
        ( oric->disp.ir.sy >= spb->y+40 ) &&
        ( oric->disp.ir.sy < spb->y+40+spb->h ) )
      break;
  }

  if( !oric->disp.currpanel->btns[i].w )
    return;

  /* This button becomes the active button */
  oric->disp.currbutton = i;

  spb = &oric->disp.currpanel->btns[oric->disp.currbutton];

  /* Perform any button-type specific activation */
  switch( spb->type )
  {
    case BTYP_LIST:
      {
        struct listdata *data = (struct listdata *)spb->data;
        if (!data) break;

        /* If the list is not in any special state... */
        if( spb->state == 0 )
        {
          /* Check if they clicked over the list, or the dragbar */
          spb->state = (oric->disp.ir.sx < (spb->x+28+spb->w)) ? 1 : 2;

          /* If they clicked over the list, select the item being hovered over */
          if( spb->state == 1 )
          {
            data->sel = data->hover;
          }
        }
      }
      break;

    case BTYP_RADIO:
      /* Work out which option they clicked over */
      spb->state = (oric->disp.ir.sy-(spb->y+40)) / 24; 

      /* Perform action based on the button ID */
      switch( spb->id )
      {
        case BTN_SYSTEM:
          oric->settype = spb->state;
          break;
        
        case BTN_DISK:
          oric->setdrivetype = spb->state;
          if (oric->drivetype != oric->setdrivetype)
          {
            int i;
            for (i=0; i<MAX_DRIVES; i++)
              disk_eject(oric, i);
          }
          break;
      }
      break;

    case BTYP_CHECKBOX:
      /* Toggle checked state */
      spb->state = !spb->state;

      /* Perform action based on the button ID */
      switch( spb->id )
      {
        case BTN_VIDEOSTRETCH:
          oric->odisp.hstretch = spb->state;
          render_set_displayzobj( oric, 10 );
          break;
        
        case BTN_VIDEOLINEAR:
          oric->odisp.linearscale = spb->state;
          break;
        
        case BTN_FILEOPTION:
          switch( oric->disp.currfilereq )
          {
            case FR_TAPE:
              oric->tapeautorun = spb->state;
              break;
          }
          break;
      }
      break;

    case BTYP_BUTTON:
      /* Buttons can only do one things... be pressed! */
      spb->state = 1;
      break;
  }
}

/* Held over a button */
void button_hold( void )
{
  struct sp_button *spb;

  spb = &oric->disp.currpanel->btns[oric->disp.currbutton];

  switch( spb->type )
  {
    case BTYP_LIST:
      {
        struct listdata *data = (struct listdata *)spb->data;
        if (!data) break;

        switch( spb->state )
        {
          case 1:
            /* Hold over the list itself */
            break;

          case 2:
            /* Hold over the knob */
            if( data->knobsize != 0.0f )
            {
              /* Calculate the list position */
              int newtop = ((oric->disp.ir.sy - (spb->y+40)) * data->total) / spb->h;

              if( newtop > (data->total-data->visible))
              {
                newtop = data->total-data->visible;
              }

              if( newtop != data->top )
              {
                data->top = newtop;
                refreshpanel( oric );
              }
            }
            break;
        }
      }
      break;

    case BTYP_BUTTON:
      if( ( !oric->disp.ir.smooth_valid ) ||
          ( oric->disp.ir.sx < spb->x+60 ) ||
          ( oric->disp.ir.sx >= spb->x+spb->w+60 ) ||
          ( oric->disp.ir.sy < spb->y+40 ) ||
          ( oric->disp.ir.sy >= spb->y+64 ) )
        spb->state = 0;
      else
        spb->state = 1;
      break;
  }
}

int button_release( void )
{
  int done = 0;
  int i;
  struct sp_button *spb;

  if( oric->disp.currbutton == -1 )
    return done;

  spb = &oric->disp.currpanel->btns[oric->disp.currbutton];

  switch( spb->type )
  {
    case BTYP_LIST:
      {
        struct listdata *data = (struct listdata *)spb->data;
        struct listitem *item = data->head;

        if( data )
        {
          if( spb->state == 1 )
          {
            if( ( data->sel != -1 ) && ( data->hover == data->sel ) )
            {
              switch( oric->disp.currfilereq )
              {
                case FR_TAPE:
                  // Try and load the tape image
                  for( i=0; i<data->sel; i++)
                  {
                    if( !item ) break;
                    item = item->next;
                  }

                  if( i != data->sel ) break;

                  tape_make_fname( item->content );
                  tape_load_tap( oric, tapefile );

                  if( oric->tapeautorun )
                  {
                    init_machine( oric, oric->type );
                    kbd_queuekeys( &oric->ay, "CLOAD\"\"\r" );
                    setemustate( oric, ES_RUNNING );
                  } else {
                    setpanel( oric, SP_MAIN );
                  }
                  break;
                
                case FR_DISK0:
                case FR_DISK1:
                case FR_DISK2:
                case FR_DISK3:
                  // Try and load the disk image
                  for (i=0; i<data->sel; i++)
                  {
                    if (!item) break;
                    item = item->next;
                  }

                  if( i != data->sel ) break;
                  disk_make_fname( item->content );
                  if ( !diskimage_load( oric, diskfile, oric->disp.currfilereq-FR_DISK0 ) )
                    break;

                  if (oric->drivetype == DRV_NONE)
                  {
                    oric->setdrivetype = oric->drivetype = DRV_MICRODISC;
                    init_machine( oric, oric->type );
                    setemustate( oric, ES_RUNNING );
                  }
                  else
                  {
                    setpanel( oric, SP_MAIN );
                  }
                  break;
              }
            }
          }
          data->hover = -1;
          data->sel = -1;
        }

        spb->state = 0;
      }
      break;

    case BTYP_BUTTON:
      if( spb->state )
      {
        int ok;

        switch( spb->id )
        {
          case BTN_CLOSESETTINGS:
            setemustate( oric, ES_RUNNING );
            break;

          case BTN_QUIT:
            done = 1;
            break;
          
          case BTN_INSERTTAPE:
            settingspanels[SP_FILEREQ].title         = "Insert Tape";
            settingspanels[SP_FILEREQ].btns[1].type  = BTYP_CHECKBOX;
            settingspanels[SP_FILEREQ].btns[1].str   = "Reset and CLOAD";
            settingspanels[SP_FILEREQ].btns[1].state = oric->tapeautorun;
            ok = scan_dir( &settingspanels[SP_FILEREQ].btns[0], tapepath );
            if( !ok )
            {
              strcpy( tapepath, "/" );
              ok = scan_dir( &settingspanels[SP_FILEREQ].btns[0], tapepath );
            }

            if (ok)
            {
              oric->disp.currfilereq = FR_TAPE;
              setpanel( oric, SP_FILEREQ );
            }
            break;

          case BTN_INSERTDISK0:
          case BTN_INSERTDISK1:
          case BTN_INSERTDISK2:
          case BTN_INSERTDISK3:
            settingspanels[SP_FILEREQ].title        = "Insert Disk 0";
            settingspanels[SP_FILEREQ].btns[1].type = BTYP_HIDDEN;
            settingspanels[SP_FILEREQ].btns[1].str  = "";
            ok = scan_dir( &settingspanels[SP_FILEREQ].btns[0], diskpath );
            if( !ok )
            {
              strcpy( diskpath, "/" );
              ok = scan_dir( &settingspanels[SP_FILEREQ].btns[0], diskpath );
            }

            if (ok)
            {
              oric->disp.currfilereq = FR_DISK0+(spb->id-BTN_INSERTDISK0);
              setpanel( oric, SP_FILEREQ );
            }
            break;
          
          case BTN_HWSETTINGS:
            setpanel( oric, SP_HW );
            break;

          case BTN_VIDEOSETTINGS:
            setpanel( oric, SP_VIDEO );
            break;
          
          case BTN_MAINSETTINGS:
            refresh_settings( oric );
            setpanel( oric, SP_MAIN );
            break;
          
          case BTN_RESET:
            init_machine( oric, oric->type );
            setemustate( oric, ES_RUNNING );
            break;

          case BTN_LOADKEYJOY1:
          case BTN_LOADKEYJOY2:
          case BTN_LOADKEYJOY3:
          case BTN_LOADKEYJOY4:
          case BTN_LOADKEYJOY5:
//            LoadKeys(spb->id -  BTN_LOADKEYJOY1);
            break;

          case BTN_SAVEKEYJOY1:
          case BTN_SAVEKEYJOY2:
          case BTN_SAVEKEYJOY3:
          case BTN_SAVEKEYJOY4:
//            SaveKeys(spb->id - BTN_SAVEKEYJOY1);
            break;

          case BTN_CONFIG_JOYSTICK_KEYS:
            setpanel( oric, SP_JOYKEY );
            break;
        }
        
        spb->state = 0;
      }
      break;
  }

  oric->disp.currbutton = -1;

  return done;
}

int do_frame_settings( void )
{
  int done = 0;

  if( oric->inputs.wpaddown & WPAD_BUTTON_HOME )
  {
    refresh_settings( oric );
    setemustate( oric, ES_RUNNING );
    return done;
  }

  if( oric->inputs.wpadheld & WPAD_BUTTON_DOWN )
  {
    int oldtop;

    switch( oric->disp.currpanelid )
    {
      case SP_FILEREQ:
        {
          struct listdata *data = (struct listdata *)oric->disp.currpanel->btns[0].data;

          if( !data ) break;

          oldtop = data->top;

          data->top++;
          if( data->top > (data->total-data->visible) ) data->top = data->total-data->visible;
          if( data->top < 0 ) data->top = 0;
          
          if( data->top != oldtop )
            refreshpanel( oric );
        }
        break;
    }
  }

  if( oric->inputs.wpadheld & WPAD_BUTTON_UP )
  {
    switch( oric->disp.currpanelid )
    {
      case SP_FILEREQ:
        {
          struct listdata *data = (struct listdata *)oric->disp.currpanel->btns[0].data;

          if( !data ) break;

          if( data->top > 0 )
          {
            data->top--;
            refreshpanel( oric );
          }
        }
        break;
    }
  }

  if( oric->inputs.wpadheld & WPAD_BUTTON_A )
  {
    if( oric->disp.currbutton == -1 )
    {
      button_press();
    } else {
      button_hold();
    }
  } else {
    done |= button_release();
  }

  return done;
}

// Entry point
int main( int argc, char **argv )
{
  if( init() )
  {
    int done = 0;

    while( !done )
    {
      audio_mix( oric );

      WPAD_IR( 0, &oric->disp.ir );
      WPAD_ScanPads();

      oric->inputs.wpadheld = WPAD_ButtonsHeld( 0 );
      oric->inputs.wpaddown = WPAD_ButtonsDown( 0 );

      switch( oric->emustate )
      {
        case ES_RUNNING:
          done |= do_frame_running();
          break;
        
        case ES_SETTINGS:
          done |= do_frame_settings();
          break;
      }

      render( oric );
    }
  }
  shut();

  return 0;
}
