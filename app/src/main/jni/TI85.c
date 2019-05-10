/** ATI85: portable TI85 emulator ****************************/
/**                                                         **/
/**                          TI85.c                         **/
/**                                                         **/
/** This file contains implementation for the TI-specific   **/
/** hardware. Initialization code and definitions needed    **/
/** for the machine-dependent drivers are also here.        **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2009                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include "TI85.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>

#include <android/log.h>
#define LOG_TAG "TI85"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#ifdef ZLIB
#include <zlib.h>
#define fopen           gzopen
#define fclose          gzclose
#define fread(B,L,N,F)  gzread(F,B,(L)*(N))
#define fwrite(B,L,N,F) gzwrite(F,B,(L)*(N))
#define fseek           gzseek
#define rewind          gzrewind
#define fgetc           gzgetc
#define ftell           gztell
#endif

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define FITWITHIN(x, l, h) (MIN(MAX(x, l), h))

/** User-defined parameters for ATI85 ************************/
int  Mode      = 0;          /* Various operating mode bits  */
byte Verbose   = 3;          /* Debug messages ON/OFF switch */
byte UPeriod   = 100;        /* % of actual screen updates   */
/*************************************************************/

/** Main hardware: CPU, RAM, VRAM, mappers *******************/
Z80  CPU;                    /* Z80 CPU registers and state  */
byte *Page[4];               /* 4x16kB read-only addr space  */
byte *RAM,*ROM;              /* Preallocated RAM/ROM buffers */
int  RAMSize,ROMSize;        /* RAM/ROM sizes, in bytes      */
byte Ports[32];              /* I/O ports                    */
TI83LCD LCD;                 /* TI82/83/84 LCD controller    */
byte ScreenOn;               /* 1: Show screen buffer        */
byte ExitNow;                /* 1: Exit the emulator         */
byte KbdStatus[8];           /* Keypad matrix status         */
byte StartupOn;              /* [ON] key counter on startup  */
char RAMPath[256];           /* RAM file name buffer         */
char ROMPath[256];           /* ROM file name buffer         */
/*************************************************************/

/** Working directory names, etc. ****************************/
const char *LinkPeer = 0;          /* Link peer IP address   */
int LinkPort         = 8385;       /* Link peer IP port      */
/*************************************************************/

/** Configuration, by model **********************************/
const TIConfig Config[] =
{
  { ATI_TI85,  "TI85.png",  "TI85.ROM",  0x20000, "TI85.RAM",  0x8000  },
  { ATI_TI86,  "TI86.png",  "TI86.ROM",  0x40000, "TI86.RAM",  0x20000 },
  { ATI_TI82,  "TI82.png",  "TI82.ROM",  0x20000, "TI82.RAM",  0x8000  },
  { ATI_TI83,  "TI83.png",  "TI83.ROM",  0x40000, "TI83.RAM",  0x8000  },
  { ATI_TI83P, "TI83P.png", "TI83P.ROM", 0x80000, "TI83P.RAM", 0x8000  },
  { ATI_TI83SE,"TI83SE.png","TI83SE.ROM",0x200000,"TI83SE.RAM",0x20000 },
  { ATI_TI84P, "TI84P.png", "TI84P.ROM", 0x100000,"TI84P.RAM", 0x20000 },
  { ATI_TI84SE,"TI84SE.png","TI84SE.ROM",0x200000,"TI84SE.RAM",0x20000 },
  { 0,0,0,0,0,0 }
};

