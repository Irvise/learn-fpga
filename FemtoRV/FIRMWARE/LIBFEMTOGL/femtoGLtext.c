#include <femtoGL.h>

//#define FONT_8x16 // VGA font (WIP)
//#define FONT_8x8 // CGA font
#define FONT_5x6 // pico8 font by zep
//#define FONT_3x5 // pico8 default font (still some pbs with scrolling)

#if   defined(FONT_8x16)
#define FONT_WIDTH  8
#define FONT_HEIGHT 16
#elif defined(FONT_8x8)
#define FONT_WIDTH  8
#define FONT_HEIGHT 8
#elif defined(FONT_5x6)
#define FONT_WIDTH  6
#define FONT_HEIGHT 8 /* Yes, 8 and not 6, because some cars have legs ! */
#elif defined(FONT_3x5)
#define FONT_WIDTH  4
#define FONT_HEIGHT 6
#endif

static int scrolling = 0; 
static int cursor_X = 0;
static int cursor_Y = 0;
static int display_start_line = 0;
static int last_char_was_CR = 0;

void GL_tty_init() {
    GL_init(GL_MODE_OLED);
    GL_clear();
    set_putcharfunc(GL_putchar);
    cursor_X = 0;
    cursor_Y = 0;
    scrolling = 0;
    display_start_line = 0;
    oled1(0xA1, 0); /* reset display start line. */
}

void GL_tty_goto_xy(int X, int Y) {
    cursor_X = X;
    cursor_Y = Y;
}

/* 
 Using SSD1351 'display start line' command, we 
 can scroll the terminal without memorizing the 
 contents anywhere !    
 */
void GL_tty_scroll() {
    if(cursor_Y >= GL_height) {
       scrolling = 1;
       cursor_Y = 0;
    }
    if(!scrolling) {
	return;
    }
    GL_fill_rect(0,cursor_Y,GL_width-1,cursor_Y+FONT_HEIGHT-1, GL_bg);
    display_start_line += FONT_HEIGHT;
    if(display_start_line >= GL_height) {
       display_start_line = 0;
    }
    oled1(0xA1, display_start_line);
}

int GL_putchar(int c) {

   if(last_char_was_CR) {
      GL_tty_scroll();
      last_char_was_CR = 0;
   }
   
   
#ifdef FONT_3x5
    // In the pico8 font, small caps and big caps
    // are swapped (I don't know why). TODO: fix
    // the data instead, will be cleaner...
    if(c >= 'A' && c <= 'Z') {
	c = c - 'A' + 'a';
    } else if(c >= 'a' && c <= 'z') {
	c = c - 'a' + 'A';
    }
#endif
    
    if(c == '\r') {
	if(cursor_X >= FONT_WIDTH) {
	    cursor_X -= FONT_WIDTH;
	}
	return c;
    }
    
    if(c == '\n') {
        last_char_was_CR = 1; 
	cursor_X = 0;
        cursor_Y += FONT_HEIGHT;
	return c;
    }
   
    GL_putchar_xy(cursor_X, cursor_Y, (char)c); 
    cursor_X += FONT_WIDTH;
    if(cursor_X >= GL_width) {
       GL_putchar('\n');
    }
    return c;
}

void GL_putchar_xy(int X, int Y, char c) {
#if defined(FONT_8x16)   
   GL_write_window(X,Y,X+7,Y+15);   
   uint16_t* car_ptr = font_8x16 + (int)c * 8;
   for(int row=0; row<16; ++row) {
      for(int col=0; col<8; ++col) {
	 uint32_t BW = (car_ptr[col] & (1 << row)) ? 255 : 0;
	 GL_WRITE_DATA_UINT16(BW ? GL_fg : GL_bg);
      }
   }
#elif defined(FONT_8x8)
   GL_write_window(X,Y,X+7,Y+7);   
   uint8_t* car_ptr = font_8x8 + (int)c * 8;
   for(int row=0; row<8; ++row) {
      for(int col=0; col<8; ++col) {
	 uint32_t BW = (car_ptr[col] & (1 << row)) ? 255 : 0;
	 GL_WRITE_DATA_UINT16(BW ? GL_fg : GL_bg);
      }
   }
#elif defined(FONT_5x6)
   GL_write_window(X,Y,X+5,Y+7);
   uint32_t chardata = font_5x6[c - ' '];
   // bit 30 indicates whether character needs to be shifted downwards by
   // two pixels (for instance, for letters 'p','q','g')
   int shifted = chardata & (1 << 30);
   for(int row=0; row<8; ++row) {
       for(int col=0; col<6; ++col) {
	   uint32_t BW = 0;
	   if(col < 5) {
	       unsigned int coldata = (chardata >> (6 * col)) & 63;
	       if(shifted) {
		   coldata = coldata << 2;
	       }
	       BW = (coldata & (1 << row)) ? 255 : 0;
	   }
	   GL_WRITE_DATA_UINT16(BW ? GL_fg : GL_bg);
       }
   }
#elif defined(FONT_3x5)
   GL_write_window(X,Y,X+3,Y+5);
   uint16_t car_data = font_3x5[c - ' '];
   for(int row=0; row<6; ++row) {
      for(int col=0; col<4; ++col) {
	  uint32_t BW = 0;
	  if(col < 3) {
	      uint32_t coldata = (car_data >> (5 * col)) & 31;
	      BW = (coldata & (1 << row)) ? 255 : 0;
	  }
          GL_WRITE_DATA_UINT16(BW ? GL_fg : GL_bg);	 
      }
   }
#endif
}
		