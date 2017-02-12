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
#include "keyboard.h"
#include "disk.h"
#include "oriic.h"
#include "render.h"
#include "textures.h"
#include "textures_tpl.h"

extern u32 rom_basic11b[];
static guVector axis = { 0, 0, 1 };

// Initialise a scalable object
void sobj_init( struct scaleobj *so, f32 ox, f32 oy, f32 x, f32 y, f32 w, f32 h, f32 scale, int a )
{
  so->ox     = ox;
  so->oy     = oy;
  so->x      = x;
  so->y      = y;
  so->w      = w;
  so->h      = h;
  so->scale  = so->tscale = scale;
  so->a      = so->ta     = (a<<8);
  so->dscale = 0.0f;
  so->da     = 0;
}

// Start a new scaled zoom
void sobj_zoom( struct scaleobj *so, f32 scale, int a, f32 frames )
{
  so->tscale  = scale;
  so->ta      = a<<8;
  so->dscale  = (so->tscale-so->scale) / frames;
  so->da      = (so->ta-so->a) / (int)frames;
  so->settled = 0;
}

// Initialise a zoomable object
void zobj_init( struct zoomobj *zo, f32 x, f32 y, f32 w, f32 h, int a )
{
  zo->x = zo->tx = x;
  zo->y = zo->ty = y;
  zo->w = zo->tw = w;
  zo->h = zo->th = h;
  zo->a = zo->ta = (a<<8);
  zo->dx = 0.0f;
  zo->dy = 0.0f;
  zo->dw = 0.0f;
  zo->dh = 0.0f;
  zo->da = 0;
  zo->settled = 1;
}

// Start a new zoom
void zobj_zoom( struct zoomobj *zo, f32 x, f32 y, f32 w, f32 h, int a, f32 frames )
{
  zo->tx = x;
  zo->ty = y;
  zo->tw = w;
  zo->th = h;
  zo->ta = a<<8;
  zo->dx = (zo->tx-zo->x) / frames;
  zo->dy = (zo->ty-zo->y) / frames;
  zo->dw = (zo->tw-zo->w) / frames;
  zo->dh = (zo->th-zo->h) / frames;
  zo->da = (zo->ta-zo->a) / (int)frames;
  zo->settled = 0;
}

static void zoomfloat( f32 *curr, f32 targ, f32 *delta )
{
  if( ( (*curr) == targ ) || ( (*delta) == 0 ) )
    return;

  if( (*curr) < targ )
  {
    *curr += *delta;
    if( (*curr) >= targ )
    {
      *curr = targ;
      *delta = 0.0f;
    }
    return;
  }

  *curr += *delta;
  if( (*curr) <= targ )
  {
    *curr = targ;
    *delta = 0.0f;
  }
}

static void zoomfixed( int *curr, int targ, int *delta )
{
  if( ( (*curr) == targ ) || ( (*delta) == 0 ) )
    return;

  if( (*curr) < targ )
  {
    *curr += *delta;
    if( (*curr) >= targ )
    {
      *curr = targ;
      *delta = 0;
    }
    return;
  }

  *curr += *delta;
  if( (*curr) <= targ )
  {
    *curr = targ;
    *delta = 0;
  }
}

void sobj_update( struct scaleobj *so )
{
  zoomfloat( &so->scale, so->tscale, &so->dscale );
  zoomfixed( &so->a,     so->ta,     &so->da     );

  so->settled = ((so->scale==so->tscale)&&(so->a==so->ta));
}

void zobj_update( struct zoomobj *zo )
{
  zoomfloat( &zo->x, zo->tx, &zo->dx );
  zoomfloat( &zo->y, zo->ty, &zo->dy );
  zoomfloat( &zo->w, zo->tw, &zo->dw );
  zoomfloat( &zo->h, zo->th, &zo->dh );
  zoomfixed( &zo->a, zo->ta, &zo->da );

  zo->settled = ((zo->x==zo->tx)&&(zo->y==zo->ty)&&(zo->w==zo->tw)&&(zo->h==zo->th)&&(zo->a==zo->ta));
}

// Print a string in the oric font into an RGBA buffer
void render_printstr( u32 *rgbabuf, int rgbaw, int x, int y, char *str, u32 fg )
{
  int i, c, cx, cy;
  u8 *fptr, chdat;
  u32 *dptr;

  dptr = &rgbabuf[y*rgbaw+x];
  for( i=0; str[i]; i++ )
  {
    if (str[i] == '|') break;
    c = str[i]-32;
    if( c < 0 ) c = 0;

    fptr = ((u8 *)rom_basic11b)+0x3c78+(c*8);
    for( cy=0; cy<8; cy++ )
    {
      chdat = *(fptr++);
      for( cx=0; cx<6; cx++, dptr++ )
      {
        if( chdat & 0x20 ) *dptr = fg;
        chdat <<= 1;
      }
      dptr += rgbaw-6;
    }

    x += 6;
    dptr = &rgbabuf[y*rgbaw+x];
  }
}

void render_dblprintstr( u32 *rgbabuf, int rgbaw, int x, int y, char *str, u32 fg )
{
  int i, c, cx, cy;
  u8 *fptr, chdat;
  u32 *dptr;

  dptr = &rgbabuf[y*rgbaw+x];
  for( i=0; str[i]; i++ )
  {
    c = str[i]-32;
    if( c < 0 ) c = 0;

    fptr = ((u8 *)rom_basic11b)+0x3c78+(c*8);
    for( cy=0; cy<8; cy++ )
    {
      chdat = *(fptr++);
      for( cx=0; cx<6; cx++, dptr+=2 )
      {
        if( chdat & 0x20 )
        {
          dptr[0] = fg;
          dptr[1] = fg;
          dptr[rgbaw] = fg;
          dptr[rgbaw+1] = fg;
        }
        chdat <<= 1;
      }
      dptr += rgbaw*2-12;
    }

    x += 12;
    dptr = &rgbabuf[y*rgbaw+x];
  }
}