/** Keys[][] *************************************************/
/** Map between KBD_* values and keyboard rows/columns.     **/
/*************************************************************/
const byte Keys[256][2] =
{
  /* 0x00: [2ND][ALPHA][XVAR][GRAPH][STAT][PRGM][CUSTOM][LOG] */
  { 6,0x20 },{ 5,0x80 },{ 4,0x80 },{ 5,0x40 },
  { 4,0x40 },{ 3,0x40 },{ 2,0x40 },{ 5,0x20 },
  /* 0x08: [DEL][MORE][SIN][COS][CLEAR][ENTER][TAN][LN] */
  { 3,0x80 },{ 6,0x80 },{ 4,0x20 },{ 3,0x20 },
  { 1,0x40 },{ 1,0x01 },{ 2,0x20 },{ 5,0x10 },
  /* 0x10: [EE][SQR][STO][ON][SIGN][F1][F2][F3] */
  { 4,0x10 },{ 5,0x08 },{ 5,0x02 },{ 7,0x08 },
  { 2,0x01 },{ 6,0x10 },{ 6,0x08 },{ 6,0x04 },
  /* 0x18: [F4][F5][?][EXIT][LEFT][RIGHT][UP][DOWN] */
  { 6,0x02 },{ 6,0x01 },{ 0,0x00 },{ 6,0x40 },
  { 0,0x02 },{ 0,0x04 },{ 0,0x08 },{ 0,0x01 },
  /* 0x20: [?][?][?][?][?][?][?][?] */
  { 0,0x00 },{ 0,0x00 },{ 0,0x00 },{ 0,0x00 },
  { 0,0x00 },{ 0,0x00 },{ 0,0x00 },{ 0,0x00 },
  /* 0x28: [(][)][*][+][,][-][.][/] */
  { 3,0x10 },{ 2,0x10 },{ 1,0x08 },{ 1,0x02 },
  { 5,0x04 },{ 1,0x04 },{ 3,0x01 },{ 1,0x10 },
  /* 0x30: [0][1][2][3][4][5][6][7] */
  { 4,0x01 },{ 4,0x02 },{ 3,0x02 },{ 2,0x02 },
  { 4,0x04 },{ 3,0x04 },{ 2,0x04 },{ 4,0x08 },
  /* 0x38: [8][9][?][?][?][?][?][?] */
  { 3,0x08 },{ 2,0x08 },{ 0,0x00 },{ 0,0x00 },
  { 0,0x00 },{ 0,0x00 },{ 0,0x00 },{ 0,0x00 },
  /* 0x40... */
  { 0,0x00 },{ 0,0x00 },{ 0,0x00 },{ 0,0x00 },
  { 0,0x00 },{ 0,0x00 },{ 0,0x00 },{ 0,0x00 },
  { 0,0x00 },{ 0,0x00 },{ 0,0x00 },{ 0,0x00 },
  { 0,0x00 },{ 0,0x00 },{ 0,0x00 },{ 0,0x00 },
  { 0,0x00 },{ 0,0x00 },{ 0,0x00 },{ 0,0x00 },
  { 0,0x00 },{ 0,0x00 },{ 0,0x00 },{ 0,0x00 },
  /* 0x58: [?][?][?][?][?][?][^][?] */
  { 0,0x00 },{ 0,0x00 },{ 0,0x00 },{ 0,0x00 },
  { 0,0x00 },{ 0,0x00 },{ 1,0x20 },{ 0,0x00 },
  /* 0x60... */
  { 0,0x00 },{ 0,0x00 },{ 0,0x00 },{ 0,0x00 },
  { 0,0x00 },{ 0,0x00 },{ 0,0x00 },{ 0,0x00 },
  { 0,0x00 },{ 0,0x00 },{ 0,0x00 },{ 0,0x00 },
  { 0,0x00 },{ 0,0x00 },{ 0,0x00 },{ 0,0x00 },
  { 0,0x00 },{ 0,0x00 },{ 0,0x00 },{ 0,0x00 },
  { 0,0x00 },{ 0,0x00 },{ 0,0x00 },{ 0,0x00 },
  { 0,0x00 },{ 0,0x00 },{ 0,0x00 },{ 0,0x00 },
  { 0,0x00 },{ 0,0x00 },{ 0,0x00 },{ 0,0x00 },
  { 0,0x00 },{ 0,0x00 },{ 0,0x00 },{ 0,0x00 },
  { 0,0x00 },{ 0,0x00 },{ 0,0x00 },{ 0,0x00 },
  { 0,0x00 },{ 0,0x00 },{ 0,0x00 },{ 0,0x00 },
  { 0,0x00 },{ 0,0x00 },{ 0,0x00 },{ 0,0x00 },
  { 0,0x00 },{ 0,0x00 },{ 0,0x00 },{ 0,0x00 },
  { 0,0x00 },{ 0,0x00 },{ 0,0x00 },{ 0,0x00 },
  { 0,0x00 },{ 0,0x00 },{ 0,0x00 },{ 0,0x00 },
  { 0,0x00 },{ 0,0x00 },{ 0,0x00 },{ 0,0x00 }
};

byte SIOExchange(byte Vout);

void TI83LCDReset(void);
byte TI83LCDDataRD(void);
void TI83LCDDataWR(register byte V);
void TI83LCDCtrl(register byte V);

void TI85Mapper(register byte Port5);
void TI86Mapper(register byte Port5,register byte Port6);
void TI83Mapper(register byte Port0,register byte Port2,register byte Port4);
void TI83PMapper(register byte Port4,register byte Port6,register byte Port7);
void TI84PMapper(register byte Port4,register byte Port6,register byte Port7,register byte Port5);

byte *TI83PPage(register byte PortValue);
byte *TI84PPage(register byte PortValue);

void TI85Colors(register byte V);
void TI83Colors(register byte V);

/** StartTI85() **********************************************/
/** Allocate memory, load ROM image, initialize hardware,   **/
/** CPU and start the emulation. This function returns 0 in **/
/** the case of failure.                                    **/
/*************************************************************/
int StartTI85()
{
  word A;
  int J,I,K;

  Page[0]=Page[1]=Page[2]=Page[3]=0;
  RAMSize=ROMSize=0;
  RAM=ROM=0;

  CPU.TrapBadOps = Verbose&0x10;
  CPU.IPeriod    = TIMER_CLK;
  CPU.IAutoReset = 1;
  ExitNow        = 0;

  /* UPeriod has ot be in 1%..100% range */
  UPeriod=UPeriod<1? 1:UPeriod>100? 100:UPeriod;

  /* Find largest RAM/ROM sizes to allocate */
  for(I=J=K=0;Config[J].ROMSize;++J)
  {
    if(Config[J].RAMSize>I) I=Config[J].RAMSize;
    if(Config[J].ROMSize>K) K=Config[J].ROMSize;
  }

  /* Allocate memory for RAM/ROM */
  if(Verbose) LOGD("Allocating %dkB+%dkB for RAM+ROM...",I>>10,K>>10);
  RAM = (byte *)malloc(I+K);
  if(Verbose) LOGD(RAM? "OK":"FAILED");
  if(!RAM) return(0);
  memset(RAM, NORAM, I+K);
  ROM=RAM+I;

  /* Reset hardware, force loading system ROM */
  J    = Mode;
  Mode|= ATI_MODEL;
  Mode = ResetTI85(J);

  /* Make sure ROM has been loaded and mode changed */
  if(Mode!=J) return(0);

  /* Try loading state */
  if(RAMPath) {
    J=LoadSTA(RAMPath);
    if(Verbose)
      LOGD("Loading %s...%s\n",RAMPath,J? "OK":"FAILED");
  }

  if(Verbose) LOGD("RUNNING ROM CODE...\n");
  A=RunZ80(&CPU);

  if(Verbose) LOGD("EXITED at PC = %Xh.\n",A);
  return(A);
}

/** TrashTI85() **********************************************/
/** Free memory allocated by StartTI85().                   **/
/*************************************************************/
void TrashTI85()
{
  int J;

  /* Save state */
  if(RAMPath&&RAM)
  {
    J=SaveSTA(RAMPath);
    if(Verbose) LOGD("Saving %s...%s\n",RAMPath,J? "OK":"FAILED");
  }

  /* Free memory */
  if(RAM) { free(RAM);RAM=ROM=0; }
}

