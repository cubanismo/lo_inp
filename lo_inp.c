/*
	BJL-uploader for Linux
	Copyright (c) 1997, Joe Britt

	Freely distributable and modifiable.
	I only ask that if you use this as the basis of some other work,
	you give me credit for this original version.  Thanks.

	Special thanks to 42Bastian, for being extremely helpful in debugging
	the communication with the BJL-ROM in the Jag.  He's the man!

		JOE       Joe Britt
		42BS      42Bastian Schick <elw5basc(at)gp.fht-esslingen.de>
		JBS       John Sohn (jsohn@ee.net)
		ZS        Zerosquare (Jagware - jagware.org)

	Change History:

	30 Oct 97 JOE     First version.
	31 Oct 97 42BS    added DJGPP support, but don't know if it works...
	 1 Dec 97 JBS     added Win95 console support and enhanced DGJPP functionality
	10 Jan 98 JBS     supports linking with Windows 95 GUI application
	14 Feb 98 42BS    GEMDOS - relocator debuged / trans-routines in bjl.s

     4 Jan 99 42BS    removed win95 stuff, tested under Linux.
                      repl. gWait by port-accesse (out is 1usec, ever)

    15 Mar 06 ZS      converted hardware I/O to use inpout32.dll (to make it work under WinNT/2000/XP)
                      ported functionality from bjl.s to this file (bjl.s is no longer needed)
                      tweaked the wait handling in SendNibble() (should be more reliable)
                      added a progress indicator
                      bug fixed : the file is not sent if the loading failed

    12 Jul 06 ZS      modified SendWord to match the one in the original bjl.s.
                      added a 100 ms delay in InitPortNormal() and InitPortHyper().
                      raised the process' and thread's priorities by default (see PriorityBoost() and the new "-x" option.)
                      added the date in the "usage" info, and fixed a typo.

     9 Nov 07 ZS      made it Linux-compatible again
                      inline low-level communications functions to (try to) reduce latency
*/


/* gWait was used to adapt PC-speed,

   486DX4-100    200
   PII-450       2000 ????
*/


// ZS
/* #define BJL_S            // use asm-file  */

// ZS
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <inttypes.h>

// ZS
#ifdef __WIN32__
	#include <windows.h>   
	#include "inpout32.h"  
	#define HAVE_X86_IO 1
	#define inb(a)          Inp32((a))
	#define outb(a,b)       Out32((b),(a))
#endif

// ZS
#ifdef __linux__
	#if defined(__i386__) || defined(__x86_64__)
		#include <sys/io.h>
		#define HAVE_X86_IO 1
	#endif
	#include <unistd.h>
	#include <errno.h>
#endif


// ZS
/*
#ifdef DJGPP
#  include <inlines/pc.h>
#  define outb(data,port) outportb((port),(data))
#  define inb(port)       inportb(port)
#  define ioperm(a,b,c)   (0)
#endif
*/

#if !defined(HAVE_X86_IO)
#error "No I/O mechanism available.  Currently only x86 platforms supported"
#endif
typedef unsigned char Boolean;


#define true   1
#define false  0

#define kLPT1Base    0x378
#define kLPT2Base    0x278

/*
from http://www.paranoia.com/~filipg/HTML/LINK/PORTS/F_PARALLEL5.html#PARALLEL_008

STATUS
	7 6 5 4 3 2 1 0
	* . . . . . . .  Busy . . (pin 11), high=0, low=1 (inverted)
	. * . . . . . .  Ack  . . (pin 10), high=1, low=0 (true)
	. . * . . . . .  No paper (pin 12), high=1, low=0 (true)
	. . . * . . . .  Selected (pin 13), high=1, low=0 (true)
	. . . . * . . .  Error. . (pin 15), high=1, low=0 (true)
	. . . . . * * *  Undefined

CONTROL
	7 6 5 4 3 2 1 0
	* * . . . . . .  Unused (undefined on read, ignored on write)
	. . * . . . . .  Bidirectional control, see below
	. . . * . . . .  Interrupt control, 1=enable, 0=disable
	. . . . * . . .  Select . . (pin 17), 1=low, 0=high (inverted)
	. . . . . * . .  Initialize (pin 16), 1=high, 0=low (true)
	. . . . . . * .  Auto Feed  (pin 14), 1=low, 0=high (inverted)
	. . . . . . . *  Strobe . . (pin 1),  1=low, 0=high (inverted)
*/

