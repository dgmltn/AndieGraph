/** AlmostTI: portable TI calcs emulator *********************/
/**                                                         **/
/**                          TI85.h                         **/
/**                                                         **/
/** This file contains declarations relevant to the drivers **/
/** and TI85 emulation itself. See Z80.h for #defines       **/
/** related to Z80 emulation.                               **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2009                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef TI85_H
#define TI85_H

#include "Z80/Z80.h"           /* Z80 CPU emulation          */

#define PAGESIZE  0x4000       /* Size of a RAM page         */
#define NORAM     0xFF         /* Byte to be returned from   */
                               /* non-existing addresses     */

#define CPU_CLOCK 6000000         /* TI85 Z80 CPU clock (Hz) */
#define VIDEO_CLK (CPU_CLOCK/50)  /* Screen refresh period   */
#define TIMER_CLK (CPU_CLOCK/200) /* Time clock period       */

/** Mode Bits ************************************************/
#define ATI_MODEL   0xFF00        /* Calculator model        */
#define ATI_TI85    0x0000        /* Emulate TI85 calculator */
#define ATI_TI86    0x0100        /* Emulate TI86 calculator */
#define ATI_TI82    0x0200        /* Emulate TI82 calculator */
#define ATI_TI83    0x0400        /* Emulate TI83 calculator */
#define ATI_TI83P   0x0800        /* Emulate TI83+ calculator*/
#define ATI_TI83SE  0x1000        /* Emulate TI83+ Silver Ed */
#define ATI_TI84    0x2000        /* Emulate TI84 calculator */
#define ATI_TI84P   0x2000        /* Emulate TI84+ calculator*/
#define ATI_TI84SE  0x4000        /* Emulate TI84+ Silver Ed */

#define TI83P_FAMILY ((Mode&ATI_MODEL)>=ATI_TI83P)
#define TI83_FAMILY  ((Mode&ATI_MODEL)>=ATI_TI82)
#define TI85_FAMILY  ((Mode&ATI_MODEL)<=ATI_TI86)

/** I/O Ports ************************************************/
#define PORT_LCDBUF   Ports[0]   /* x?AAAAAA                 */
#define PORT_KEYPAD   Ports[1]   /* xKKKKKKK                 */
#define PORT_CONTRAST Ports[2]   /* xCCCCCCC                 */
#define PORT_CONTROL  Ports[3]   /* xxx?LTSK                 */
#define PORT_LCDCTRL  Ports[4]   /* xxxWWIIF                 */
#define PORT_ROMPAGE  Ports[5]   /* xxxxxPPP or xMxxPPPP     */
#define PORT_POWER    Ports[6]   /* ???????P                 */
#define PORT_LINK     Ports[7]   /* CCCCDDDD                 */
#define PORT_ROMPAGE2 Ports[8]   /* xMxxPPPP in TI86         */
#define PORT_STATUS   Ports[15]  /* 0000KTLO                 */
#define PORT_ROMPAGE3 Ports[16]  /* xxxxxPPP in TI83+SE      */

#define TIMER_IRQ_ON  (PORT_CONTROL&0x04)  /* Timer IRQ on   */
#define VIDEO_IRQ_ON  (PORT_CONTROL&0x02)  /* Video IRQ on   */
#define ONKEY_IRQ_ON  (PORT_CONTROL&0x01)  /* ON key IRQ on  */
#define LCD_ON        (PORT_CONTROL&0x08)  /* LCD display on */
#define SLEEP_ON      ((PORT_CONTROL&0x0F)==0x01)

#define SCREEN_BUFFER (Page[3]+((int)(PORT_LCDBUF&0x3F)<<8))

/** TI82/TI83/TI84 LCD Controller ****************************/
#define TI83LCD_BUSY 0x80 /* LCD controller is busy          */
#define TI83LCD_8BIT 0x40 /* Using 12x8 scr width (else 16x6)*/
#define TI83LCD_ON   0x20 /* LCD screen is on                */
#define TI83LCD_DIR  0x03 /* Position increment direction    */

typedef struct
{ 
  byte Buffer[16*64]; /* 120x64 display buffer + 8 pixels */
  byte Status;        /* LCD_* status buts */
  byte Col,Row;       /* Cursor position*/
  byte Delay;         /* 1: Delay next data read */
  byte Scroll;        /* Vertical scroll value */
  byte Contrast;      /* Contrast value */
} TI83LCD;

/** Configuration by Model ***********************************/
typedef struct
{
  int         Model;
  const char *Backdrop;
  const char *ROMFile;
  int         ROMSize;
  const char *RAMFile;
  int         RAMSize;
} TIConfig;
 
/** Keys *****************************************************/
#define KBD_SET(K)  KbdStatus[Keys[K][0]]&=~Keys[K][1]
#define KBD_RES(K)  KbdStatus[Keys[K][0]]|=Keys[K][1]
#define IS_KBD(K)   !(KbdStatus[Keys[K][0]]&Keys[K][1])