/** ResetTI85() **********************************************/
/** Reset TI85 hardware to new operating modes. Returns new **/
/** modes, possibly not the same as NewMode.                **/
/*************************************************************/
int ResetTI85(int NewMode)
{
  FILE *F;
  int J,M;

  /* Figure out configuration */
  for(M=0;Config[M].ROMSize&&((NewMode&ATI_MODEL)!=Config[M].Model);++M);
  if(!Config[M].ROMSize) return(Mode);

  /* If calculator model changed... */
  if((Mode^NewMode)&ATI_MODEL)
  {
    /* Try loading ROM file */
    J=0;

    if(Verbose) LOGD("Loading %s...",ROMPath);
    if(F=fopen(ROMPath,"rb"))
    {
      if(Verbose) LOGD("Reading %s...",ROMPath);
      J = fread(ROM,1,Config[M].ROMSize,F);
      if (J != Config[M].ROMSize) {
        if (Verbose) LOGD("ROM Size 0x%x does not match expected value 0x%x", J, Config[M].ROMSize);
        J = 0;
      }
      fclose(F);
    }

    if(Verbose) LOGD(J? "OK":"FAILED");

    /* If failed loading ROM file, default to previous model */
    if(!J) NewMode=(NewMode&~ATI_MODEL)|(Mode&ATI_MODEL);
    else
    {
      /* Load faceplate backdrop image */
      ShowBackdrop(Config[M].Backdrop);

      /* New RAM/ROM sizes are now valid */
      RAMSize = Config[M].RAMSize;
      ROMSize = Config[M].ROMSize;

      /* Clear memory contents */
      memset(RAM,NORAM,RAMSize);

      /* If dealing with TI83+... */
      if(Config[M].Model==ATI_TI83P)
      {
        /* Clear EEPROM program storage */
        ROM[0x78000]=0;
        for(J=0x78001;J<0x7C000;J++) ROM[J]=0xFF;
      }
    }
  }

  /* Clear ports and keyboard map */
  memset(KbdStatus,0xFF,sizeof(KbdStatus));
  memset(Ports,0x00,sizeof(Ports));

  /* Reset state */
  Mode          = NewMode;
  StartupOn     = 128;
  PORT_LCDBUF   = 0x3C;
  PORT_LCDCTRL  = 0x16;
  PORT_LINK     = 0xC3;
  PORT_STATUS   = 0x08;
  PORT_ROMPAGE  = 0x00;
  PORT_ROMPAGE2 = 0x00;

  /* Depending on the mode... */
  switch(Mode&ATI_MODEL)
  {
    case ATI_TI82:
      /* TI82-specific initialization */
      Page[0] = ROM;
      Page[1] = ROM;
      Page[2] = RAM;
      Page[3] = RAM+0x4000;
      /* Reset LCD controller */
      TI83LCDReset();
      break;

    case ATI_TI83:
      /* TI83-specific registers */
      PORT_LINK    = 0x0C;
      PORT_ROMPAGE = 0x99;
      PORT_POWER   = 0x00;
      /* TI83-specific initialization */
      Page[0] = ROM;
      Page[1] = ROM+0x4000;
      Page[2] = RAM+0x4000;
      Page[3] = RAM;
      /* Reset LCD controller */
      TI83LCDReset();
      break;

    case ATI_TI83P:
      /* TI83+ specific initialization */
      PORT_LINK     = 0x03;
      PORT_POWER    = 0x76;
      PORT_ROMPAGE  = 0x01; 
      PORT_ROMPAGE2 = 0x41;
      /* Initial memory layout */
      TI83PMapper(PORT_POWER,PORT_ROMPAGE,PORT_ROMPAGE2);
      /* Reset LCD controller */
      TI83LCDReset();
      break;

    case ATI_TI83SE:
    case ATI_TI84P:
    case ATI_TI84SE:
      /* TI83+SE/TI84+/TI84+SE specific initialization */
      PORT_LINK     = 0x03;
      PORT_POWER    = 0x76;
      PORT_ROMPAGE  = 0x01; 
      PORT_ROMPAGE2 = 0x41;
      PORT_ROMPAGE3 = 0x00;
      /* Initial memory layout */
      TI84PMapper(PORT_POWER,PORT_ROMPAGE,PORT_ROMPAGE2,PORT_ROMPAGE3);
      /* Reset LCD controller */
      TI83LCDReset();
      break;

   case ATI_TI85:
      /* TI85-specific initialization */
      Page[0] = ROM;
      Page[1] = ROM;
      Page[2] = RAM;
      Page[3] = RAM+0x4000;
      break;

    case ATI_TI86:
      /* Do TI86-specific initialization */
      Page[0] = ROM;
      Page[1] = ROM;
      Page[2] = ROM;
      Page[3] = RAM;
      break;
  }

  /* Reset CPU */
  ResetZ80(&CPU);
  return(Mode);
}

/** SaveSTA() ************************************************/
/** Save emulation state to a .STA file.                    **/
/*************************************************************/
int SaveSTA(const char *FileName)
{
    LOGD("Saving state: %s", FileName);
  FILE *F;

  /* Open state file */
  F=fopen(FileName,"wb");
  if(!F) return(0);

  /* Write out hardware state */
  if(fwrite(&Mode,1,sizeof(Mode),F)!=sizeof(Mode))
  { fclose(F);unlink(FileName);return(0); }
  if(fwrite(&CPU,1,sizeof(CPU),F)!=sizeof(CPU))
  { fclose(F);unlink(FileName);return(0); }
  if(fwrite(Ports,1,sizeof(Ports),F)!=sizeof(Ports))
  { fclose(F);unlink(FileName);return(0); }
  if(fwrite(&LCD,1,sizeof(LCD),F)!=sizeof(LCD))
  { fclose(F);unlink(FileName);return(0); }
  if(fwrite(RAM,1,RAMSize,F)!=RAMSize)
  { fclose(F);unlink(FileName);return(0); }

  /* Done */
  fclose(F);
  return(1);
}

