/*******************************************************/
/* file: ports.c                                       */
/* abstract:  This file contains the routines to       */
/*            output values on the JTAG ports, to read */
/*            the TDO bit, and to read a byte of data  */
/*            from the prom                            */
/* Revisions:                                          */
/* 12/01/2008:  Same code as before (original v5.01).  */
/*              Updated comments to clarify instructions.*/
/*              Add print in setPort for xapp058_example.exe.*/
/*******************************************************/
#include "ports.h"
#include <bcm2835.h>
#include "uart.h"
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

#ifdef DEBUG_MODE
#include "stdio.h"
#endif

#ifdef WIN95PP
#include "conio.h"

#define DATA_OFFSET    (unsigned short) 0
#define STATUS_OFFSET  (unsigned short) 1
#define CONTROL_OFFSET (unsigned short) 2

typedef union outPortUnion {
    unsigned char value;
    struct opBitsStr {
        unsigned char tdi:1;
        unsigned char tck:1;
        unsigned char tms:1;
        unsigned char zero:1;
        unsigned char one:1;
        unsigned char bit5:1;
        unsigned char bit6:1;
        unsigned char bit7:1;
    } bits;
} outPortType;

typedef union inPortUnion {
    unsigned char value;
    struct ipBitsStr {
        unsigned char bit0:1;
        unsigned char bit1:1;
        unsigned char bit2:1;
        unsigned char bit3:1;
        unsigned char tdo:1;
        unsigned char bit5:1;
        unsigned char bit6:1;
        unsigned char bit7:1;
    } bits;
} inPortType;

static inPortType in_word;
static outPortType out_word;
static unsigned short base_port = 0x378;
static int once = 0;
#endif
#ifndef DEBUG_MODE
extern FILE * input;
#else
extern FILE * in;
#endif
#ifndef DEBUG_MODE
const char*   const xsvf_pzErrorName[]  =
{
    "No error",
    "ERROR:  Unknown",
    "ERROR:  TDO mismatch",
    "ERROR:  TDO mismatch and exceeded max retries",
    "ERROR:  Unsupported XSVF command",
    "ERROR:  Illegal state specification",
    "ERROR:  Data overflows allocated MAX_LEN buffer size"
};
#else
extern const char* xsvf_pzErrorName[];
#endif


#define JTAG_TDI 23
#define JTAG_TCK 17
#define JTAG_TMS 24
#define JTAG_TDO 22

