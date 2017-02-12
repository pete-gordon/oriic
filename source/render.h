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

#define DEFAULT_FIFO_SIZE (256 * 1024)

// Higher-level stuff
void render_togglekbd( struct machine *oric );
void render_set_displayzobj( struct machine *oric, int frames );

// Scaleable object management
void sobj_init( struct scaleobj *so, f32 ox, f32 oy, f32 x, f32 y, f32 w, f32 h, f32 scale, int a );
void sobj_zoom( struct scaleobj *so, f32 scale, int a, f32 frames );
void sobj_update( struct scaleobj *so );

// Zoomable object management
void zobj_init( struct zoomobj *zo, f32 x, f32 y, f32 w, f32 h, int a );
void zobj_zoom( struct zoomobj *zo, f32 x, f32 y, f32 w, f32 h, int a, f32 frames );
void zobj_update( struct zoomobj *zo );

// General purpose routines
void render_printstr( u32 *rgbabuf, int rgbaw, int x, int y, char *str, u32 fg );
void render_printstr_solid( u32 *rgbabuf, int rgbaw, int x, int y, char *str, u32 fg, u32 bg );
void render_dblprintstr( u32 *rgbabuf, int rgbaw, int x, int y, char *str, u32 fg );

// Texture management
void render_bindtexture( GXTexObj *texObj, u8 *texp, int w, int h, int mode );
void render_unbindtexture( void );
void render_texturemangle( int w, int h, u8 *src, u8 *dest );

// Main functions
void render( struct machine *oric );
int render_init( struct machine *oric );
void render_shut( struct machine *oric );

void render_printstr( u32 *rgbabuf, int rgbaw, int x, int y, char *str, u32 fg );
void render_texturemangle( int w, int h, u8 *src, u8 *dest );