// Print a string in the oric font into an RGBA buffer, including a background colour
void render_printstr_solid( u32 *rgbabuf, int rgbaw, int x, int y, char *str, u32 fg, u32 bg )
{
  int i, c, cx, cy;
  u8 *fptr, chdat;
  u32 *dptr;

  dptr = &rgbabuf[y*rgbaw+x];
  for( i=0; str[i]; i++ )
  {
    c = str[i]-32;
    if( c < 0 ) c = 0;

    fptr = ((u8 *)rom_basic11b)+0x3c78+(c*8);
    for( cy=0; cy<8; cy++ )
    {
      chdat = *(fptr++);
      for( cx=0; cx<6; cx++ )
      {
        *(dptr++) = (chdat&0x20) ? fg : bg;
        chdat <<= 1;
      }
      dptr += rgbaw-6;
    }

    x += 6;
    dptr = &rgbabuf[y*rgbaw+x];
  }
}

// Set up to use this texture
void render_bindtexture( GXTexObj *texObj, u8 *texp, int w, int h, int mode )
{
  GX_InitTexObj( texObj, texp, w, h, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE );
  GX_InitTexObjLOD( texObj, mode, mode, 0.0f, 0.0f, 0.0f, 0, 0, GX_ANISO_1 );
  GX_LoadTexObj( texObj, GX_TEXMAP0 );

  GX_SetTevOp( GX_TEVSTAGE0, GX_MODULATE );
  GX_SetVtxDesc( GX_VA_TEX0, GX_DIRECT );
}

// Finished with this texture
void render_unbindtexture( void )
{
  GX_SetTevOp( GX_TEVSTAGE0, GX_PASSCLR );
  GX_SetVtxDesc( GX_VA_TEX0, GX_NONE );
}

// Finish the current frame and swap the buffers
void render_flip( struct machine *oric )
{
  GX_DrawDone();
	
  oric->disp.fb ^= 1;
  GX_SetZMode( GX_TRUE, GX_LEQUAL, GX_TRUE );
  GX_SetColorUpdate( GX_TRUE );
  GX_CopyDisp( oric->disp.xfb[oric->disp.fb], GX_TRUE );
  VIDEO_SetNextFramebuffer( oric->disp.xfb[oric->disp.fb] );
  VIDEO_Flush();
}

// Convert a normal 24bit RGB texture to the Wii GPU format
void render_texturemangle( int w, int h, u8 *src, u8 *dest )
{
  int x, y, qw, qh, rw;

  qw = w/4;
  qh = h/4;
  rw = w*4;

  for (y = 0; y < qh; y++)
    for (x = 0; x < qw; x++)
    {
      int blockbase = (y * qw + x) * 8;

      u64 fieldA = *((u64 *)(&src[(y*4)*rw+x*16]));
      u64 fieldB = *((u64 *)(&src[(y*4)*rw+x*16+8]));

      ((u64 *) dest)[blockbase] = 
        ((fieldA & 0xFF00000000ULL) << 24) | ((fieldA & 0xFF00000000000000ULL) >> 8) | 
        ((fieldA & 0xFFULL) << 40) | ((fieldA & 0xFF000000ULL) << 8) | 
        ((fieldB & 0xFF00000000ULL) >> 8) | ((fieldB & 0xFF00000000000000ULL) >> 40) | 
        ((fieldB & 0xFFULL) << 8) | ((fieldB & 0xFF000000ULL) >> 24);
      ((u64 *) dest)[blockbase+4] =
        ((fieldA & 0xFFFF0000000000ULL) << 8) | ((fieldA & 0xFFFF00ULL) << 24) |
        ((fieldB & 0xFFFF0000000000ULL) >> 24) | ((fieldB & 0xFFFF00ULL) >> 8);

      fieldA = *((u64 *)(&src[(y*4+1)*rw+x*16]));
      fieldB = *((u64 *)(&src[(y*4+1)*rw+x*16+8]));

      ((u64 *) dest)[blockbase+1] = 
        ((fieldA & 0xFF00000000ULL) << 24) | ((fieldA & 0xFF00000000000000ULL) >> 8) | 
        ((fieldA & 0xFFULL) << 40) | ((fieldA & 0xFF000000ULL) << 8) | 
        ((fieldB & 0xFF00000000ULL) >> 8) | ((fieldB & 0xFF00000000000000ULL) >> 40) | 
        ((fieldB & 0xFFULL) << 8) | ((fieldB & 0xFF000000ULL) >> 24);
      ((u64 *) dest)[blockbase+5] =
        ((fieldA & 0xFFFF0000000000ULL) << 8) | ((fieldA & 0xFFFF00ULL) << 24) |
        ((fieldB & 0xFFFF0000000000ULL) >> 24) | ((fieldB & 0xFFFF00ULL) >> 8);

      fieldA = *((u64 *)(&src[(y*4+2)*rw+x*16]));
      fieldB = *((u64 *)(&src[(y*4+2)*rw+x*16+8]));

      ((u64 *) dest)[blockbase+2] = 
        ((fieldA & 0xFF00000000ULL) << 24) | ((fieldA & 0xFF00000000000000ULL) >> 8) | 
        ((fieldA & 0xFFULL) << 40) | ((fieldA & 0xFF000000ULL) << 8) | 
        ((fieldB & 0xFF00000000ULL) >> 8) | ((fieldB & 0xFF00000000000000ULL) >> 40) | 
        ((fieldB & 0xFFULL) << 8) | ((fieldB & 0xFF000000ULL) >> 24);
      ((u64 *) dest)[blockbase+6] =
        ((fieldA & 0xFFFF0000000000ULL) << 8) | ((fieldA & 0xFFFF00ULL) << 24) |
        ((fieldB & 0xFFFF0000000000ULL) >> 24) | ((fieldB & 0xFFFF00ULL) >> 8);

      fieldA = *((u64 *)(&src[(y*4+3)*rw+x*16]));
      fieldB = *((u64 *)(&src[(y*4+3)*rw+x*16+8]));

      ((u64 *) dest)[blockbase+3] = 
        ((fieldA & 0xFF00000000ULL) << 24) | ((fieldA & 0xFF00000000000000ULL) >> 8) | 
        ((fieldA & 0xFFULL) << 40) | ((fieldA & 0xFF000000ULL) << 8) | 
        ((fieldB & 0xFF00000000ULL) >> 8) | ((fieldB & 0xFF00000000000000ULL) >> 40) | 
        ((fieldB & 0xFFULL) << 8) | ((fieldB & 0xFF000000ULL) >> 24);
      ((u64 *) dest)[blockbase+7] =
        ((fieldA & 0xFFFF0000000000ULL) << 8) | ((fieldA & 0xFFFF00ULL) << 24) |
        ((fieldB & 0xFFFF0000000000ULL) >> 24) | ((fieldB & 0xFFFF00ULL) >> 8);
    }
}

