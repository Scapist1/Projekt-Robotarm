/*
 * ssd1306.h
 *based upon Adafruit_SSD1306 at github  -adjusted to C using i2c address 0x78 for writing and 0x7A for reading
 * Created: 03-01-2018 17:33:51
 *  Author: osch
  *page refers to:  SSD1306 Advanced information Matrix apr. 2008  rev. 1.1 www.solomon-systech.com describes all used commands and data used for the display 128x64 dot matrix
  *p reference to this documentation
  *initialization is from the DD-2864bY-3A rev c p 19
  *both data sheets are at git hub in this project
 */ 

#ifndef SSD1306_H_
#define SSD1306_H_

#include <stdbool.h>

  #define SSD1306_128_64
//#define SSD1306_128_32
//   #define SSD1306_96_16
#if defined SSD1306_128_64 && defined SSD1306_128_32
#error "Only one SSD1306 display can be specified at once in SSD1306.h"
#endif
#if !defined SSD1306_128_64 && !defined SSD1306_128_32 && !defined SSD1306_96_16
#error "At least one SSD1306 display must be specified in SSD1306.h"
#endif

#if defined SSD1306_128_64
#define SSD1306_LCDWIDTH                  128
#define SSD1306_LCDHEIGHT                 64
#endif
#if defined SSD1306_128_32
#define SSD1306_LCDWIDTH                  128
#define SSD1306_LCDHEIGHT                 32
#endif
#if defined SSD1306_96_16
#define SSD1306_LCDWIDTH                  96
#define SSD1306_LCDHEIGHT                 16
#endif

// #define pgm_read_byte(addr) (*(const unsigned char *)(addr))
//command data defined p 28 - 32
#define SSD1306_LCDWIDTH      128
#define SSD1306_LCDHEIGHT      64
#define SSD1306_SETCONTRAST   0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF
#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA
#define SSD1306_SETVCOMDETECT 0xDB
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9
#define SSD1306_SETMULTIPLEX 0xA8
#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10
#define SSD1306_SETSTARTLINE 0x40
#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR   0x22
#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8
#define SSD1306_SEGREMAP 0xA0
#define SSD1306_CHARGEPUMP 0x8D
#define SSD1306_EXTERNALVCC 0x1
#define SSD1306_SWITCHCAPVCC 0x2
// Scrolling #defines
#define SSD1306_ACTIVATE_SCROLL 0x2F
#define SSD1306_DEACTIVATE_SCROLL 0x2E
#define SSD1306_SET_VERTICAL_SCROLL_AREA 0xA3
#define SSD1306_RIGHT_HORIZONTAL_SCROLL 0x26
#define SSD1306_LEFT_HORIZONTAL_SCROLL 0x27
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL 0x2A

#define BLACK 0
#define WHITE 1
#define INVERSE 2

typedef uint8_t bitmap_t[8][128];
extern uint8_t _i2c_address; //lige prøvet at lave extern så vi ikk får kopier... Altså men det er jo ikke os der har skrevet den her kode så virker lidt risky at ændre hehe
void InitializeDisplay();
void sendStrXY( char *string, int X, int Y);
void sendStr( char *string);
void setXY(unsigned char row,unsigned char col);
void sendCharXY(unsigned char data, int X, int Y);
void SendChar(unsigned char data);
void displayOn(void);
void displayOff(void);
void clear_display(void);
void printBigTime(char *string);
void reset_display(void);
void printBigNumber(char string, int X, int Y);
void bmp(bitmap_t b);
void setPageAddress();
void setColAddress();
void ssd1306_setpos(uint8_t x, uint8_t y);
void ssd1306_draw_bmp(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, const uint8_t bitmap[]);
//scroll
void startscrollright(uint8_t start, uint8_t stop);
void startscrollleft(uint8_t start, uint8_t stop);

void startscrolldiagright(uint8_t start, uint8_t stop);
void startscrolldiagleft(uint8_t start, uint8_t stop);
void stopscroll(void);
void dim(bool dim);
void print_fonts();
void drawPixel(int16_t x, int16_t y, uint16_t color);

#endif