void portsInitialize()
{
  if (!bcm2835_init()) {
	printf("\nError initializing IO. Consider using sudo.\n");
	exit(1);
  }
  bcm2835_gpio_fsel(JTAG_TDI, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_set_pud(JTAG_TDI, BCM2835_GPIO_PUD_OFF);
  bcm2835_gpio_fsel(JTAG_TCK, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_set_pud(JTAG_TCK, BCM2835_GPIO_PUD_OFF);
  bcm2835_gpio_fsel(JTAG_TMS, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_set_pud(JTAG_TMS, BCM2835_GPIO_PUD_OFF);
  bcm2835_gpio_fsel(JTAG_TDO, BCM2835_GPIO_FSEL_INPT);
  bcm2835_gpio_set_pud(JTAG_TDO, BCM2835_GPIO_PUD_OFF);
}



/* setPort:  Implement to set the named JTAG signal (p) to the new value (v).*/
/* if in debugging mode, then just set the variables */
void setPort(short p,short val)
{
#ifdef WIN95PP
    /* Old Win95 example that is similar to a GPIO register implementation.
       The old Win95 example maps individual bits of the 
       8-bit register (out_word) to the JTAG signals: TCK, TMS, TDI. 
       */

    /* Initialize static out_word register bits just once */
    if (once == 0) {
        out_word.bits.one = 1;
        out_word.bits.zero = 0;
        once = 1;
    }

    /* Update the local out_word copy of the JTAG signal to the new value. */
    if (p==TMS)
        out_word.bits.tms = (unsigned char) val;
    if (p==TDI)
        out_word.bits.tdi = (unsigned char) val;
    if (p==TCK) {
        out_word.bits.tck = (unsigned char) val;
        (void) _outp( (unsigned short) (base_port + 0), out_word.value );
        /* To save HW write cycles, this example only writes the local copy
           of the JTAG signal values to the HW register when TCK changes. */
    }
#endif
    /* Printing code for the xapp058_example.exe.  You must set the specified
       JTAG signal (p) to the new value (v).  See the above, old Win95 code
       as an implementation example. */
    if (p==TMS) {
        bcm2835_gpio_write(JTAG_TMS, val);
    }
    if (p==TDI) {
        bcm2835_gpio_write(JTAG_TDI, val);
    }
    if (p==TCK) {
        bcm2835_gpio_write(JTAG_TCK, val);
        usleep(1);
#ifdef DEBUG_MODE
    //    printf( "TCK = %d;  TMS = %d;  TDI = %d\n", g_iTCK, g_iTMS, g_iTDI );
#endif
    }
}


/* toggle tck LH.  No need to modify this code.  It is output via setPort. */
void pulseClock()
{
    setPort(TCK,0);  /* set the TCK port to low  */
    setPort(TCK,1);  /* set the TCK port to high */
}


/* readByte:  Implement to source the next byte from your XSVF file location */
/* read in a byte of data from the file */

void readByte(unsigned char *data)
{
#ifndef DEBUG_MODE
   *data = fgetc(input);
#else
   *data = fgetc(in);
#endif
}

/* readTDOBit:  Implement to return the current value of the JTAG TDO signal.*/
/* read the TDO bit from port */
unsigned char readTDOBit()
{
#ifdef WIN95PP
    /* Old Win95 example that is similar to a GPIO register implementation.
       The old Win95 reads the hardware input register and extracts the TDO
       value from the bit within the register that is assigned to the
       physical JTAG TDO signal. 
       */
    in_word.value = (unsigned char) _inp( (unsigned short) (base_port + STATUS_OFFSET) );
    if (in_word.bits.tdo == 0x1) {
        return( (unsigned char) 1 );
    }
#endif
	if(bcm2835_gpio_lev(JTAG_TDO) == HIGH)	{
		return( (unsigned char) 1 );
	}
	else {
		return( (unsigned char) 0 );
	}
}

void output_error(int error_code) {

	printf("\n%s\n",xsvf_pzErrorName[error_code]);

}

/* waitTime:  Implement as follows: */
/* REQUIRED:  This function must consume/wait at least the specified number  */
/*            of microsec, interpreting microsec as a number of microseconds.*/
/* REQUIRED FOR SPARTAN/VIRTEX FPGAs and indirect flash programming:         */
/*            This function must pulse TCK for at least microsec times,      */
/*            interpreting microsec as an integer value.                     */
/* RECOMMENDED IMPLEMENTATION:  Pulse TCK at least microsec times AND        */
/*                              continue pulsing TCK until the microsec wait */
/*                              requirement is also satisfied.               */
void waitTime(long microsec)
{
    //static long tckCyclesPerMicrosec  = 4; /* must be at least 1 */
    //long        tckCycles   = microsec * tckCyclesPerMicrosec;
    //long        i;

    /* This implementation is highly recommended!!! */
    /* This implementation requires you to tune the tckCyclesPerMicrosec 
       variable (above) to match the performance of your embedded system
       in order to satisfy the microsec wait time requirement. */
    //for ( i = 0; i < tckCycles; ++i )
    //{
    //    pulseClock();
    //}
    setPort(TCK, 0);
    usleep(microsec);

#if 0
    /* Alternate implementation */
    /* For systems with TCK rates << 1 MHz;  Consider this implementation. */
    /* This implementation does not work with Spartan-3AN or indirect flash
       programming. */
    if ( microsec >= 50L )
    {
        /* Make sure TCK is low during wait for XC18V00/XCFxxS */
        /* Or, a running TCK implementation as shown above is an OK alternate */
        setPort( TCK, 0 );

        /* Use Windows Sleep().  Round up to the nearest millisec */
        _sleep( ( microsec + 999L ) / 1000L );
    }
    else    /* Satisfy FPGA JTAG configuration, startup TCK cycles */
    {
        for ( i = 0; i < microsec;  ++i )
        {
            pulseClock();
        }
    }
#endif

#if 0
    /* Alternate implementation */
    /* This implementation is valid for only XC9500/XL/XV, CoolRunner/II CPLDs, 
       XC18V00 PROMs, or Platform Flash XCFxxS/XCFxxP PROMs.  
       This implementation does not work with FPGAs JTAG configuration. */
    /* Make sure TCK is low during wait for XC18V00/XCFxxS PROMs */
    /* Or, a running TCK implementation as shown above is an OK alternate */
    setPort( TCK, 0 );
    /* Use Windows Sleep().  Round up to the nearest millisec */
    _sleep( ( microsec + 999L ) / 1000L );
#endif
}