void render_outline( f32 x, f32 y, f32 w, f32 h, u8 r, u8 g, u8 b, u8 a )
{
  GX_Begin( GX_LINESTRIP, GX_VTXFMT0, 5 );
    GX_Position3f32(  x,  y, 1.0f );
    GX_Color4u8( r, g, b, a );
    GX_Position3f32(  x+w,  y, 1.0f );
    GX_Color4u8( r, g, b, a );
    GX_Position3f32(  x+w,  y+h, 1.0f );
    GX_Color4u8( r, g, b, a );
    GX_Position3f32(  x,  y+h, 1.0f );
    GX_Color4u8( r, g, b, a );
    GX_Position3f32(  x,  y, 1.0f );
    GX_Color4u8( r, g, b, a );
  GX_End();
}

// Render the keyboard!
void render_kbd( struct machine *oric )
{
  GXTexObj texObj;
  struct zoomobj *zo = &oric->disp.zobj[ZO_KBD];
  struct kbdkey *k;
  int i, tid=0;

  switch( oric->type )
  {
    case MACH_ORIC1:   tid = gfx_oric1kbd;   break;
    case MACH_PRAVETZ: tid = gfx_pravetzkbd; break;
    default:           tid = gfx_atmoskbd;   break;
  }

  TPL_GetTexture( &oric->disp.texTPL, tid, &texObj );
  GX_LoadTexObj( &texObj, GX_TEXMAP0 );

  GX_SetTevOp( GX_TEVSTAGE0, GX_MODULATE );
  GX_SetVtxDesc( GX_VA_TEX0, GX_DIRECT );

  GX_LoadPosMtxImm( oric->disp.GXmodelView2D, GX_PNMTX0 );

  GX_Begin( GX_QUADS, GX_VTXFMT0, 4 );
    GX_Position3f32(       zo->x,       zo->y, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, zo->a>>8 ); GX_TexCoord2f32( 0.0f, 0.0f );
    GX_Position3f32( zo->x+zo->w,       zo->y, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, zo->a>>8 ); GX_TexCoord2f32( 1.0f, 0.0f );
    GX_Position3f32( zo->x+zo->w, zo->y+zo->h, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, zo->a>>8 ); GX_TexCoord2f32( 1.0f, 1.0f );
    GX_Position3f32(       zo->x, zo->y+zo->h, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, zo->a>>8 ); GX_TexCoord2f32( 0.0f, 1.0f );
  GX_End();

  render_unbindtexture();

  if( oric->emustate != ES_RUNNING ) return;

  GX_SetLineWidth( 14, GX_TO_ZERO );

  oric->overkey = -1;
  if( ( zo->settled ) && ( oric->disp.ir.smooth_valid ) )
  {
    for( i=0; oric->keylayout[i].x!=-1; i++ )
    {
      k = &oric->keylayout[i];
      if( ( oric->disp.ir.sx >= (zo->x+k->x) ) &&
          ( oric->disp.ir.sx < (zo->x+k->x+k->w) ) &&
          ( oric->disp.ir.sy >= (zo->y+k->y) ) &&
          ( oric->disp.ir.sy < (zo->y+k->y+k->h) ) )
      {
        render_outline( zo->x+k->x, zo->y+k->y, k->w, k->h, 0xff, 0xff, 0xff, 0xff );
        oric->overkey = i;
      }
    }
  }

  for( i=0; oric->keylayout[i].x!=-1; i++ )
  {
    k = &oric->keylayout[i];

    if( k->highlight > 0 )
    {
      GX_Begin( GX_QUADS, GX_VTXFMT0, 4 );
        GX_Position3f32(      zo->x+k->x,      zo->y+k->y, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, oric->keylayout[i].highlight );
        GX_Position3f32( zo->x+k->x+k->w,      zo->y+k->y, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, oric->keylayout[i].highlight );
        GX_Position3f32( zo->x+k->x+k->w, zo->y+k->y+k->h, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, oric->keylayout[i].highlight );
        GX_Position3f32(      zo->x+k->x, zo->y+k->y+k->h, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, oric->keylayout[i].highlight );
      GX_End();

      if( k->highlightfade )
      {
        k->highlight -= 4;
        if( k->highlight < 0 )
        {
          k->highlight = 0;
          k->highlightfade = 0;
        }
      }
    }
  }
  if( zo->settled )
  {
    int i;
    unsigned int b;
    f32 x, y, w, h, m;

    render_bindtexture( &texObj, oric->disp.kbdinftex, 160, 224, GX_LINEAR );
    GX_Begin( GX_QUADS, GX_VTXFMT0, 4 );
      GX_Position3f32(  20.0f,  16.0f, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, 0xff ); GX_TexCoord2f32( 0.0f, 0.0f );
      GX_Position3f32( 180.0f,  16.0f, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, 0xff ); GX_TexCoord2f32( 1.0f, 0.0f );
      GX_Position3f32( 180.0f, 240.0f, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, 0xff ); GX_TexCoord2f32( 1.0f, 1.0f );
      GX_Position3f32(  20.0f, 240.0f, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, 0xff ); GX_TexCoord2f32( 0.0f, 1.0f );
    GX_End();

    x = 20.0f;
    m = 48.0f;
    for( i=0, b=0x1; kbd_wiimote_images[i]!=-2; i+=3, b <<= 1 )
    {
      if( kbd_wiimote_images[i] == -1 ) continue;
      w = (f32)kbd_wiimote_images[i+1];
      h = (f32)kbd_wiimote_images[i+2];

      if ((x+w) > 200.0f)
      {
        x = 20.0f;
        m += 48.0f;
      }

      y = m-(h/2);
      if( oric->disp.kbd_wiimote_show&b )
      {
        TPL_GetTexture( &oric->disp.texTPL, kbd_wiimote_images[i], &texObj );
        GX_LoadTexObj( &texObj, GX_TEXMAP0 );
        GX_Begin( GX_QUADS, GX_VTXFMT0, 4 );
          GX_Position3f32(   x, y,   0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, 0xff ); GX_TexCoord2f32( 0.0f, 0.0f );
          GX_Position3f32( x+w, y,   0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, 0xff ); GX_TexCoord2f32( 1.0f, 0.0f );
          GX_Position3f32( x+w, y+h, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, 0xff ); GX_TexCoord2f32( 1.0f, 1.0f );
          GX_Position3f32(   x, y+h, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, 0xff ); GX_TexCoord2f32( 0.0f, 1.0f );
        GX_End();

        x += w;
      }
    }
    render_unbindtexture();
  }
}
    
