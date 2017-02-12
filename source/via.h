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

enum
{
  VIA_MAIN = 0,
  VIA_TELESTRAT
};

struct via
{
  // CPU accessible registers
  u8  ifr;
  u8  irb, orb, irbl;
  u8  ira, ora, iral;
  u8  ddra;
  u8  ddrb;
  u8  t1l_l;
  u8  t1l_h;
  u16 t1c;
  u8  t2l_l;
  u8  t2l_h;  
  u16 t2c;
  u8  sr;
  u8  acr;
  u8  pcr;
  u8  ier;

  // Pins and ports
  u8  ca1, ca2;
  u8  cb1, cb2;
  u8  srcount;
  u8  t1reload, t2reload;
  s16 srtime;

  // Internal state stuff
  int t1run, t2run;
  int ca2pulse, cb2pulse;
  int srtrigger;

  void (*w_iorb)(struct via *,unsigned char);
  void (*w_iora)(struct via *);
  void (*w_iora2)(struct via *);
  void (*w_ddra)(struct via *);
  void (*w_ddrb)(struct via *);
  void (*w_pcr)(struct via *);
  void (*w_ca2ext)(struct via *);
  void (*w_cb2ext)(struct via *);
  void (*r_iora)(struct via *);
  void (*r_iora2)(struct via *);
  void (*r_iorb)(struct via *);
  void (*ca2pulsed)(struct via *);
  void (*cb2pulsed)(struct via *);
  void (*cb2shifted)(struct via *);
  void (*orbchange)(struct via *);

  unsigned char (*read_port_a)(struct via *);
  unsigned char (*read_port_b)(struct via *);
  void (*write_port_a)(struct via *v, unsigned char, unsigned char);
  void (*write_port_b)(struct via *v, unsigned char, unsigned char);

  struct machine *oric;
  int irqbit;
};

#define VIA_IORB   0
#define VIA_IORA   1
#define VIA_DDRB   2
#define VIA_DDRA   3
#define VIA_T1C_L  4
#define VIA_T1C_H  5
#define VIA_T1L_L  6
#define VIA_T1L_H  7
#define VIA_T2C_L  8
#define VIA_T2C_H  9
#define VIA_SR    10
#define VIA_ACR   11
#define VIA_PCR   12
#define VIA_IFR   13
#define VIA_IER   14
#define VIA_IORA2 15

#define PCRB_CA1CON 0
#define PCRF_CA1CON (1<<PCRB_CA1CON)
#define PCRF_CA2CON (0x0e)
#define PCRB_CB1CON 4
#define PCRF_CB1CON (1<<PCRB_CB1CON)
#define PCRF_CB2CON (0xe0)

#define ACRB_PALATCH 0
#define ACRF_PALATCH (1<<ACRB_PALATCH)
#define ACRB_PBLATCH 1
#define ACRF_PBLATCH (1<<ACRB_PBLATCH)
#define ACRF_SRCON   (0x1c)
#define ACRB_T2CON   5
#define ACRF_T2CON   (1<<ACRB_T2CON)
#define ACRF_T1CON   (0xc0)

#define VIRQB_CA2 0
#define VIRQF_CA2 (1<<VIRQB_CA2)
#define VIRQB_CA1 1
#define VIRQF_CA1 (1<<VIRQB_CA1)
#define VIRQB_SR 2
#define VIRQF_SR (1<<VIRQB_SR)
#define VIRQB_CB2 3
#define VIRQF_CB2 (1<<VIRQB_CB2)
#define VIRQB_CB1 4
#define VIRQF_CB1 (1<<VIRQB_CB1)
#define VIRQB_T2 5
#define VIRQF_T2 (1<<VIRQB_T2)
#define VIRQB_T1 6
#define VIRQF_T1 (1<<VIRQB_T1)


// Init/Reset
void via_init( struct via *v, struct machine *oric, int viatype );

// Move timers on etc.
void via_clock( struct via *v, unsigned int cycles );

// Write VIA from CPU
void via_write( struct via *v, int offset, u8 data );

// Read VIA from CPU
u8 via_read( struct via *v, int offset );

// Write CA1,CA2,CB1,CB2 from external device
// (data is treated as bool)
void via_write_CA1( struct via *v, u8 data );
void via_write_CA2( struct via *v, u8 data );
void via_write_CB1( struct via *v, u8 data );
void via_write_CB2( struct via *v, u8 data );

// Write to IFR from the monitor
void via_mon_write_ifr( struct via *v, u8 data );

// Read without side-effects for monitor
u8 via_mon_read( struct via *v, int offset );
