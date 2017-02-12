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

// Machine types
enum
{
  MACH_ORIC1 = 0,
  MACH_ORIC1_16K,
  MACH_ATMOS,
  MACH_PRAVETZ,
  MACH_TELESTRAT
};

enum
{
  DRV_NONE = 0,
  DRV_MICRODISC,
  DRV_JASMIN,
  DRV_PRAVETZ
};

// Emulation states
enum
{
  ES_SETTINGS = 0,
  ES_RUNNING
};

// Zoomable object IDs
enum
{
  ZO_DISPLAY = 0,
  ZO_KBD,
  ZO_LAST
};

// Scalable object IDs
enum
{
  SO_SETTINGS = 0,
  SO_LAST
};

// Settings panel IDs
enum
{
  SP_MAIN = 0,
  SP_FILEREQ,
  SP_HW,
  SP_VIDEO,
  SP_JOYKEY,
  SP_LAST
};

// Settings panel button types
enum
{
  BTYP_BUTTON = 0,
  BTYP_CHECKBOX,
  BTYP_RADIO,
  BTYP_LIST,
  BTYP_HIDDEN
};

enum
{
  FR_TAPE = 0,
  FR_DISK0,
  FR_DISK1,
  FR_DISK2,
  FR_DISK3
};

// Settings panel button IDs
enum
{
  BTN_MAINSETTINGS=0,

  BTN_INSERTTAPE,
  BTN_INSERTDISK0,
  BTN_INSERTDISK1,
  BTN_INSERTDISK2,
  BTN_INSERTDISK3,

  BTN_OK,
  BTN_FILELIST,
  BTN_FILEOPTION,

  BTN_HWSETTINGS,
  BTN_VIDEOSETTINGS,

  BTN_RESET,

  BTN_CLOSESETTINGS,
  BTN_QUIT,

  BTN_SYSTEM,
  BTN_DISK,

  BTN_VIDEOSTRETCH,
  BTN_VIDEOLINEAR,

  BTN_CONFIG_JOYSTICK_KEYS,
  BTN_SAVEKEYJOY1,
  BTN_SAVEKEYJOY2,
  BTN_SAVEKEYJOY3,
  BTN_SAVEKEYJOY4,
  BTN_LOADKEYJOY1,
  BTN_LOADKEYJOY2,
  BTN_LOADKEYJOY3,
  BTN_LOADKEYJOY4,
  BTN_LOADKEYJOY5
};

// Zoomable object
struct zoomobj
{
  f32 x, y, w, h;
  int a;
  f32 tx, ty, tw, th;
  int ta;
  f32 dx, dy, dw, dh;
  int da;
  int settled;
};

// Scalable object
struct scaleobj
{
  f32 ox, oy;
  f32 x, y, w, h;
  f32 scale, tscale, dscale;
  int a, ta, da;
  int settled;
};

// Settings panel button
struct sp_button
{
  int   type;
  f32   x, y, w;
  int   id;
  int   state;
  char *str;

  // Auto calculated
  f32   h;  // except for list (yay!)

  void *data;
};

struct listdata
{
  struct listitem *head;
  int top, total, visible, hover, sel;
  f32 knobsize;
};

struct listitem
{
  struct listitem *next;
  struct listitem *prev;
  char   *content;
  char   *display;
};

// Settings panel
struct settingspanel
{
  char             *title;
  struct sp_button *btns;
};

// Wii display information
struct wii_display
{
  int w, h;
  int fb;
  int is50hz;
  int framecount;
  void *xfb[2];
  void *gp_fifo;
  Mtx GXmodelView2D;
  GXRModeObj *rmode;
  TPLFile texTPL;
  struct zoomobj  zobj[ZO_LAST];
  struct scaleobj sobj[SO_LAST];
  ir_t ir;
  int pleaseinvtexes;

  u32 pnlrgb[520*400];
  u8 *pnltex;

  u32 kbdinfrgb[160*224];
  u8 *kbdinftex;
  unsigned int kbd_wiimote_show;
  unsigned int kbd_nunchuck_show;
  unsigned int kbd_classic_show;

  int                   currpanelid, currfilereq;
  struct settingspanel *currpanel;
  int                   currbutton;

  int showkbd;
};

// Oric display
struct oric_display
{
  u32  rgb[240*224];   // Oric display in RGBA format
  u8  *tex;             // Wii mangled texture of the above
  int  hstretch, linearscale;
};

struct telebankinfo
{
  unsigned char type;
  unsigned char *ptr;
};

struct wii_inputs
{
    u32 wpadheld, wpaddown;
};

// The state of the emulation
struct machine
{
  int                 type;
  int                 settype;

  struct wii_display  disp;
  struct oric_display odisp;
  struct m6502        cpu;
  struct oric_ula     ula;
  struct ay8912       ay;
  struct via          via;

  struct wii_inputs   inputs;

  struct kbdkey      *keylayout;
  int                 overkey;

  unsigned char       mem[65536];
  unsigned char      *rom;

  int                 cyclesperraster;
  int                 emustate;
  int                 romdis;
  int                 romon;

  int                 vsynchack;
  int                 vsync;

  int                 prclock;
  int                 prclose;
  FILE               *prf;

  int                 rampattern;

  u8                  porta_ay;
  u8                  porta_is_ay;

  struct telebankinfo tele_bank[8];
  struct via          tele_via;
  struct acia         tele_acia;
  int                 tele_currbank;
  unsigned char       tele_banktype;

  int                 tapemotor;
  int                 tapeturbo;
  int                 tapeturbo_forceoff;
  int                 tapeturbo_syncstack;
  int                 tapebit;
  int                 tapedupbytes;
  int                 tapelen;
  int                 tapecount;
  int                 tapeout;
  int                 tapetime;
  int                 tapeparity;
  int                 tapehitend;
  int                 tapehdrend;
  int                 tapedelay;
  int                 rawtape;
  int                 nonrawend;
  u32                 tapeoffs;
  u8                 *tapebuf;
  char                tapename[128];
  int                 tapeautorun;
  int                 tapeautorewind;
  int                 tapeautoinsert;
  char                lasttapefile[128];

  int drivetype, setdrivetype;
  struct wd17xx    wddisk;
  struct microdisc md;
  struct jasmin jasmin;
  char diskname[MAX_DRIVES][32];

  // Filename decoding patch addresses
  int pch_fd_cload_getname_pc;
  int pch_fd_recall_getname_pc;
  int pch_fd_getname_addr;
  int pch_fd_available;

  // Turbo tape patch addresses
  int pch_tt_getsync_pc;
  int pch_tt_getsync_end_pc;
  int pch_tt_getsync_loop_pc;
  int pch_tt_readbyte_pc;
  int pch_tt_readbyte_end_pc;
  int pch_tt_readbyte_storebyte_addr;
  int pch_tt_readbyte_storezero_addr;
  int pch_tt_readbyte_setcarry;
  int pch_tt_available;
};