/** LoadSTA() ************************************************/
/** Load emulation state from a .STA file.                  **/
/*************************************************************/
int LoadSTA(const char *FileName)
{
    LOGD("Loading State: %s", FileName);

  FILE *F;
  int J;

  /* Open state file */
  F=fopen(FileName,"rb");
  if(!F) return(0);

  /* Read and match modes */
  if(fread(&J,1,sizeof(J),F)!=sizeof(J)) { fclose(F);return(0); }
  if((J!=Mode)&&(J!=ResetTI85(J)))       { fclose(F);return(0); }

  /* Read in hardware state */
  if(fread(&CPU,1,sizeof(CPU),F)!=sizeof(CPU))
  { fclose(F);ResetTI85(Mode);return(0); }
  if(fread(Ports,1,sizeof(Ports),F)!=sizeof(Ports))
  { fclose(F);ResetTI85(Mode);return(0); }
  if(fread(&LCD,1,sizeof(LCD),F)!=sizeof(LCD))
  { fclose(F);ResetTI85(Mode);return(0); }
  if(fread(RAM,1,RAMSize,F)!=RAMSize)
  { fclose(F);ResetTI85(Mode);return(0); }

  /* Restore memory layout */
  if(Mode&ATI_TI86)      TI86Mapper(PORT_ROMPAGE,PORT_ROMPAGE2);
  else if(Mode&ATI_TI85) TI85Mapper(PORT_ROMPAGE);
  else if(TI83P_FAMILY)  TI83PMapper(PORT_POWER,PORT_ROMPAGE,PORT_ROMPAGE2);
  else if(TI83_FAMILY)   TI83Mapper(PORT_LINK,PORT_ROMPAGE,PORT_POWER);

  /* Restore colors */
  if(TI83_FAMILY) TI83Colors(LCD.Contrast); else TI85Colors(PORT_CONTRAST);

  /* If not in "off" state, cancel [ON] key */
  if(!SLEEP_ON) StartupOn=0;

  /* Done */
  fclose(F);
  return(1);
}

/** RdZ80() **************************************************/
/** Z80 emulation calls this function to read a byte from   **/
/** address A of Z80 address space. Now moved to Z80.c and  **/
/** made inlined to speed things up.                        **/
/*************************************************************/
#ifndef TI85
byte RdZ80(word A) { return(Page[A>>14][A&0x3FFF]); }
#endif

/** WrZ80() **************************************************/
/** Z80 emulation calls this function to write byte V to    **/
/** address A of Z80 address space.                         **/
/*************************************************************/
#ifndef TI85
void WrZ80(word A,byte V) { if(Page[A>>14]<ROM) Page[A>>14][A&0x3FFF]=V; }
#endif

/** PatchZ80() ***********************************************/
/** Z80 emulation calls this function when it encounters a  **/
/** special patch command (ED FE) provided for user needs.  **/
/*************************************************************/
void PatchZ80(Z80 *R) { }

/** DebugZ80() ***********************************************/
/** This function should exist if DEBUG is #defined. When   **/
/** Trace!=0, it is called after each command executed by   **/
/** the CPU, and given the Z80 registers. Emulation exits   **/
/** if DebugZ80() returns 0.                                **/
/*************************************************************/
#ifdef DEBUG
byte DebugZ80(register Z80 *R) {
    return 1;
}
#endif