#define kDataReg     0
#define kStatusReg   1               /* BUSY is bit 7 */
#define kControlReg  2               /* STB is bit 0 */


// ZS
static void BoostPriority(void);


/* internal prototypes */

static unsigned long relocate(uint8_t * pdata, unsigned long * len, uint32_t *addr, unsigned long skip);
static void SendFile( uint32_t addr, unsigned long skip , uint8_t * buf, unsigned long len);

static uint32_t ReverseEndian(uint32_t d)
{
  return  ((d << 24) |
	  ((d & 0x0000ff00) << 8) |
	  ((d & 0x00ff0000) >> 8) |
	  (d >> 24));
}

#define BE_TO_LE(_x)    ReverseEndian(_x)
#define LE_TO_BE(_x)    ReverseEndian(_x)


#define BE_TO_LEW(_x)    ( (uint16_t)( ((_x)>>8)|((_x)<<8) ) )
#define LE_TO_BEW(_x)    BE_TO_LEW(_x)


static Boolean ParseCmds( unsigned long argc, char *argv[] );
static void ArgValue( char *s, unsigned long *value );


static unsigned long gBase = kLPT1Base;                 /* default to LPT2 */
static uint32_t gBaseAddr = 0x4000;                     /* default base */
static unsigned long gHeaderSkip = 0;                   /* default header size */ /* was 28,changed 42BS */
static unsigned long gWait = 1;
static int   gSwitchCommand = 1;
static int   gStartLoader = 0;
static int   g4BitMode = 1;
static int   gDebugUpload = 0;
static char *gImageName = "Joe Britt";
static int   HighPriority = 1;

void (*SendNibble)(uint8_t c) = NULL;
void (*SendWord)(uint16_t w) = NULL;
void (*InitPortNormal)(void) = NULL;
void (*InitPortHyper)(void) = NULL;

// ZS
#ifdef __WIN32__
	#define Delay(x) 	Sleep(x)
#endif

#ifdef __linux__
	#define Delay(x) 	usleep((x)*1000)
#endif


#if defined(HAVE_X86_IO)
// ZS
static inline void waitX86()
{
    int a;
    for (a = gWait; a; a--) outb(0x00, 0x80);
}

/*
From LOADER.DOC:

1) Wait for BUSY to be low => inactive
2) Send LSB
3) Set /STROBE to low => active
4) Wait for BUSY high => active
5) Set /STROBE to high
6) continue for the next three bytes
*/
// ZS
static void SendNibbleX86(uint8_t c)
{
  while( !(inb( gBase+kStatusReg ) & 0x80) )       /* wait for BUSY = 0 */
    ;
  waitX86();                                       // ZS
  outb( c, gBase+kDataReg );                       /* drive data */
  waitX86();                                       // ZS
  outb( 0x01, gBase+kControlReg );                 /* output, /STB = 0 */
  waitX86();                                       // ZS
  while( (inb( gBase+kStatusReg ) & 0x80) )        /* wait for BUSY = 1 */
    ;
  waitX86();                                       // ZS

  // ZS
  // outb( 0x20, gBase+kControlReg );                 /* input, /STB is high */
  outb( 0x00, gBase+kControlReg );                   /* input, /STB is high */

  waitX86();                                       // ZS
}
#endif // defined(HAVE_X86_IO)


// ZS
static inline void SendByte(uint8_t c)
{
    if (g4BitMode)
    {
        SendNibble(c << 4);
        SendNibble(c & 0xF0);
    }
    else
    {
        SendNibble(c);
    }
}


// ZS
static inline void SendLongHyper(uint32_t w)
{
	SendByte(w & 0xFF);
	w >>= 8;
	SendByte(w & 0xFF);
	w >>= 8;
	SendByte(w & 0xFF);
	w >>= 8;
	SendByte(w & 0xFF);
}


