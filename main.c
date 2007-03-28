/* Name: main.c
 * Project: Multiple NES/SNES to USB converter
 * Author: Raphael Assenat <raph@raphnet.net>
 * Copyright: (C) 2007 Raphael Assenat <raph@raphnet.net>
 * License: Proprietary, free under certain conditions. See Documentation.
 * Tabsize: 4
 * Comments: Based on HID-Test by Christian Starkjohann
 */

#define F_CPU   12000000L  

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <string.h>

#include "usbdrv.h"
#include "oddebug.h"
#include "gamepad.h"

#include "snes.h"

#include "leds.h"
#include "devdesc.h"

int usbCfgSerialNumberStringDescriptor[] PROGMEM = {
 	USB_STRING_DESCRIPTOR_HEADER(USB_CFG_SERIAL_NUMBER_LENGTH),
 	'1', '0', '0', '0'
 };

static Gamepad *curGamepad;


/* ----------------------- hardware I/O abstraction ------------------------ */

static void hardwareInit(void)
{
	uchar	i, j;

	// init port C as input with pullup
	DDRC = 0x00;
	PORTC = 0xff;
	
	/* 1101 1000 bin: activate pull-ups except on USB lines 
	 *
	 * USB signals are on bit 0 and 2. 
	 *
	 * Bit 1 is connected with bit 0 (rev.C pcb error), so the pullup
	 * is not enabled.
	 * */
	PORTD = 0xf8;   

	/* Usb pin are init as outputs */
	DDRD = 0x01 | 0x04;    

	
	j = 0;
	while(--j){     /* USB Reset by device only required on Watchdog Reset */
		i = 0;
		while(--i); /* delay >10ms for USB reset */
	}
	DDRD = 0x00;    /* 0000 0000 bin: remove USB reset condition */
			/* configure timer 0 for a rate of 12M/(1024 * 256) = 45.78 Hz (~22ms) */
	TCCR0 = 5;      /* timer 0 prescaler: 1024 */

	TCCR2 = (1<<WGM21)|(1<<CS22)|(1<<CS21)|(1<<CS20);
	OCR2 = 196; // for 60 hz

}

static uchar    reportBuffer[12];    /* buffer for HID reports */



/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

static uchar    idleRate;           /* in 4 ms units */

static uchar reportPos=0;

uchar	usbFunctionSetup(uchar data[8])
{
	usbRequest_t    *rq = (void *)data;

	usbMsgPtr = reportBuffer;
	if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    /* class request type */
		if(rq->bRequest == USBRQ_HID_GET_REPORT){  /* wValue: ReportType (highbyte), ReportID (lowbyte) */
			/* we only have one report type, so don't look at wValue */
			reportPos=0;
			//curGamepad->buildReport(reportBuffer);
			//return curGamepad->report_size;
			return 0xff;
		}else if(rq->bRequest == USBRQ_HID_GET_IDLE){
			usbMsgPtr = &idleRate;
			return 1;
		}else if(rq->bRequest == USBRQ_HID_SET_IDLE){
			idleRate = rq->wValue.bytes[1];
		}
	}else{
	/* no vendor specific requests implemented */
	}
	return 0;
}

uchar usbFunctionRead(uchar *data, uchar len)
{
	char i,c;
	for (c=0; reportPos < sizeof(reportBuffer) && c<len; c++, reportPos++)
	{
		*data = reportBuffer[reportPos];
		i++;
	}
	return c;
}

/* ------------------------------------------------------------------------- */

int main(void)
{
	char must_report = 0, first_run = 1;
	uchar   idleCounter = 0;
//	int run_mode;

	// led pin as output
//	DDRD |= 0x20;

#if 0
	/* Dip switch common: DB0, outputs: DB1 and DB2 */
	DDRB |= 0x01;
	DDRB &= ~0x06; 
	
	PORTB |= 0x06; /* enable pull up on DB1 and DB2 */
	PORTB &= ~0x01; /* Set DB0 to low */

	_delay_ms(10); /* let pins settle */
	run_mode = (PINB & 0x06)>>1;

	switch(run_mode)
	{
		default:
		case 3:
			curGamepad = snesGetGamepad();
			break;
	}
#endif
	
	curGamepad = snesGetGamepad();

	// configure report descriptor according to
	// the current gamepad
	rt_usbHidReportDescriptor = curGamepad->reportDescriptor;
	rt_usbHidReportDescriptorSize = curGamepad->reportDescriptorSize;

	if (curGamepad->deviceDescriptor != 0)
	{
		rt_usbDeviceDescriptor = (void*)curGamepad->deviceDescriptor;
		rt_usbDeviceDescriptorSize = curGamepad->deviceDescriptorSize;
	}
	else
	{
		// use descriptor from devdesc.c
		//
		rt_usbDeviceDescriptor = (void*)usbDescrDevice;
		rt_usbDeviceDescriptorSize = getUsbDescrDevice_size();
	}

	//wdt_enable(WDTO_2S);
	hardwareInit();
	curGamepad->init();
	odDebugInit();
	usbInit();
	sei();
	DBG1(0x00, 0, 0);

	
	for(;;){	/* main event loop */
		wdt_reset();

		// this must be called at each 50 ms or less
		usbPoll();

		if (first_run) {
			curGamepad->update();
			first_run = 0;
		}

		if(TIFR & (1<<TOV0)){   /* 22 ms timer */
			TIFR = 1<<TOV0;
			if(idleRate != 0){
				if(idleCounter > 4){
					idleCounter -= 5;   /* 22 ms in units of 4 ms */
				}else{
					idleCounter = idleRate;
					must_report = 1;
				}
			}
		}

		if (TIFR & (1<<OCF2))
		{
			TIFR = 1<<OCF2;
			if (!must_report)
			{
				curGamepad->update();
				if (curGamepad->changed()) {
					must_report = 1;
				}
			}

		}
		
			
		if(must_report)
		{
			if (usbInterruptIsReady())
			{ 	
				int reported=0;
				unsigned char empty=0;

				curGamepad->buildReport(reportBuffer);
				reportPos = 0;

				while (reported < curGamepad->report_size)
				{
					int cur_report_siz;

					
					if (curGamepad->report_size - reported >= 8)
						cur_report_siz = 8;
					else
						cur_report_siz = curGamepad->report_size - reported;

					usbSetInterrupt(&reportBuffer[reported], cur_report_siz);

					while (!usbInterruptIsReady())
					{
						usbPoll();
						wdt_reset();
					}
					
					reported += cur_report_siz;
				}

				usbSetInterrupt(&empty, 0);				
				must_report = 0;
			}
		}
	}
	return 0;
}

/* ------------------------------------------------------------------------- */