/** InZ80() **************************************************/
/** Z80 emulation calls this function to read a byte from   **/
/** a given I/O port.                                       **/
/*************************************************************/
byte InZ80(word Port)
{
  byte J;

  /* Simulate different models on different port ranges */
  Port=(Port&0xFF)|(Mode&ATI_MODEL);

#ifdef DEBUG
  LOGD("READ from IO port %02Xh at PC=%04Xh\n",Port&0xFF,CPU.PC.W);
#endif

  switch(Port)
  {
    case 0x0001: /* TI85 Keypad    */
    case 0x0101: /* TI86 Keypad    */
    case 0x0201: /* TI82 Keypad    */
    case 0x0401: /* TI83 Keypad    */
    case 0x0801: /* TI83+ Keypad   */
    case 0x1001: /* TI83+SE Keypad */
    case 0x2001: /* TI84+ Keypad   */
    case 0x4001: /* TI84+SE Keypad */
      J    = PORT_KEYPAD;
      Port = J&0x40? 0xFF:KbdStatus[6];
      Port&= J&0x20? 0xFF:KbdStatus[5];
      Port&= J&0x10? 0xFF:KbdStatus[4];
      Port&= J&0x08? 0xFF:KbdStatus[3];
      Port&= J&0x04? 0xFF:KbdStatus[2];
      Port&= J&0x02? 0xFF:KbdStatus[1];
      Port&= J&0x01? 0xFF:KbdStatus[0];
      return(Port);

    case 0x0000: /* TI85 Video Buffer      */
    case 0x0002: /* TI85 LCD Contrast      */
    case 0x0004: /* TI85 LCD Control       */
    case 0x0005: /* TI85 ROM Page 4000h    */
    case 0x0006: /* TI85 Power Register    */
    case 0x0100: /* TI86 Video Buffer      */
    case 0x0102: /* TI86 LCD Contrast      */
    case 0x0105: /* TI86 Memory Page 4000h */
    case 0x0404: /* TI83 IRQ Control       */
      return(Ports[Port&0x07]);

    case 0x0104: /* TI86 Power Register    */
      return(PORT_POWER);

    case 0x0106: /* TI86 Memory Page 8000h */
      return(PORT_ROMPAGE2);

    case 0x0003: /* TI85 Status    */
    case 0x0103: /* TI86 Status    */
    case 0x0203: /* TI82 Status    */
    case 0x0403: /* TI83 Status    */
    case 0x0803: /* TI83+ Status   */
    case 0x1003: /* TI83+SE Status */
    case 0x2003: /* TI84+ Status   */
    case 0x4003: /* TI84+SE Status */
    case 0x0804: /* TI83+ Status ???   */
    case 0x1004: /* TI83+SE Status ??? */
    case 0x2004: /* TI84+ Status ???   */
    case 0x4004: /* TI84+SE Status ??? */
      return(PORT_STATUS);

    case 0x0007: /* TI85 Link Register    */
    case 0x0107: /* TI86 Link Register    */
    case 0x0200: /* TI82 Link Register    */
//    J=PORT_LINK&0x0C;
//    PORT_LINK=J|(~((J>>2)|SIOExchange(J>>2))&0x03);
      return(PORT_LINK);

    case 0x0400: /* TI83 Link Register    */
    case 0x0800: /* TI83+ Link Register   */
    case 0x1000: /* TI83+SE Link Register */
    case 0x2000: /* TI84+ Link Register   */
    case 0x4000: /* TI84+SE Link Register */
      return(PORT_LINK);

    case 0x0202: /* TI82 Memory Page 4000h */
    case 0x0402: /* TI83 Memory Page 4000h */
      return(PORT_ROMPAGE);

    case 0x0210: /* TI82 LCD Status    */
    case 0x0410: /* TI83 LCD Status    */
    case 0x0810: /* TI83+ LCD Status   */
    case 0x1010: /* TI83+SE LCD Status */
    case 0x2010: /* TI84+ LCD Status   */
    case 0x4010: /* TI83+SE LCD Status */
      return(LCD.Status);

    case 0x0211: /* TI82 VRAM    */
    case 0x0411: /* TI83 VRAM    */
    case 0x0811: /* TI83+ VRAM   */
    case 0x1011: /* TI83+SE VRAM */
    case 0x2011: /* TI84+ VRAM   */
    case 0x4011: /* TI84+SE VRAM */
      return(TI83LCDDataRD());   
    case 0x0414: /* TI83 ??? */
      return(0x01);

    case 0x0802: /* TI83+ Hardware Status   */
      /* Report "battery ok" + "TI83+" */
      return(0x0b|((PORT_ROMPAGE3&0x07)<<3));
    case 0x1002: /* TI83+SE Hardware Status */
      /* Report "battery ok" + "TI83+SE" */
      return(0x81);
    case 0x2002: /* TI84+ Hardware Status   */
    case 0x4002: /* TI84+SE Hardware Status */
      /* Report "battery ok" + "TI84+/TI84+SE" */
      return(0xA1);

    case 0x1021: /* TI83+SE Hardware Type  */
    case 0x4021: /* TI84+SE Hardware Type  */
      /* Report "TI83+SE/TI84+SE" */
      return(0x01);
    case 0x2021: /* TI84+ Hardware Type    */
      /* Report "TI84+" */
      return(0x00);

    case 0x0806: /* TI83+ Memory Page #1   */
    case 0x1006: /* TI83+SE Memory Page #1 */
    case 0x2006: /* TI84+ Memory Page #1   */
    case 0x4006: /* TI84+SE Memory Page #1 */
      return(PORT_ROMPAGE);

    case 0x0807: /* TI83+ Memory Page #2   */
    case 0x1007: /* TI83+SE Memory Page #2 */
    case 0x2007: /* TI84+ Memory Page #2   */
    case 0x4007: /* TI84+SE Memory Page #2 */
      return(PORT_ROMPAGE2);

    case 0x1005: /* TI83+SE Memory Page #3 */
    case 0x2005: /* TI84+ Memory Page #3   */
    case 0x4005: /* TI84+SE Memory Page #3 */
      return(PORT_ROMPAGE3);
  }

  if(Verbose&0x02) LOGE("READ from IO port %02Xh\n",Port&0xFF);
  return(NORAM);
}