// Render the settings panel!
void render_settings_panel( struct machine *oric )
{
  GXTexObj texObj;
  Mtx m, mv;
  struct scaleobj *so = &oric->disp.sobj[SO_SETTINGS];
  struct settingspanel *sp = oric->disp.currpanel;

  TPL_GetTexture( &oric->disp.texTPL, gfx_settingsbg, &texObj );
  GX_LoadTexObj( &texObj, GX_TEXMAP0 );

  GX_SetTevOp( GX_TEVSTAGE0, GX_MODULATE );
  GX_SetVtxDesc( GX_VA_TEX0, GX_DIRECT );

  guMtxIdentity( m );
  guMtxScaleApply( m, m, so->scale, so->scale, 1.0f );
  guMtxTransApply( m, m, so->ox, so->oy, 0 );
  guMtxConcat( oric->disp.GXmodelView2D, m, mv );
  GX_LoadPosMtxImm( mv, GX_PNMTX0 );

  GX_Begin( GX_QUADS, GX_VTXFMT0, 4 );
    GX_Position3f32(       so->x,       so->y, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 0.0f, 0.0f );
    GX_Position3f32( so->x+so->w,       so->y, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 1.0f, 0.0f );
    GX_Position3f32( so->x+so->w, so->y+so->h, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 1.0f, 1.0f );
    GX_Position3f32(       so->x, so->y+so->h, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 0.0f, 1.0f );
  GX_End();

  if( sp )
  {
    int i, j, k, cstate;
    struct sp_button *spb;

    for( i=0; sp->btns[i].w; i++ )
    {
      spb = &sp->btns[i];

      switch( spb->type )
      {
        case BTYP_RADIO:
          cstate = -1;
          k = 0;
          while( ( spb->str[k] != 0 ) && ( spb->str[k] != '|' ) ) k++;
          if( spb->str[k] != 0 ) k++;
          for( j=0; spb->str[k]; j+=24 )
          {
            if( cstate != (spb->state==j/24) )
            {
              cstate = (spb->state==j/24);
              TPL_GetTexture( &oric->disp.texTPL, cstate ? gfx_radio_on : gfx_radio_off, &texObj );
              GX_LoadTexObj( &texObj, GX_TEXMAP0 );
            }

            GX_Begin( GX_QUADS, GX_VTXFMT0, 4 );
            GX_Position3f32(       so->x+spb->x,       so->y+spb->y+j, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 0.0f, 0.0f );
            GX_Position3f32( so->x+spb->x+28.0f,       so->y+spb->y+j, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 1.0f, 0.0f );
            GX_Position3f32( so->x+spb->x+28.0f, so->y+spb->y+j+24.0f, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 1.0f, 1.0f );
            GX_Position3f32(       so->x+spb->x, so->y+spb->y+j+24.0f, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 0.0f, 1.0f );
            GX_End();

            while( ( spb->str[k] != 0 ) && ( spb->str[k] != '|' ) ) k++;
            if( spb->str[k] != 0 ) k++;
          }

          spb->h = j;
          break;

        case BTYP_CHECKBOX:
          TPL_GetTexture( &oric->disp.texTPL, spb->state ? gfx_cbox_on : gfx_cbox_off, &texObj );
          GX_LoadTexObj( &texObj, GX_TEXMAP0 );

          GX_Begin( GX_QUADS, GX_VTXFMT0, 4 );
            GX_Position3f32(       so->x+spb->x,       so->y+spb->y, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 0.0f, 0.0f );
            GX_Position3f32( so->x+spb->x+32.0f,       so->y+spb->y, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 1.0f, 0.0f );
            GX_Position3f32( so->x+spb->x+32.0f, so->y+spb->y+24.0f, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 1.0f, 1.0f );
            GX_Position3f32(       so->x+spb->x, so->y+spb->y+24.0f, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 0.0f, 1.0f );
          GX_End();
          spb->h = 24;
          break;

        case BTYP_BUTTON:
          TPL_GetTexture( &oric->disp.texTPL, spb->state ? gfx_buttondn_lft : gfx_buttonup_lft, &texObj );
          GX_LoadTexObj( &texObj, GX_TEXMAP0 );

          GX_Begin( GX_QUADS, GX_VTXFMT0, 4 );
            GX_Position3f32(       so->x+spb->x,       so->y+spb->y, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 0.0f, 0.0f );
            GX_Position3f32( so->x+spb->x+16.0f,       so->y+spb->y, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 1.0f, 0.0f );
            GX_Position3f32( so->x+spb->x+16.0f, so->y+spb->y+24.0f, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 1.0f, 1.0f );
            GX_Position3f32(       so->x+spb->x, so->y+spb->y+24.0f, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 0.0f, 1.0f );
          GX_End();

          TPL_GetTexture( &oric->disp.texTPL, spb->state ? gfx_buttondn_mid : gfx_buttonup_mid, &texObj );
          GX_LoadTexObj( &texObj, GX_TEXMAP0 );

          GX_Begin( GX_QUADS, GX_VTXFMT0, 4 );
            GX_Position3f32(                so->x+spb->x+16.0f,       so->y+spb->y, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 0.0f, 0.0f );
            GX_Position3f32( so->x+spb->x+16.0f+(spb->w-32.0f),       so->y+spb->y, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 1.0f, 0.0f );
            GX_Position3f32( so->x+spb->x+16.0f+(spb->w-32.0f), so->y+spb->y+24.0f, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 1.0f, 1.0f );
            GX_Position3f32(                so->x+spb->x+16.0f, so->y+spb->y+24.0f, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 0.0f, 1.0f );
          GX_End();

          TPL_GetTexture( &oric->disp.texTPL, spb->state ? gfx_buttondn_rgt : gfx_buttonup_rgt, &texObj );
          GX_LoadTexObj( &texObj, GX_TEXMAP0 );

          GX_Begin( GX_QUADS, GX_VTXFMT0, 4 );
            GX_Position3f32( so->x+spb->x+(spb->w-16.0f),       so->y+spb->y, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 0.0f, 0.0f );
            GX_Position3f32(         so->x+spb->x+spb->w,       so->y+spb->y, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 1.0f, 0.0f );
            GX_Position3f32(         so->x+spb->x+spb->w, so->y+spb->y+24.0f, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 1.0f, 1.0f );
            GX_Position3f32( so->x+spb->x+(spb->w-16.0f), so->y+spb->y+24.0f, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 0.0f, 1.0f );
          GX_End();
          spb->h = 24;
          break;
      }
    }
  }

  render_unbindtexture();

  if( sp )
  {
    int i;
    struct sp_button *spb;

    for( i=0; sp->btns[i].w; i++ )
    {
      spb = &sp->btns[i];

      switch( spb->type )
      {
        case BTYP_RADIO:
          GX_Begin( GX_LINESTRIP, GX_VTXFMT0, 9 );
            GX_Position3f32(        so->x+spb->x+4,       so->y+spb->y-10, 0.0f ); GX_Color4u8( 0x00, 0x00, 0x00, so->a>>9 );
            GX_Position3f32( so->x+spb->x+spb->w-4,       so->y+spb->y-10, 0.0f ); GX_Color4u8( 0x00, 0x00, 0x00, so->a>>9 );
            GX_Position3f32( so->x+spb->x+spb->w  ,       so->y+spb->y- 6, 0.0f ); GX_Color4u8( 0x00, 0x00, 0x00, so->a>>9 );
            GX_Position3f32( so->x+spb->x+spb->w  , so->y+spb->y+spb->h  , 0.0f ); GX_Color4u8( 0x00, 0x00, 0x00, so->a>>9 );
            GX_Position3f32( so->x+spb->x+spb->w-4, so->y+spb->y+spb->h+4, 0.0f ); GX_Color4u8( 0x00, 0x00, 0x00, so->a>>9 );
            GX_Position3f32(        so->x+spb->x+4, so->y+spb->y+spb->h+4, 0.0f ); GX_Color4u8( 0x00, 0x00, 0x00, so->a>>9 );
            GX_Position3f32(        so->x+spb->x  , so->y+spb->y+spb->h  , 0.0f ); GX_Color4u8( 0x00, 0x00, 0x00, so->a>>9 );
            GX_Position3f32(        so->x+spb->x  ,       so->y+spb->y- 6, 0.0f ); GX_Color4u8( 0x00, 0x00, 0x00, so->a>>9 );
            GX_Position3f32(        so->x+spb->x+4,       so->y+spb->y-10, 0.0f ); GX_Color4u8( 0x00, 0x00, 0x00, so->a>>9 );
          GX_End();
          break;
        
        case BTYP_LIST:
          {
            struct listdata *data = (struct listdata *)spb->data;
            int knobred = (spb->state == 2) ? 0xff : 0x88;

            GX_Begin( GX_QUADS, GX_VTXFMT0, 8 );
              GX_Position3f32(           so->x+spb->x,        so->y+spb->y, 0.0f ); GX_Color4u8( 0x00, 0x00, 0x00, so->a>>9 );
              GX_Position3f32( so->x+spb->x+spb->w-32,        so->y+spb->y, 0.0f ); GX_Color4u8( 0x00, 0x00, 0x00, so->a>>9 );
              GX_Position3f32( so->x+spb->x+spb->w-32, so->y+spb->y+spb->h, 0.0f ); GX_Color4u8( 0x00, 0x00, 0x00, so->a>>9 );
              GX_Position3f32(           so->x+spb->x, so->y+spb->y+spb->h, 0.0f ); GX_Color4u8( 0x00, 0x00, 0x00, so->a>>9 );

              GX_Position3f32( so->x+spb->x+spb->w-32,        so->y+spb->y, 0.0f ); GX_Color4u8( 0x00, 0x00, 0x00, so->a>>8 );
              GX_Position3f32(    so->x+spb->x+spb->w,        so->y+spb->y, 0.0f ); GX_Color4u8( 0x00, 0x00, 0x00, so->a>>8 );
              GX_Position3f32(    so->x+spb->x+spb->w, so->y+spb->y+spb->h, 0.0f ); GX_Color4u8( 0x00, 0x00, 0x00, so->a>>8 );
              GX_Position3f32( so->x+spb->x+spb->w-32, so->y+spb->y+spb->h, 0.0f ); GX_Color4u8( 0x00, 0x00, 0x00, so->a>>8 );
            GX_End();

            if( data ) data->hover = -1;

            if( ( oric->disp.ir.smooth_valid ) && ( data ) )
            {
              if( ( oric->disp.ir.sx >= spb->x+60 ) &&
                  ( oric->disp.ir.sx < spb->x+spb->w+28 ) &&
                  ( oric->disp.ir.sy >= spb->y+40 ) &&
                  ( oric->disp.ir.sy < spb->y+40+spb->h ) &&
                  ( spb->state != 2 ) )
              {
                int line = (oric->disp.ir.sy - (spb->y+40)) / 12;

                if ( ( spb->state == 1 ) && ( data->sel != -1 ) )
                {
                  if ( abs( line-(data->sel-data->top) ) > 1 )
                  {
                    line = -1;
                  } else {
                    line = data->sel-data->top;
                  }
                }

                if( ( line >= 0 ) && ( (line+data->top) < data->total ) )
                {
                  data->hover = line+data->top;
  
                  GX_Begin( GX_QUADS, GX_VTXFMT0, 4 );
                    GX_Position3f32(           so->x+spb->x,    so->y+spb->y+line*12, 0.0f ); GX_Color4u8( 0x88, 0x00, 0x00, so->a>>8 );
                    GX_Position3f32( so->x+spb->x+spb->w-32,    so->y+spb->y+line*12, 0.0f ); GX_Color4u8( 0x88, 0x00, 0x00, so->a>>8 );
                    GX_Position3f32( so->x+spb->x+spb->w-32, so->y+spb->y+line*12+12, 0.0f ); GX_Color4u8( 0x88, 0x00, 0x00, so->a>>8 );
                    GX_Position3f32(           so->x+spb->x, so->y+spb->y+line*12+12, 0.0f ); GX_Color4u8( 0x88, 0x00, 0x00, so->a>>8 );
                  GX_End();
                }
              }
            }

            if( (data) && (data->knobsize != 0.0f) )
            {
              f32 knobtop = (((f32)data->top) * spb->h) / ((f32)data->total);

              if( knobtop < 0.0f )
              {
                knobtop = 0.0f;
              }

              if( knobtop > (spb->h - data->knobsize) )
              {
                knobtop = spb->h - data->knobsize;
              }

              GX_Begin( GX_QUADS, GX_VTXFMT0, 4 );
                GX_Position3f32( so->x+spb->x+spb->w-28, so->y+spb->y+knobtop,                0.0f ); GX_Color4u8( knobred, 0x00, 0x00, so->a>>8 );
                GX_Position3f32(  so->x+spb->x+spb->w-4, so->y+spb->y+knobtop,                0.0f ); GX_Color4u8( knobred, 0x00, 0x00, so->a>>8 );
                GX_Position3f32(  so->x+spb->x+spb->w-4, so->y+spb->y+knobtop+data->knobsize, 0.0f ); GX_Color4u8( knobred, 0x00, 0x00, so->a>>8 );
                GX_Position3f32( so->x+spb->x+spb->w-28, so->y+spb->y+knobtop+data->knobsize, 0.0f ); GX_Color4u8( knobred, 0x00, 0x00, so->a>>8 );
              GX_End();
            } else {
              GX_Begin( GX_QUADS, GX_VTXFMT0, 4 );
                GX_Position3f32( so->x+spb->x+spb->w-28, so->y+spb->y,        0.0f ); GX_Color4u8( knobred, 0x00, 0x00, so->a>>8 );
                GX_Position3f32(  so->x+spb->x+spb->w-4, so->y+spb->y,        0.0f ); GX_Color4u8( knobred, 0x00, 0x00, so->a>>8 );
                GX_Position3f32(  so->x+spb->x+spb->w-4, so->y+spb->y+spb->h, 0.0f ); GX_Color4u8( knobred, 0x00, 0x00, so->a>>8 );
                GX_Position3f32( so->x+spb->x+spb->w-28, so->y+spb->y+spb->h, 0.0f ); GX_Color4u8( knobred, 0x00, 0x00, so->a>>8 );
              GX_End();
            }
          }
          break;
      }
    }
  }

  render_bindtexture( &texObj, oric->disp.pnltex, 520, 400, GX_LINEAR );

  GX_Begin( GX_QUADS, GX_VTXFMT0, 4 );
    GX_Position3f32(       so->x,       so->y, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 0.0f, 0.0f );
    GX_Position3f32( so->x+so->w,       so->y, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 1.0f, 0.0f );
    GX_Position3f32( so->x+so->w, so->y+so->h, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 1.0f, 1.0f );
    GX_Position3f32(       so->x, so->y+so->h, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, so->a>>8 ); GX_TexCoord2f32( 0.0f, 1.0f );
  GX_End();

  render_unbindtexture();
}

