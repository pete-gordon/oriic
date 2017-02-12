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

#define AUDIO_BUFFERS 3

// Audio buffer size
#define AUDIO_BUFLEN 8192

// Integer fraction bits to use when mapping
// clock cycles to audio samples
#define FPBITS 10

#define WRITELOG_SIZE (AUDIO_BUFLEN*12)

#define CYCLESPERSECOND (312*64*50)
#define CYCLESPERSAMPLE ((CYCLESPERSECOND<<FPBITS)/48000)

// GI addressing
#define AYBMB_BC1  0
#define AYBMF_BC1  (1<<AYBMB_BC1)
#define AYBMB_BDIR 1
#define AYBMF_BDIR (1<<AYBMB_BDIR)

// 8912 registers
enum
{
  AY_CHA_PER_L = 0,
  AY_CHA_PER_H,
  AY_CHB_PER_L,
  AY_CHB_PER_H,
  AY_CHC_PER_L,
  AY_CHC_PER_H,
  AY_NOISE_PER,
  AY_STATUS,
  AY_CHA_AMP,
  AY_CHB_AMP,
  AY_CHC_AMP,
  AY_ENV_PER_L,
  AY_ENV_PER_H,
  AY_ENV_CYCLE,
  AY_PORT_A,
  NUM_AY_REGS
};

struct aywrite
{
  u32 cycle;
  u8  reg;
  u8  val;
};

struct ay8912
{
  u8              bmode, creg;
  u8              regs[NUM_AY_REGS], eregs[NUM_AY_REGS];
  int             keystates[8], newnoise;
  int             soundon;
  int             tapenoiseon;
  u32             toneper[3], noiseper, envper;
  u16             tonebit[3], noisebit[3], vol[3], newout;
  s32             ct[3], ctn, cte;
  u32             tonepos[3], tonestep[3];
  s32             sign[3], out[3], envpos;
  u8             *envtab;
  struct machine *oric;
  u32             currnoise, rndrack;
  s16             output;
  s16             tapeout;
  u32             ccycle, lastcyc, ccyc;
  u32             keybitdelay, currkeyoffs;

  char           *keyqueue;
  int             keysqueued, kqoffs;

  int             do_logcycle_reset;
  s32             logged;
  u32             logcycle, newlogcycle;
  struct aywrite  writelog[WRITELOG_SIZE];
};

int audio_init( struct machine *oric );
void audio_shut( struct machine *oric );
void audio_mix( struct machine *oric );
void stop_audio(void);
void start_audio(void);

void ay_init( struct ay8912 *ay, struct machine *oric );
void ay_ticktock( struct ay8912 *ay, int cycles );
void ay_update_keybits( struct ay8912 *ay );

void ay_set_bc1( struct ay8912 *ay, u8 state );
void ay_set_bdir( struct ay8912 *ay, u8 state );
void ay_set_bcmode( struct ay8912 *ay, u8 bc1, u8 bdir );
void ay_modeset( struct ay8912 *ay );