static inline void SendLong(uint32_t w)
{
// ZS
/*
register int a;

  for( a = gWait; a; --a)
		;
*/
  SendByte( (uint8_t)w );
  SendByte( (uint8_t)(w >> 8) );
  SendByte( (uint8_t)(w >> 16) );
  SendByte( (uint8_t)(w >> 24) );
}

#if defined(HAVE_X86_IO)
// ZS
static void SendWordX86(uint16_t w)
{
	unsigned int i, j;

	for (j = 1; j <= 3; j++) w = ((w << 1) & 0xFFFF) | (w >> 15);
	for (i = 1; i <= 8; i++)
	{
        while (inb(gBase+kStatusReg) & 0x80);
        waitX86();
        outb(w & 6, gBase+kDataReg);					
        waitX86();
        outb(0, gBase+kControlReg);
        waitX86();
        for (j = 1; j <= 2; j++) w = ((w << 1) & 0xFFFF) | (w >> 15);
		while (!(inb(gBase+kStatusReg) & 0x80));	
        waitX86();
        outb(1, gBase+kControlReg);					
        waitX86();
	}
}


// ZS
static void InitPortNormalX86()
{
	outb(0, gBase+kDataReg);
	outb(0, gBase+kStatusReg);
	outb(1, gBase+kControlReg);
	outb(1, gBase+kControlReg);
	Delay(100);                    // ZS
}

// ZS
static void InitPortHyperX86()
{
	outb(0, gBase+kDataReg);
	outb(0, gBase+kStatusReg);
	outb(0, gBase+kControlReg);
	Delay(100);                    // ZS
}
#endif // defined(HAVE_X86_IO)


static int LoadFile( char *fn, uint32_t *addr, unsigned long *skip, uint8_t **buf)
{
  FILE *fp;
  unsigned long len;
  size_t nr;

  if ( (fp = fopen( fn, "rb" )) == (FILE *)NULL ){
    printf("Couldn't open file '%s'!\n", gImageName);
    return -1;
  }

  fseek(fp, 0, SEEK_END);

  len = ftell(fp);

  printf("%s is %ld bytes long\n", fn, len);

  fseek( fp, 0, SEEK_SET );   /* 42BS */

  *buf = (uint8_t *)malloc( len );

  if ( *buf == NULL ){
    printf("LoadFile: couldn't alloc buf!\n");
    return -1;
  }

  nr = fread( *buf, sizeof((*buf)[0]), len, fp );
  if ( nr != len) {
    printf("LoadFile: couldn't read entire file from disk: len = %lu, read = %zu\n", len, nr);
    free(*buf);
    *buf = NULL;
    return -1;
  }

  *skip = relocate( *buf, &len , addr, *skip);    // relocate program

  return len;
}