/** OutZ80() *************************************************/
/** Z80 emulation calls this function to write byte V to a  **/
/** given I/O port.                                         **/
/*************************************************************/
void OutZ80(word Port,byte V)
{
  byte Vin,Vout;

  /* Simulate different models on different port ranges */
  Port=(Port&0xFF)|(Mode&ATI_MODEL);

#ifdef DEBUG
  LOGE("WRITE %02Xh to IO port %02Xh at PC=%04Xh\n",V,Port&0xFF,CPU.PC.W);
#endif

  switch(Port)
  {
    case 0x0000: /* TI85 Video Buffer */
    case 0x0100: /* TI86 Video Buffer */
      PORT_LCDBUF=V;
      return;

    case 0x0001: /* TI85 Keypad    */
    case 0x0101: /* TI86 Keypad    */
    case 0x0201: /* TI82 Keypad    */
    case 0x0401: /* TI83 Keypad    */
    case 0x0801: /* TI83+ Keypad   */
    case 0x1001: /* TI83+SE Keypad */
    case 0x2001: /* TI84+ Keypad   */
    case 0x4001: /* TI84+SE Keypad */
      PORT_KEYPAD=V;
      return;

    case 0x0002: /* TI85 LCD Contrast */
    case 0x0102: /* TI86 LCD Contrast */
      PORT_CONTRAST=V&0x1F;
      TI85Colors(V);
      return;

    case 0x0003: /* TI85 Control    */
    case 0x0103: /* TI86 Control    */
    case 0x0203: /* TI82 Control    */
    case 0x0403: /* TI83 Control    */
    case 0x0803: /* TI83+ Control   */
    case 0x1003: /* TI83+SE Control */
    case 0x2003: /* TI84+ Control   */
    case 0x4003: /* TI84+SE Control */
      PORT_CONTROL=V;
      PORT_STATUS&=V|~0x07;
      return;

    case 0x0004: /* TI85 LCD Control */
      PORT_LCDCTRL=V;
      return;

    case 0x0005: /* TI85 ROM Page 4000h */
    case 0x0202: /* TI82 ROM Page 4000h */
      /* Plain TI82/TI85 only allow ROM at 4000h */
      Page[1] = ROM+((int)(V&0x07)<<14);
      PORT_ROMPAGE=V;
      return;

    case 0x0006: /* TI85 Power Register */
    case 0x0104: /* TI86 Power Register */
      PORT_POWER=V;
      return;

    case 0x0007: /* TI85 Link Register */
    case 0x0107: /* TI86 Link Register */
    case 0x0200: /* TI82 Link Register */
//      V=(V|(V>>2))&0x0C;
//      PORT_LINK=V|(~((V>>2)|SIOExchange(V>>2))&0x03);
      PORT_LINK=(PORT_LINK&0xF3)|(V&0x0C);
      return;

    case 0x0105: /* TI86 Memory Page 4000h */
      /* TI86 allows either ROM or RAM at 4000h */
      PORT_ROMPAGE=V;
      Page[1] = V&0x40?
        RAM+((int)(V&0x07)<<14)
      : ROM+((int)(V&0x0F)<<14);
      return;

    case 0x0106: /* TI86 Memory Page 8000h */
      /* TI86 allows either ROM or RAM at 8000h */
      PORT_ROMPAGE2=V;
      Page[2] = V&0x40?
        RAM+((int)(V&0x07)<<14)
      : ROM+((int)(V&0x0F)<<14);
      return;

    case 0x0400: /* TI83 Link Register + Memory Bit */
      PORT_LINK=((V^0x03)|0x0C)&0x1F;
      TI83Mapper(V,PORT_ROMPAGE,PORT_POWER);
      return;   

    case 0x0800: /* TI83+ Link Register + Link Assist   */
    case 0x1000: /* TI83+SE Link Register + Link Assist */
    case 0x2000: /* TI84+ Link Register + Link Assist   */
    case 0x4000: /* TI84+SE Link Register + Link Assist */
      PORT_LINK=((V&0x03)<<4)|(V&0x04)|0x03;
      return;   

    case 0x0402: /* TI83 Memory Page 4000h */
      PORT_ROMPAGE=V;
      TI83Mapper(PORT_LINK,V,PORT_POWER);
      return;   

    case 0x0404: /* TI83 Power + Timer + Memory Bit */
      PORT_POWER=V;
      TI83Mapper(PORT_LINK,PORT_ROMPAGE,V);
      return;   

    case 0x0210: /* TI82 LCD Command    */
    case 0x0410: /* TI83 LCD Command    */
    case 0x0810: /* TI83+ LCD Command   */
    case 0x1010: /* TI83+SE LCD Command */
    case 0x2010: /* TI84+ LCD Command   */
    case 0x4010: /* TI84+SE LCD Command */
      TI83LCDCtrl(V);
      return;   

    case 0x0211: /* TI82 VRAM Access    */ 
    case 0x0411: /* TI83 VRAM Access    */ 
    case 0x0811: /* TI83+ VRAM Access   */ 
    case 0x1011: /* TI83+SE VRAM Access */ 
    case 0x2011: /* TI84+ VRAM Access   */ 
    case 0x4011: /* TI84+SE VRAM Access */ 
      TI83LCDDataWR(V);
      return;   

    case 0x0804: /* TI83+ Timers + Memory Map */
      PORT_POWER=V;
      TI83PMapper(V,PORT_ROMPAGE,PORT_ROMPAGE2);
      return;
   
    case 0x1004: /* TI83+SE Timers + Memory Map */
    case 0x2004: /* TI84+ Timers + Memory Map   */
    case 0x4004: /* TI84+SE Timers + Memory Map */
      PORT_POWER=V;
      TI84PMapper(V,PORT_ROMPAGE,PORT_ROMPAGE2,PORT_ROMPAGE3);
      return;
   
    case 0x0806: /* TI83+ Memory Page #1 */
      PORT_ROMPAGE=V;
      TI83PMapper(PORT_POWER,V,PORT_ROMPAGE2);
      return;   

    case 0x1006: /* TI83+SE Memory Page #1 */
    case 0x2006: /* TI84+ Memory Page #1   */
    case 0x4006: /* TI84+SE Memory Page #1 */
      PORT_ROMPAGE=V;
      TI84PMapper(PORT_POWER,V,PORT_ROMPAGE2,PORT_ROMPAGE3);
      return;   

    case 0x0807: /* TI83+ Memory Page #2 */
      PORT_ROMPAGE2=V;
      TI83PMapper(PORT_POWER,PORT_ROMPAGE,V);
      return;   

    case 0x1007: /* TI83+SE Memory Page #2 */
    case 0x2007: /* TI84+ Memory Page #2   */
    case 0x4007: /* TI84+SE Memory Page #2 */
      PORT_ROMPAGE2=V;
      TI84PMapper(PORT_POWER,PORT_ROMPAGE,V,PORT_ROMPAGE3);
      return;   

    case 0x0805: /* TI83+ Flash Protect Model */
      PORT_ROMPAGE3=V;
      return;   

    case 0x1005: /* TI83+SE Memory Page #3 */
    case 0x2005: /* TI84+ Memory Page #3   */
    case 0x4005: /* TI84+SE Memory Page #3 */
      PORT_ROMPAGE3=V;
      TI84PMapper(PORT_POWER,PORT_ROMPAGE,PORT_ROMPAGE2,V);
      return;   
  }

  if(Verbose&0x02) LOGE("WRITE %02Xh to IO port %02Xh\n",V,Port&0xFF);
}

