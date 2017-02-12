
struct kbdkey
{
  int x, y, w, h;
  int highlight;
  int highlightfade;
  int apressed, bpressed, jpressed;
  int wpadbits_wiimote;
  int wpadbits_nunchuck;
  int wpadbits_classic;
};


int kbd_init( void );
void kbd_set( struct machine *oric );
void kbd_handler( struct machine *oric );
void kbd_queuekeys( struct ay8912 *ay, char *str );
void kbd_keypress( struct ay8912 *ay, unsigned short key, int down );
void kbd_osdkeypress( struct ay8912 *ay, int key, int down );

extern int kbd_wiimote_images[];
extern int kbd_nunchuck_images[];
extern int kbd_classic_images[];