int main( int argc,char *argv[] )
{
  if ( !ParseCmds( argc, argv ) )
  {
    printf("Lo_inp - November 9, 2007\n");
    printf("\n");
    printf("usage: loader [-s skipbytes] [-w wait] [-b baseaddr] [-p lptbase] [-n] [-8] <filename>\n");
    printf("       skipbytes is # bytes in header to skip.   default = %ld\n", gHeaderSkip);
    printf("       baseaddr is addr to upload to.            default = 0x%08x\n", gBaseAddr);
    printf("       lptbase specifies base addr of LPT port.  default = 0x%lx\n", gBase);
    printf("       wait - counter between longs.             default = %ld\n",gWait);
    printf("       -n => don`t send switch-command\n");
    printf("       -8 => use 8Bit transmission (4Bit is default)\n");
    printf("       -x => do not use high-priority mode.\n");

    return -1;
  }

#if defined(HAVE_X86_IO)
  SendWord = &SendWordX86;
  SendNibble = &SendNibbleX86;
  InitPortNormal = &InitPortNormalX86;
  InitPortHyper = &InitPortHyperX86;
#endif // defined(HAVE_X86_IO)
 
  // ZS
  #ifdef __WIN32__
  	if (!Inpout32_Init())
	{
		printf("Couldn't load Inpout32.dll, which is needed to access the parallel port.\n");
		printf("\n");
		printf("Check that this file is present either in the current folder, or in your Windows\\System folder.\n");

		return -1;
	}
  #endif

  // ZS
  #if defined(__linux__) && defined(HAVE_X86_IO)
  	// port 0x80 is for timing
  	if ( (ioperm(gBase, 3, true) != 0) || (ioperm(0x80,1,true) != 0) )
	{
		printf("Couldn't get access to requested I/O ports.\n");
		printf("\n");
		printf("Please check that no other hardware or software is using the parallel port,\n");
		printf("and that you gave root permissions to this program.\n");
		
		return -1;
	}  
  #endif // defined(__linux__) && defined(HAVE_X86_IO)

  // ZS	 
  if (HighPriority) BoostPriority(); 

  {
    uint8_t *buffer;

    // ZS
    /* unsigned long length; */
    int length;

    if ( (length = LoadFile( gImageName,
			     &gBaseAddr,
			     &gHeaderSkip,
			     &buffer ) ) > 0 ){
      SendFile( gBaseAddr, gHeaderSkip , buffer, length);
    }
  }

  // ZS
  #if defined(__linux__) && defined(HAVE_X86_IO)
    ioperm(gBase, 3, false);	
	ioperm(0x80, 1, false);
  #endif  

  // ZS
  return 0;
}


static void ArgValue( char *s, unsigned long *value )
{
  if ( s && isdigit(s[0]) )
  {
    if ( s[1] == 'x' )
      sscanf( s, "%x", (int *)value );
    else
      sscanf( s, "%d", (int *)value );
  }
}


static Boolean ParseCmds( unsigned long argc, char *argv[] )
{
   char *s;
   unsigned long cnt;
   unsigned long tmp;

   if ( argc == 1 )
     return false;


  for ( cnt = 1; cnt < argc; cnt++ )
  {
    s = argv[cnt];
    if ( s[0] == '-' )
      switch ( s[1] )
      {
      case 's':
	cnt++;
	ArgValue( argv[cnt], &gHeaderSkip );
	break;

      case 'b':
	cnt++;
	ArgValue( argv[cnt], &tmp );
        gBaseAddr = tmp;
	break;

      case 'p':
	cnt++;
	ArgValue( argv[cnt], &gBase );
	break;

      case 'w':
	cnt++;
	ArgValue(argv[cnt], &gWait);
	break;

      case 'n':
	gSwitchCommand = 0;
	break;

      case '8' :
	g4BitMode = 0;
	break;
/*
      case 'f':
	cnt++;
	gImageName = argv[cnt];
	break;
*/

    // ZS
    case 'x':
    HighPriority = 0;
    break;

      default:
	printf("Unknown option !\n");
	exit(1);
      }
    else
    {
      gImageName = s;
    }
  }
  return true;
}


static void SendFile( uint32_t addr, unsigned long skip, uint8_t *buf, unsigned long len)
{
  uint32_t *copy;
  int32_t cksum = 0;
  uint32_t data;
  uint8_t * save_buf = buf;   /* 42BS */
  unsigned long len2;               // ZS

  printf("start is $%x\n"
	 "transmit-lenght is %ld\n", addr, len);

  buf += skip;                 // skip some bytes

  len = ((len + 3) & ~3);      /* round up */

  if ( gSwitchCommand )
    {
      InitPortNormal();

      SendWord(0);               /* syncronize */
      SendWord(0);

      if ( g4BitMode ){
	SendWord(0xb8);
      } else {
	SendWord(0xb7);
      }
      //	InitPortHyper();
    }

  InitPortHyper();
  SendLongHyper( 0 );          /* syncronize */
  // for(;;)
  SendLongHyper( 0x22071970 ); /* send magic number */
  SendLongHyper( addr );       /* send starting addr */
  SendLongHyper( (uint32_t)len+4 );      /* send length */

  len >>= 2;                   /* bytes -> words */
  len2 = len;                  // ZS
  /* lets say :32-bit words :-) */

  copy = (uint32_t *)buf;

  while ( len-- ){
    data = BE_TO_LE(*copy++);
    cksum -= data;
    SendLongHyper( data );
    if (!(len & 0xFF)) printf("uploading... %u %%\r", (unsigned int)((100 * (len2 - len)) / len2));     // ZS
  }
  printf("\n");                 // ZS
  SendLongHyper( cksum );       /* send checksum */

  printf("cksum = %08x\n", cksum);

  free( save_buf );     /* 42BS */
}