void render_pointer( struct machine *oric )
{
  GXTexObj texObj;
  Mtx m, mv;

  if( !oric->disp.ir.smooth_valid ) return;

  TPL_GetTexture( &oric->disp.texTPL, gfx_pointer1, &texObj );
  GX_LoadTexObj( &texObj, GX_TEXMAP0 );

  GX_SetTevOp( GX_TEVSTAGE0, GX_MODULATE );
  GX_SetVtxDesc( GX_VA_TEX0, GX_DIRECT );

  guMtxRotAxisDeg( m, &axis, oric->disp.ir.angle );
  guMtxTransApply( m, m, oric->disp.ir.sx, oric->disp.ir.sy, 0 );
  guMtxConcat( oric->disp.GXmodelView2D, m, mv );
  GX_LoadPosMtxImm( mv, GX_PNMTX0 );

  GX_Begin( GX_QUADS, GX_VTXFMT0, 4 );
    GX_Position3f32( -48, -48, 20.0f ); GX_Color4u8( 0xff, 0xff, 0xff, 0xff ); GX_TexCoord2f32( 0.0f, 0.0f );
    GX_Position3f32(  48, -48, 20.0f ); GX_Color4u8( 0xff, 0xff, 0xff, 0xff ); GX_TexCoord2f32( 1.0f, 0.0f );
    GX_Position3f32(  48,  48, 20.0f ); GX_Color4u8( 0xff, 0xff, 0xff, 0xff ); GX_TexCoord2f32( 1.0f, 1.0f );
    GX_Position3f32( -48,  48, 20.0f ); GX_Color4u8( 0xff, 0xff, 0xff, 0xff ); GX_TexCoord2f32( 0.0f, 1.0f );
  GX_End();

  render_unbindtexture();
}