/** LoopZ80() ************************************************/
/** Z80 emulation calls this function periodically to check **/
/** if the system hardware requires any interrupts.         **/
/*************************************************************/
word LoopZ80(Z80 *R)
{
  static int  UCount=0;
  static byte ICount=0;
  byte ONKeyOn;

  /* When calculator turned off, exit */
  if(!StartupOn&&SLEEP_ON&&(R->IFF&IFF_HALT)) ExitNow=1;

  /* Refresh keypad state, get [ON] key status */
  ONKeyOn=Keypad()||StartupOn;

  /* [ON] key is held at startup */
  if(StartupOn) --StartupOn;

  /* Screen updates 1/4 of the timer ticks */
  ICount=(ICount+1)&3;

  /* Update status port */
  PORT_STATUS = (ONKeyOn?               0x00:0x08)
              | (TIMER_IRQ_ON?          0x04:0x00)
              | (VIDEO_IRQ_ON?          0x02:0x00)
              | (ONKEY_IRQ_ON&&ONKeyOn? 0x01:0x00);

  /* Refresh display 1/4 of the time */
  if(UCount>=400) { UCount-=400;RefreshScreen(); }
  UCount+=UPeriod;

  /* Return any pending interrupts */
  return(ExitNow? INT_QUIT:(PORT_STATUS&0x07)? INT_IRQ:INT_NONE);
}

/** TI8*Colors() *********************************************/
/** Set colors from the contrast value.                     **/
/*************************************************************/
void TI85Colors(register byte V)
{
  V = 0x7C-((V&0x1F)<<2);
  SetColor(1,V,V,V+0x20);
  V = 0xFF-V;
  SetColor(0,V-0x10,V,V-0x10);
}

void TI83Colors(register byte V)
{
  /* Setting pixel colors for screen to emulate
     low and high battery */
  int pxOn = 255 - MIN((((V&0x3F)-20)*10)+0, 254);
  byte pxOnR = (byte) FITWITHIN(pxOn, 1, 191);
  byte pxOnG = (byte) FITWITHIN(pxOn, 1, 199);
  byte pxOnB = (byte) FITWITHIN(pxOn, 33, 191);
  int pxOff = 511 - MAX((((V&0x3F)-12)*10)+0, 254);
  byte pxOffR = (byte) FITWITHIN(pxOff, 1, 191);
  byte pxOffG = (byte) FITWITHIN(pxOff, 1, 199);
  byte pxOffB = (byte) FITWITHIN(pxOff, 33, 191);

  // Setting the color for turned on pixels
  SetColor(1, pxOnR, pxOnG, pxOnB);
  // Setting the color for turned off pixels
  SetColor(0, pxOffR, pxOffG, pxOffB);
}

/** TI8*Page() ***********************************************/
/** Memory page addresses for different calc models.        **/
/*************************************************************/
byte *TI83PPage(register byte PortValue)
{
  return(PortValue&0x40?
    RAM+(((int)PortValue<<14)&(RAMSize-1))
  : ROM+(((int)PortValue<<14)&(ROMSize-1))
  );
}

byte *TI84PPage(register byte PortValue)
{
  return(PortValue&0x80?
    RAM+(((int)PortValue<<14)&(RAMSize-1))
  : ROM+(((int)PortValue<<14)&(ROMSize-1))
  );
}

/** TI85Mapper() *********************************************/
/** TI85 memory mapper, one 16kB ROM page at 4000h.         **/
/*************************************************************/
void TI85Mapper(register byte Port5)
{
  Page[0] = ROM;
  Page[1] = ROM+((int)(Port5&0x07)<<14);
  Page[2] = RAM;
  Page[3] = RAM+0x4000;
}

/** TI86Mapper() *********************************************/
/** TI85 memory mapper, two 16kB ROM/RAM pages.             **/
/*************************************************************/
void TI86Mapper(register byte Port5,register byte Port6)
{
  /* TI86 allows either ROM or RAM at 4000h and 8000h */
  Page[0] = ROM;
  Page[1] = Port5&0x40?
    RAM+((int)(Port5&0x07)<<14)
  : ROM+((int)(Port5&0x0F)<<14);
  Page[2] = Port6&0x40?
    RAM+((int)(Port6&0x07)<<14)
  : ROM+((int)(Port6&0x0F)<<14);
  Page[3] = RAM;
}

/** TI83Mapper() *********************************************/
/** TI83 memory mapper, based on bits from three ports.     **/
/*************************************************************/
void TI83Mapper(register byte Port0,register byte Port2,register byte Port4)
{
  byte *SwapPage;

  /* Compute swappable page address */
  SwapPage = Port2&0x40?
    RAM+((int)(Port2&0x01)<<14)
  : ROM+((int)(Port0&0x10)<<13)+((int)(Port2&0x07)<<14);

  /* Set up pages at 4000h,8000h,C000h */
  if(!(Port4&0x01))
  {
    Page[0] = ROM;
    Page[1] = SwapPage;
    Page[2] = Port2&0x80? RAM+((int)(Port2&0x08)<<11)
            : Port0&0x10? ROM+0x20000
            : ROM+((int)(Port2&0x08)<<11);
    Page[3] = RAM;
  }
  else if(Port2&0x40)
  {
    Page[0] = ROM;
    Page[1] = RAM;
    Page[2] = RAM+0x4000;
    Page[3] = Port2&0x80?
              RAM+((int)(Port2&0x08)<<11)
            : ROM+((int)(Port2&0x08)<<11)+((int)(Port0&0x10)<<13);
  }
  else
  {
    Page[0] = ROM;
    Page[1] = ROM+((int)(Port0&0x10)<<13);
    Page[2] = SwapPage;
    Page[3] = Port2&0x80?
              RAM+((int)(Port2&0x08)<<11)
            : ROM+((int)(Port2&0x08)<<11)+((int)(Port0&0x10)<<13);
  }
}

/** TI83PMapper() ********************************************/
/** TI83+ memory mapper.                                    **/
/*************************************************************/
void TI83PMapper(register byte Port4,register byte Port6,register byte Port7)
{
  /* Depending on the memory map selection... */
  if(Port4&0x01)
  {
    Page[0] = ROM;
    Page[1] = RAM;
    Page[2] = TI83PPage(Port6);
    Page[3] = TI83PPage(Port7);
  }
  else
  {
    Page[0] = ROM;
    Page[1] = TI83PPage(Port6);
    Page[2] = TI83PPage(Port7);
    Page[3] = RAM;
  }
}