/**********************/
/* GEMDOS - relocator */
/**********************/
/* this function sees if it finds a GEMDOS or JServer - header
	 if so, skip,addr and len are adjusted
*/

static unsigned long relocate( uint8_t *pdata, unsigned long *len , uint32_t *addr, unsigned long skip)
{

  if ( (pdata[0] == 0x60) && (pdata[1] == 0x1a) ){ // GEMDOS - header

    if ( *((uint32_t *)(pdata+28)) == BE_TO_LE(0x4a414752) ){ /* JAGR */

      uint16_t type = BE_TO_LEW(*((uint16_t *)(pdata+32)));
      unsigned long new_len;

      if ( type != 2 && type != 3 ){

	printf("Unsupported JServer-Header !\nLeaving ...\n");
	exit(-1);
      }

      *addr = BE_TO_LE( *(uint32_t *)(pdata+34) );  // dest and start address

      if ( (new_len = BE_TO_LE( *(uint32_t *)(pdata+38) ) )){

	*len = new_len;
      }

      if ( type == 2)
	return (28+14);
      else
	return (28+18);   // skip GEMDOS and JServer-header
    } else {
//
// BJL - program
//
      uint32_t textlen,datalen,bss_len,symblen;   /* header info */
      uint32_t offset;
      uint32_t addr2 = *addr;
      uint8_t *reloptr, *pdata2;

//    printf("Relocating BJL ...\n");

      textlen = BE_TO_LE(*(uint32_t *)(pdata+2));
      datalen = BE_TO_LE(*(uint32_t *)(pdata+6));
      bss_len = BE_TO_LE(*(uint32_t *)(pdata+10)); //  no need for this, yet
      symblen = BE_TO_LE(*(uint32_t *)(pdata+14));

//    printf("TEXT(%08x) DATA(%08x) BSS(%08x) SYM(%d)\n",textlen,datalen,bss_len,symblen);

      reloptr = pdata + textlen + datalen + symblen + 28;

      offset = BE_TO_LE(*(uint32_t *)reloptr);   // first long to relocate

//    printf("Offset :%08x\n",offset);

      reloptr += 4;                 // from now only bytes as offset

      pdata2 = pdata + 28 + offset;      // ptr to it

      addr2 = BE_TO_LE(addr2);        // change byte-order

      while ( offset ){              // until offset == 0
      // compute new address
//      *(uint32_t *)pdata2 = LE_TO_BE(BE_TO_LE( *(uint32_t *)pdata2 ) + addr2);
	*(uint32_t *)pdata2 += addr2;       // ha, it works ;-)

      // as long as offset is 1 skip 254 byte
	do{

	  offset = (uint32_t) *reloptr;
	  ++reloptr;
	  if (! --offset)
	    pdata2 += 254;
	}while ( !offset );
	pdata2 += ++offset;           // next address
      }

      *len = textlen + datalen;     // send only text and data , no symbols ...
      return ( 28 );                // skip GEMDOS header
    }
  } else if ( (pdata[0] == 0x01) && (pdata[1] == 0x50) ) {

    printf("COFF-Header found ...\n");
    *addr = BE_TO_LE(*(uint32_t *)(pdata+56));

    skip = BE_TO_LE(*(uint32_t *)(pdata+68));

    *len -= skip;

    return skip;

  } else {

    printf("No Gemdos or COFF header  !\n");

    return (skip);                    /* skip user-defined bytes */
  }
}


// ZS
static void BoostPriority(void)
{
	#ifdef __WIN32__
	    SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
    	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	#endif

	#ifdef __linux__
	errno = 0;
	if ((nice(-20) == -1) && errno)
		printf("WARNING: Failed to boost priority!\n");
	#endif
}