void render_oric_display( struct machine *oric )
{
  GXTexObj texObj;
  struct zoomobj *zo = &oric->disp.zobj[ZO_DISPLAY];

  render_bindtexture( &texObj, oric->odisp.tex, 240, 224, oric->odisp.linearscale ? GX_LINEAR : GX_NEAR );

  GX_LoadPosMtxImm( oric->disp.GXmodelView2D, GX_PNMTX0 );

  GX_Begin( GX_QUADS, GX_VTXFMT0, 4 );
    GX_Position3f32(       zo->x,       zo->y, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, zo->a>>8 ); GX_TexCoord2f32( 0.0f, 0.0f );
    GX_Position3f32( zo->x+zo->w,       zo->y, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, zo->a>>8 ); GX_TexCoord2f32( 1.0f, 0.0f );
    GX_Position3f32( zo->x+zo->w, zo->y+zo->h, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, zo->a>>8 ); GX_TexCoord2f32( 1.0f, 1.0f );
    GX_Position3f32(       zo->x, zo->y+zo->h, 0.0f ); GX_Color4u8( 0xff, 0xff, 0xff, zo->a>>8 ); GX_TexCoord2f32( 0.0f, 1.0f );
  GX_End();

  render_unbindtexture();
}

// Render one frame
void render( struct machine *oric )
{
  int kbdsettled, pnlsettled, needpointer = 0;
  int i;
  char foo[32];

  VIDEO_WaitVSync();

  if( oric->overkey != -1 )
  {
    sprintf( foo, "%02d", oric->overkey );
    render_printstr_solid( oric->odisp.rgb, 240, 0, 0, foo, 0xffffffff, 0x000040ff );
  }

  for( i=0; i<ZO_LAST; i++ )
    zobj_update( &oric->disp.zobj[i] );
  for( i=0; i<SO_LAST; i++ )
    sobj_update( &oric->disp.sobj[i] );

  kbdsettled = oric->disp.zobj[ZO_KBD].settled;
  pnlsettled = oric->disp.sobj[SO_SETTINGS].settled;

  switch( oric->emustate )
  {
    case ES_RUNNING:
      // Update the oric display texture
      render_texturemangle( 240, 224, (u8 *)oric->odisp.rgb, oric->odisp.tex );
      DCFlushRange( oric->odisp.tex, 240*224*4 );
      oric->disp.pleaseinvtexes = 1;
      break;
  }

  if( oric->disp.pleaseinvtexes )
  {
    GX_InvalidateTexAll();
    oric->disp.pleaseinvtexes = 0;
  }

  if( ( oric->disp.showkbd ) || ( !kbdsettled ) )
    render_kbd( oric );

  render_oric_display( oric );

  if( ( oric->disp.showkbd ) && ( kbdsettled ) )
    needpointer = 1;

  if( ( oric->emustate == ES_SETTINGS ) || ( !pnlsettled ) )
    render_settings_panel( oric );

  if( ( oric->emustate == ES_SETTINGS ) && ( pnlsettled ) )
    needpointer = 1;

  if( needpointer ) render_pointer( oric );

  render_flip( oric );
}