#define KBD_2ND     0x00
#define KBD_ALPHA   0x01
#define KBD_XVAR    0x02
#define KBD_GRAPH   0x03
#define KBD_STAT    0x04   /* TI85 key */
#define KBD_TABLE   0x04   /* TI86 key */
#define KBD_PRGM    0x05
#define KBD_CUSTOM  0x06
#define KBD_LOG     0x07
#define KBD_DEL     0x08
#define KBD_MORE    0x09
#define KBD_SIN     0x0A
#define KBD_COS     0x0B
#define KBD_CLEAR   0x0C
#define KBD_ENTER   0x0D
#define KBD_TAN     0x0E
#define KBD_LN      0x0F
#define KBD_EE      0x10
#define KBD_SQR     0x11
#define KBD_STO     0x12
#define KBD_ON      0x13
#define KBD_SIGN    0x14
#define KBD_F1      0x15
#define KBD_F2      0x16
#define KBD_F3      0x17
#define KBD_F4      0x18
#define KBD_F5      0x19
#define KBD_EXIT    0x1B
#define KBD_LEFT    0x1C
#define KBD_RIGHT   0x1D
#define KBD_UP      0x1E
#define KBD_DOWN    0x1F
#define KBD_POWER   '^'
#define KBD_LPARENT '('
#define KBD_RPARENT ')'
#define KBD_DIV     '/'
#define KBD_7       '7'
#define KBD_8       '8'
#define KBD_9       '9'
#define KBD_MUL     '*'
#define KBD_COMMA   ','
#define KBD_4       '4'
#define KBD_5       '5'
#define KBD_6       '6'
#define KBD_MINUS   '-'
#define KBD_2       '2'
#define KBD_3       '3'
#define KBD_4       '4'
#define KBD_PLUS    '+'
#define KBD_1       '1'
#define KBD_0       '0'
#define KBD_DOT     '.'

/** Variables used to control emulator behavior **************/
extern int  Mode;              /* Operating mode bits        */
extern byte Verbose;           /* Debugging messages ON/OFF  */
extern byte UPeriod;           /* Interrupts / Screen update */
extern char *RAMFile;          /* Default state file name    */
/*************************************************************/

extern Z80 CPU;                /* CPU registers and state    */
extern byte *Page[4];          /* 4x16kB address space       */
extern byte *ROM,*RAM;         /* RAM and ROM buffers        */
extern byte Ports[32];         /* I/O ports                  */
extern TI83LCD LCD;            /* TI82/83/84 LCD controller  */
extern byte ExitNow;           /* 1: Exit the emulator       */
extern byte KbdStatus[8];      /* Keyboard matrix status     */
extern const byte Keys[][2];   /* KBD_* to row/column map    */
extern const char *ProgDir;    /* Program directory          */
extern const char *LinkPeer;   /* Link peer IP address       */
extern int LinkPort;           /* Link peer IP port          */
extern const TIConfig Config[];/* Config parameters by model */
extern char RAMPath[256];      /* RAM file name buffer       */
extern char ROMPath[256];      /* ROM file name buffer       */

/** StartTI85() **********************************************/
/** Allocate memory, load ROM image, initialize hardware,   **/
/** CPU and start the emulation. This function returns 0 in **/
/** the case of failure.                                    **/
/*************************************************************/
int StartTI85(void);

/** TrashTI85() **********************************************/
/** Free memory allocated by StartTI85().                   **/
/*************************************************************/
void TrashTI85(void);

/** ResetTI85() **********************************************/
/** Reset TI85 hardware to new operating modes. Returns new **/
/** modes, possibly not the same as NewMode.                **/
/*************************************************************/
int ResetTI85(int NewMode);

/** SaveSTA() ************************************************/
/** Save emulation state to a .STA file.                    **/
/*************************************************************/
int SaveSTA(const char *FileName);

/** LoadSTA() ************************************************/
/** Load emulation state from a .STA file.                  **/
/*************************************************************/
int LoadSTA(const char *FileName);

/** InitMachine() ********************************************/
/** Allocate resources needed by the machine-dependent code.**/
/************************************ TO BE WRITTEN BY USER **/
int InitMachine(void);

/** TrashMachine() *******************************************/
/** Deallocate all resources taken by InitMachine().        **/
/************************************ TO BE WRITTEN BY USER **/
void TrashMachine(void);

/** RefreshScreen() ******************************************/
/** Refresh picture on the screen.                          **/
/************************************ TO BE WRITTEN BY USER **/
void RefreshScreen(void);

/** SetColor() ***********************************************/
/** Set color N (0/1) to (R,G,B).                           **/
/************************************ TO BE WRITTEN BY USER **/
void SetColor(byte N,byte R,byte G,byte B);

/** Keypad() *************************************************/
/** Poll the keyboard. Returns 1 if KBD_ON pressed, else 0. **/
/************************************ TO BE WRITTEN BY USER **/
byte Keypad(void);

/** ShowBackdrop() *******************************************/
/** Show backdrop image with calculator faceplate.          **/
/************************************ TO BE WRITTEN BY USER **/
int ShowBackdrop(const char *FileName);

#endif /* TI85_H */