/** TI84PMapper() ********************************************/
/** TI84+ and Silver Edition memory mapper.                 **/
/*************************************************************/
void TI84PMapper(register byte Port4,register byte Port6,register byte Port7,register byte Port5)
{
  /* Depending on the memory map selection... */
  if(Port4&0x01)
  {
    Page[0] = ROM;
    Page[1] = RAM;
    Page[2] = TI84PPage(Port6);
    Page[3] = TI84PPage(Port7);
  }
  else
  {
    Page[0] = ROM;
    Page[1] = TI84PPage(Port6);
    Page[2] = TI84PPage(Port7);
    Page[3] = RAM+(((int)Port5<<14)&(RAMSize-1));
  }
}

/** TI83LCDReset() *******************************************/
/** Reset TI83 LCD controller.                              **/
/*************************************************************/
void TI83LCDReset(void)
{
  memset(LCD.Buffer,0x00,sizeof(LCD.Buffer));
  LCD.Status   = 0x00;
  LCD.Row      = 0;
  LCD.Col      = 0;
  LCD.Delay    = 0;
  LCD.Scroll   = 0;
  LCD.Contrast = 0x1F;
}

/** TI83LCDDataRD() ******************************************/
/** Read data from TI83 LCD controller VRAM.                **/
/*************************************************************/
byte TI83LCDDataRD(void)
{ 
  byte J,W,*P;
    
  /* Delay reads by one */
  if(LCD.Delay) { LCD.Delay=0;return(0x00); }

  /* Read a byte */
  if(LCD.Status&TI83LCD_8BIT)
  {
    W = 14;
    J = LCD.Col>W? 0x00:LCD.Buffer[LCD.Row*16+LCD.Col];
  }
  else
  {
    W = 19;
    if(LCD.Col>W) J=0x00;
    else
    {
      J    = LCD.Col*6;
      P    = LCD.Buffer+LCD.Row*16+(J>>3);
      J   &= 0x07;
      J    = ((P[0]<<J)|(P[1]>>(8-J)))>>2;
    }
  }

  /* Advance current position */
  switch(LCD.Status&TI83LCD_DIR)
  {
    case 0: LCD.Row = LCD.Row? LCD.Row-1:63;break;
    case 1: LCD.Row = LCD.Row<63? LCD.Row+1:0;break;
    case 2: LCD.Col = (LCD.Col? LCD.Col-1:W)&31;break;
    case 3: LCD.Col = (LCD.Col==W? LCD.Col+1:0)&31;break;
  }

  /* Done */
  return(J);
}

/** TI83LCDDataWR() ******************************************/
/** Write data to TI83 LCD controller VRAM.                 **/
/*************************************************************/
void TI83LCDDataWR(register byte V)
{
  byte W,J,*P;
 
  /* Write a byte */
  if(LCD.Status&TI83LCD_8BIT)
  {
    W = 14;
    if(LCD.Col<=W) LCD.Buffer[LCD.Row*16+LCD.Col]=V;
  }
  else
  {
    W = 19;
    if(LCD.Col<=W)
    {
      J    = LCD.Col*6;
      P    = LCD.Buffer+LCD.Row*16+(J>>3);
      J   &= 0x07;
      P[0] = (P[0] & ~(0xFC>>J)) | ((V<<2)>>J);

      if(J)
      {
        J    = 8-J;
        P[1] = (P[1] & ~(0xFC<<J)) | ((V<<2)<<J);
      }
    }
  }
 
  /* Advance current position */
  switch(LCD.Status&TI83LCD_DIR)
  { 
    case 0: LCD.Row = LCD.Row? LCD.Row-1:63;break;
    case 1: LCD.Row = LCD.Row<63? LCD.Row+1:0;break;
    case 2: LCD.Col = (LCD.Col? LCD.Col-1:W)&31;break;
    case 3: LCD.Col = (LCD.Col==W? 0:LCD.Col+1)&31;break;
  }
} 
  
/** TI83LCDCtrl() ********************************************/
/** Send command to TI83 LCD controller.                    **/
/*************************************************************/
void TI83LCDCtrl(register byte V)
{
  /* Delay next LCD data read */
  LCD.Delay=1;

  /* Contrast */
  if(V>=0xC0) TI83Colors(LCD.Contrast=V&0x3F);
   
  /* Accessed row */
  else if(V>=0x80) LCD.Row=V&0x3F;
    
  /* Vertical scroll */
  else if(V>=0x40) LCD.Scroll=V&0x3F;

  /* Accessed column */
  else if(V>=0x20) LCD.Col=V&0x0F;
  
  /* Other commands */
  else switch(V)
  {
    case 0: LCD.Status&=~TI83LCD_8BIT;break;
    case 1: LCD.Status|=TI83LCD_8BIT;break;
    case 2: LCD.Status&=~TI83LCD_ON;break;
    case 3: LCD.Status|=TI83LCD_ON;break;
    case 4:
    case 5:
    case 6:
    case 7: LCD.Status=(LCD.Status&~TI83LCD_DIR)|(V-4);break;
  }
}

/** SIOExchange() ********************************************/
/** Exchange signals with the other end of the link.        **/
/*************************************************************/
byte SIOExchange(byte Vout)
{
  return(0x03);
#if 0
  static byte MakeConnection = 1;
  byte Vin;

  /* Make connection when calling this function for the first time */
  if(MakeConnection)
  {
    MakeConnection=0;
    if(LinkPeer)
    {
      if(Verbose) LOGE("Connecting to %s:%d...",LinkPeer,LinkPort);
      fflush(stdout);
      Vin=NETConnect(LinkPeer,LinkPort);
      if(Verbose) LOGE("%s\n",Vin? "OK":"FAILED");
    } 
  }

  /* Make sure other end only receives two bits */
  Vout&=0x03;

  /* Try exchanging signals with the other end */
  return(NETExchange(&Vin,&Vout,1)? Vin:0x03);  
#endif //0
}