void render_set_displayzobj( struct machine *oric, int frames )
{
  f32 x, w, h;

  if( oric->disp.showkbd )
  {
    x = 320.0f-120.0f;
    w = 240.0f;
    h = 224.0f;
  } else {
    if( !oric->odisp.hstretch )
    {
      x = 80.0f;
      w = 480.0f;
    } else {
      x = 0.0f;
      w = 640.0f;
    }
    h = 448.0f;
  }

  if( frames > 0 )
  {
    zobj_zoom( &oric->disp.zobj[ZO_DISPLAY], x, 16.0f, w, h, 225, frames );
    return;
  }

  zobj_init( &oric->disp.zobj[ZO_DISPLAY], x, 16.0f, w, h, 225 );
}

void render_togglekbd( struct machine *oric )
{
  int i;

  oric->disp.showkbd = !oric->disp.showkbd;
  int frames = 8;
  if( oric->disp.showkbd )
  {
    zobj_zoom( &oric->disp.zobj[ZO_KBD],   0.0f, 240.0f, 640.0f, 240.0f, 255, frames ); 
    oric->disp.kbd_wiimote_show = 0;
    oric->disp.kbd_nunchuck_show = 0;
    oric->disp.kbd_classic_show = 0;
  } else {
    zobj_zoom( &oric->disp.zobj[ZO_KBD], 100.0f, 464.0f, 440.0f, 140.0f,   0, frames ); 
    for( i=0; oric->keylayout[i].x!=-1; i++ )
    {
      oric->keylayout[i].apressed = 0;
      oric->keylayout[i].bpressed = 0;
      oric->keylayout[i].highlight = 0;
      oric->keylayout[i].highlightfade = 0;
    }
  }

  render_set_displayzobj( oric, frames );
}

// Initialise the rendering subsystem
int render_init( struct machine *oric )
{
  GXColor background = { 0, 0, 0, 0xff };
  f32 yscale;
  u32 xfbHeight;
  Mtx perspective;
  struct wii_display  *d  = &oric->disp;
  struct oric_display *od = &oric->odisp;

  // Initialise the video system
  VIDEO_Init();
    
  // Obtain the preferred video mode from the system
  // This will correspond to the settings in the Wii menu
  d->rmode = VIDEO_GetPreferredMode( NULL );

  switch( d->rmode->viTVMode >> 2)
  {
    case VI_PAL:  d->is50hz = 1; break;
    case VI_NTSC: d->is50hz = 0; break;
    default:      d->is50hz = 0; break;
  }

  // Allocate memory for the display in the uncached region
  d->xfb[0] = (u32 *)MEM_K0_TO_K1( SYS_AllocateFramebuffer( d->rmode ) );
  d->xfb[1] = (u32 *)MEM_K0_TO_K1( SYS_AllocateFramebuffer( d->rmode ) );

  // Set up the video registers with the chosen mode
  VIDEO_Configure( d->rmode );
    
  // Tell the video hardware where our display memory is
  d->fb = 0;
  VIDEO_SetNextFramebuffer( d->xfb[d->fb] );
    
  // Flush the video register changes to the hardware
  VIDEO_Flush();

  // Wait for Video setup to complete
  VIDEO_WaitVSync();
  if( d->rmode->viTVMode&VI_NON_INTERLACE ) VIDEO_WaitVSync();

  // Now initialise GX
  d->gp_fifo = (u8 *)memalign( 32, DEFAULT_FIFO_SIZE );

  GX_Init( d->gp_fifo, DEFAULT_FIFO_SIZE );

  // clears the bg to color and clears the z buffer
  GX_SetCopyClear( background, 0x00ffffff );

  // other gx setup
  d->w = d->rmode->fbWidth;
  d->h = d->rmode->efbHeight;

  yscale = GX_GetYScaleFactor( d->rmode->efbHeight, d->rmode->xfbHeight );
  xfbHeight = GX_SetDispCopyYScale( yscale );
  GX_SetScissor( 0, 0, d->rmode->fbWidth, d->rmode->efbHeight );
  GX_SetDispCopySrc( 0, 0, d->rmode->fbWidth, d->rmode->efbHeight );
  GX_SetDispCopyDst( d->rmode->fbWidth, xfbHeight );
  GX_SetCopyFilter( d->rmode->aa, d->rmode->sample_pattern, GX_TRUE, d->rmode->vfilter);
  GX_SetFieldMode( d->rmode->field_rendering, ((d->rmode->viHeight==2*d->rmode->xfbHeight) ? GX_ENABLE : GX_DISABLE) );

  if( d->rmode->aa )
    GX_SetPixelFmt( GX_PF_RGB565_Z16, GX_ZC_LINEAR );
  else
    GX_SetPixelFmt( GX_PF_RGB8_Z24, GX_ZC_LINEAR );

  GX_SetDispCopyGamma( GX_GM_1_0 );

  // setup the vertex descriptor
  // tells the flipper to expect direct data
  GX_ClearVtxDesc();
  GX_InvVtxCache ();
  GX_InvalidateTexAll();

  GX_SetVtxDesc( GX_VA_TEX0, GX_NONE );
  GX_SetVtxDesc( GX_VA_POS,  GX_DIRECT );
  GX_SetVtxDesc( GX_VA_CLR0, GX_DIRECT );


  GX_SetVtxAttrFmt( GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0 );
  GX_SetVtxAttrFmt( GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 );
  GX_SetVtxAttrFmt( GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0 );
  GX_SetZMode ( GX_FALSE, GX_LEQUAL, GX_TRUE );

  GX_SetNumChans( 1 );
  GX_SetNumTexGens( 1 );
  GX_SetTevOp( GX_TEVSTAGE0, GX_PASSCLR );
  GX_SetTevOrder( GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0 );
  GX_SetTexCoordGen( GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY );

  guMtxIdentity( d->GXmodelView2D );
  guMtxTransApply( d->GXmodelView2D, d->GXmodelView2D, 0.0F, 0.0F, -50.0F );
  GX_LoadPosMtxImm( d->GXmodelView2D, GX_PNMTX0 );

  guOrtho( perspective, 0, d->h, 0, d->w, 0, 300 );
  GX_LoadProjectionMtx( perspective, GX_ORTHOGRAPHIC );

  GX_SetViewport( 0, 0, d->rmode->fbWidth, d->rmode->efbHeight, 0, 1 );
  GX_SetBlendMode( GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR );
  GX_SetAlphaUpdate( GX_TRUE );
  
  GX_SetCullMode( GX_CULL_NONE );

  // Allocate texture memory for dynamic textures
  od->tex = memalign( 32, 240*224*4 );
  d->pnltex = memalign( 32, 520*400*4 );
  d->kbdinftex = memalign( 32, 160*224*4 );

  TPL_OpenTPLFromMemory( &d->texTPL, (void *)textures_tpl, textures_tpl_size );

  render_set_displayzobj( oric, 0 );
  zobj_init( &d->zobj[ZO_KBD],     100.0f, 464.0f, 440.0f, 140.0f,   0 );

  sobj_init( &d->sobj[SO_SETTINGS], 320.0f, 240.0f, -260.0f, -200.0f, 520.0f, 400.0f, 0.6f, 0 );

  d->showkbd = 0;

  return 1;
}

// Shut down the rendering subsystem
void render_shut( struct machine *oric )
{
}
