/*
 * Elan 33059 trackpad to USB mouse
 * Copyright (C) 2021 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */




// PIC18F2450 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1L
#pragma config PLLDIV = 1       // PLL Prescaler Selection bits (No prescale (4 MHz oscillator input drives PLL directly))
#pragma config CPUDIV = OSC1_PLL2// System Clock Postscaler Selection bits ([Primary Oscillator Src: /1][96 MHz PLL Src: /2])
#pragma config USBDIV = 1       // USB Clock Selection bit (used in Full-Speed USB mode only; UCFG:FSEN = 1) (USB clock source comes directly from the primary oscillator block with no postscale)

// CONFIG1H
#pragma config FOSC = HS        // Oscillator Selection bits (HS oscillator (HS))
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor disabled)
#pragma config IESO = OFF       // Internal/External Oscillator Switchover bit (Oscillator Switchover mode disabled)

// CONFIG2L
#pragma config PWRT = ON        // Power-up Timer Enable bit (PWRT enabled)
#pragma config BOR = ON         // Brown-out Reset Enable bits (Brown-out Reset enabled in hardware only (SBOREN is disabled))
#pragma config BORV = 21        // Brown-out Reset Voltage bits (2.1V)
#pragma config VREGEN = ON     // USB Voltage Regulator Enable bit (USB voltage regulator disabled)

// CONFIG2H
#pragma config WDT = ON         // Watchdog Timer Enable bit (WDT enabled)
#pragma config WDTPS = 32768    // Watchdog Timer Postscale Select bits (1:32768)

// CONFIG3H
#pragma config PBADEN = ON      // PORTB A/D Enable bit (PORTB<4:0> pins are configured as analog input channels on Reset)
#pragma config LPT1OSC = OFF    // Low-Power Timer 1 Oscillator Enable bit (Timer1 configured for higher power operation)
#pragma config MCLRE = ON       // MCLR Pin Enable bit (MCLR pin enabled; RE3 input pin disabled)

// CONFIG4L
#pragma config STVREN = OFF     // Stack Full/Underflow Reset Enable bit (Stack full/underflow will not cause Reset)
#pragma config LVP = OFF        // Single-Supply ICSP Enable bit (Single-Supply ICSP disabled)
#pragma config BBSIZ = BB1K     // Boot Block Size Select bit (1KW Boot block size)
#pragma config XINST = OFF      // Extended Instruction Set Enable bit (Instruction set extension and Indexed Addressing mode disabled (Legacy mode))

// CONFIG5L
#pragma config CP0 = OFF        // Code Protection bit (Block 0 (000800-001FFFh) or (001000-001FFFh) is not code-protected)
#pragma config CP1 = OFF        // Code Protection bit (Block 1 (002000-003FFFh) is not code-protected)

// CONFIG5H
#pragma config CPB = OFF        // Boot Block Code Protection bit (Boot block (000000-0007FFh) or (000000-000FFFh) is not code-protected)

// CONFIG6L
#pragma config WRT0 = OFF       // Write Protection bit (Block 0 (000800-001FFFh) or (001000-001FFFh) is not write-protected)
#pragma config WRT1 = OFF       // Write Protection bit (Block 1 (002000-003FFFh) is not write-protected)

// CONFIG6H
#pragma config WRTC = OFF       // Configuration Register Write Protection bit (Configuration registers (300000-3000FFh) are not write-protected)
#pragma config WRTB = OFF       // Boot Block Write Protection bit (Boot block (000000-0007FFh) or (000000-000FFFh) is not write-protected)

// CONFIG7L
#pragma config EBTR0 = OFF      // Table Read Protection bit (Block 0 (000800-001FFFh) or (001000-001FFFh) is not protected from table reads executed in other blocks)
#pragma config EBTR1 = OFF      // Table Read Protection bit (Block 1 (002000-003FFFh) is not protected from table reads executed in other blocks)

// CONFIG7H
#pragma config EBTRB = OFF      // Boot Block Table Read Protection bit (Boot block (000000-0007FFh) or (000000-000FFFh) is not protected from table reads executed in other blocks)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pic18f2450.h>

#define CLOCKSPEED 48000000


// PS2 touchpad pins
#define CLK_LAT  LATCbits.LATC1
#define CLK_TRIS TRISCbits.TRISC1
#define CLK_PORT PORTCbits.RC1

#define DAT_LAT LATCbits.LATC0
#define DAT_TRIS TRISCbits.TRISC0
#define DAT_PORT PORTCbits.RC0

#define LBUTTON_TRIS TRISBbits.TRISB7
#define LBUTTON_PORT PORTBbits.RB7
#define RBUTTON_TRIS TRISBbits.TRISB6
#define RBUTTON_PORT PORTBbits.RB6



typedef union 
{
	struct
	{
		unsigned interrupt_complete : 1;
        unsigned touchpad_error : 1;
        unsigned overflow : 1;
	};
	
	unsigned char value;
} flags_t;

typedef struct
{
    uint8_t accum;
    uint8_t value;
    uint8_t changed;
} button_state_t;
button_state_t left_button;
button_state_t right_button;
#define DEBOUNCE 10

#define HZ 1000
#define TIMER0_PERIOD (CLOCKSPEED / 4 / 32 / HZ)

flags_t flags;
uint16_t tick;

// UART ------------------------------------------------------------------------

#define UART_OUT_SIZE 256
#define UART_IN_SIZE 8
static uint16_t serial_in_count = 0;
static uint16_t serial_in_ptr = 0;
static uint16_t serial_in_ptr2 = 0;
static uint8_t serial_in_buffer[UART_IN_SIZE];
static uint16_t serial_out_count = 0;
static uint16_t serial_out_ptr = 0;
static uint16_t serial_out_ptr2 = 0;
static uint8_t serial_out_buffer[UART_OUT_SIZE];
void init_uart()
{
    RCSTA = 0b10010000;
    TXSTA = 0b00100100;
    BAUDCTL = 0b00001000;
    SPBRG = CLOCKSPEED / 4 / 115200;
    PIR1bits.RCIF = 0;
    PIE1bits.RCIE = 1;
}

void handle_uart_rx()
{
    flags.interrupt_complete = 0;
// clear interrupt
    uint8_t c = RCREG;
    if(serial_in_count < UART_IN_SIZE)
    {
        serial_in_buffer[serial_in_ptr++] = c;
        serial_in_count++;
        if(serial_in_ptr >= UART_IN_SIZE)
        {
            serial_in_ptr = 0;
        }
    }
}

void handle_uart()
{
// clear the overflow bit
    if(RCSTAbits.OERR)
    {
        RCSTAbits.OERR = 0;
        RCSTAbits.CREN = 0;
        RCSTAbits.CREN = 1;
    }

    if(PIR1bits.TXIF)
    {
        if(serial_out_count > 0)
        {
            TXREG = serial_out_buffer[serial_out_ptr2++];
            if(serial_out_ptr2 >= UART_OUT_SIZE)
            {
                serial_out_ptr2 = 0;
            }
            serial_out_count--;
        }
    }
}

void flush_uart()
{
    while(serial_out_count)
    {
        handle_uart();
    }
}


void print_byte(uint8_t c)
{
	if(serial_out_count < UART_OUT_SIZE)
	{
		serial_out_buffer[serial_out_ptr++] = c;
		serial_out_count++;
		if(serial_out_ptr >= UART_OUT_SIZE)
		{
			serial_out_ptr = 0;
		}
	}
}

void print_text(const uint8_t *s)
{
	while(*s != 0)
	{
		print_byte(*s);
		s++;
	}
}

void print_number_nospace(uint16_t number)
{
	if(number >= 10000) print_byte('0' + (number / 10000));
	if(number >= 1000) print_byte('0' + ((number / 1000) % 10));
	if(number >= 100) print_byte('0' + ((number / 100) % 10));
	if(number >= 10) print_byte('0' + ((number / 10) % 10));
	print_byte('0' + (number % 10));
}

void print_number(uint16_t number)
{
    print_number_nospace(number);
   	print_byte(' ');
}

const uint8_t hex_table[] = { 
    '0', '1', '2', '3', '4', '5', '6', '7', 
    '8', '9', 'a', 'b', 'c', 'd', 'e','f'
};

void print_hex2(uint8_t number)
{
    print_byte(hex_table[number >> 4]);
    print_byte(hex_table[number & 0xf]);
   	print_byte(' ');
}

void print_bin(uint8_t number)
{
	print_byte((number & 0x80) ? '1' : '0');
	print_byte((number & 0x40) ? '1' : '0');
	print_byte((number & 0x20) ? '1' : '0');
	print_byte((number & 0x10) ? '1' : '0');
	print_byte((number & 0x8) ? '1' : '0');
	print_byte((number & 0x4) ? '1' : '0');
	print_byte((number & 0x2) ? '1' : '0');
	print_byte((number & 0x1) ? '1' : '0');
}


// USB HID ---------------------------------------------------------------------

#define VENDOR_ID 0x04d8
#define PRODUCT_ID 0x000b
#define USB_WORD(x) (uint8_t)((x) & 0xff), (uint8_t)((x) >> 8)
#define EP0_SIZE 8
#define DESCRIPTOR_DEV     0x01
#define DESCRIPTOR_CFG     0x02
#define DESCRIPTOR_STR     0x03
#define DESCRIPTOR_INTF    0x04
#define DESCRIPTOR_EP      0x05
/* Class Descriptor Types */
#define DSC_HID         0x21
#define DSC_RPT         0x22
#define DSC_PHY         0x23
/* HID Interface Class Code */
#define HID_INTF                    0x03
/* HID Interface Class SubClass Codes */
#define BOOT_INTF_SUBCLASS          0x01
/* HID Interface Class Protocol Codes */
#define HID_PROTOCOL_NONE           0x00
#define HID_PROTOCOL_KEYBOARD       0x01
#define HID_PROTOCOL_MOUSE          0x02
#define HID_NUM_OF_DSC 1
#define _EP_IN 0x80
#define HID_EP 0x01
#define _INTERRUPT        0x03            //Interrupt Transfer
#define SETUP_TOKEN 0b00001101
// matches report size
#define EP1_SIZE 0x8


const uint8_t usb_descriptor[] = 
{
    0x12,                   // Size of this descriptor in bytes
    DESCRIPTOR_DEV,         // DEVICE descriptor type
    USB_WORD(0x0200),       // USB Spec Release Number in BCD format
    0x00,                   // Class Code
    0x00,                   // Subclass code
    0x00,                   // Protocol code
    EP0_SIZE,               // Max packet size for EP0, see usb_config.h
    USB_WORD(VENDOR_ID),    // Vendor ID
    USB_WORD(PRODUCT_ID),   // Product ID: Mouse in a circle fw demo
    USB_WORD(0x0003),       // Device release number in BCD format
    0x01,                   // Manufacturer string index 
    0x02,                   // Product string index
    0x00,                   // Device serial number string index
    0x01,                   // Number of possible configurations
};

const uint8_t usb_config1[] =
{
    0x09,                   // Size of this descriptor in bytes
    DESCRIPTOR_CFG,         // CONFIGURATION descriptor type
    USB_WORD(0x0022),       // Total length of data for this cfg
    1,                      // Number of interfaces in this cfg
    1,                      // Index value of this configuration
    0,                      // Configuration string index
    (1 << 7),               // Attributes, see usb_device.h
    50,                     // Max power consumption (2X mA)

    /* Interface Descriptor */
    0x09,                   // Size of this descriptor in bytes
    DESCRIPTOR_INTF,        // INTERFACE descriptor type
    0,                      // Interface Number
    0,                      // Alternate Setting Number
    1,                      // Number of endpoints in this intf
    HID_INTF,               // Class code
    BOOT_INTF_SUBCLASS,     // Subclass code
    HID_PROTOCOL_MOUSE,     // Protocol code
    0,                      // Interface string index

    /* HID Class-Specific Descriptor */
    0x09,                   // Size of this descriptor in bytes RRoj hack
    DSC_HID,                // HID descriptor type
    USB_WORD(0x0111),       // HID Spec Release Number in BCD format (1.11)
    0x00,                   // Country Code (0x00 for Not supported)
    HID_NUM_OF_DSC,         // Number of class descriptors, see usbcfg.h
    DSC_RPT,                // Report descriptor type
    USB_WORD(54),           // Size of the report descriptor
    
    /* user endpoint descriptors */
    0x07,                       // Size of this descriptor in bytes
    DESCRIPTOR_EP,              // Endpoint Descriptor
    HID_EP | _EP_IN,            // EndpointAddress
    _INTERRUPT,                 // Attributes
    USB_WORD(EP1_SIZE),         // size
    0x01                        // Interval
};

// Language code string descriptor
const uint8_t sd000[] = 
{
    0x04,      // length
    DESCRIPTOR_STR,    // descriptor type
    USB_WORD(0x0409)   // text
};

// Manufacturer string descriptor
const uint8_t sd001[] = 
{
    22,       // length
    DESCRIPTOR_STR,    // descriptor type
    'M', 0, 'c', 0, 'L', 0, 'i', 0, 'o', 0, 'n', 0, 'h', 0, 'e', 0, 'a', 0, 'd', 0,
};


// Product string descriptor
const uint8_t sd002[] = 
{
    18,
    DESCRIPTOR_STR,
    'T', 0, 'r', 0, 'a', 0, 'c', 0, 'k', 0, 'p', 0, 'a', 0, 'd', 0
};

// from Arduino mouse library
const uint8_t _hidReportDescriptor[] = 
{
//  Mouse
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)  // 54
    0x09, 0x02, // USAGE (Mouse)
    0xa1, 0x01, // COLLECTION (Application)
    0x09, 0x01, //   USAGE (Pointer)
    0xa1, 0x00, //   COLLECTION (Physical)
    0x85, 0x01, //     REPORT_ID (1)
    0x05, 0x09, //     USAGE_PAGE (Button)
    0x19, 0x01, //     USAGE_MINIMUM (Button 1)
    0x29, 0x03, //     USAGE_MAXIMUM (Button 3)
    0x15, 0x00, //     LOGICAL_MINIMUM (0)
    0x25, 0x01, //     LOGICAL_MAXIMUM (1)
    0x95, 0x03, //     REPORT_COUNT (3) 3 bits for buttons
    0x75, 0x01, //     REPORT_SIZE (1)
    0x81, 0x02, //     INPUT (Data,Var,Abs)
    0x95, 0x01, //     REPORT_COUNT (1)
    0x75, 0x05, //     REPORT_SIZE (5) 5 bits for padding
    0x81, 0x03, //     INPUT (Cnst,Var,Abs)
    0x05, 0x01, //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30, //     USAGE (X)
    0x09, 0x31, //     USAGE (Y)
    0x09, 0x38, //     USAGE (Wheel)
    0x15, 0x81, //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f, //     LOGICAL_MAXIMUM (127)
    0x75, 0x08, //     REPORT_SIZE (8)
    0x95, 0x03, //     REPORT_COUNT (3) 24 bits for analog
    0x81, 0x06, //     INPUT (Data,Var,Rel)
    0xc0,       //   END_COLLECTION
    0xc0,       // END_COLLECTION
};

static uint8_t usb_state;
#define USB_DETACHED_STATE 0
#define USB_ATTACHED_STATE 1
#define USB_POWERED_STATE 2
#define USB_DEFAULT_STATE 3
#define USB_ADR_PENDING_STATE 4
#define USB_ADDRESS_STATE 5
#define USB_CONFIGURED_STATE 6

static uint8_t usb_config;

static uint8_t ctrl_trf_state;
// ctrl_trf_state
#define USB_WAIT_SETUP 0
// CTRL_TRF_TX from dev to host
#define USB_WAIT_IN 1
// CTRL_TRF_RX from host to dev
#define USB_WAIT_OUT 2

static uint8_t ctrl_trf_session_owner;
// MUID = Microchip USB Class ID
// Used to identify which of the USB classes owns the current
// session of control transfer over EP0
#define MUID_NULL  0
#define MUID_USB9  1

uint16_t data_count;
const uint8_t *data_ptr;

// Buffer Descriptor Status Register Initialization Parameters
#define _BSTALL     0x04  // Buffer Stall enable
#define _DTSEN      0x08  // Data Toggle Synch enable
#define _INCDIS     0x10  // Address increment disable
#define _KEN        0x20  // SIE keeps buff descriptors enable
#define _DAT0       0x00  // DATA0 packet expected next
#define _DAT1       0x40  // DATA1 packet expected next
#define _DTSMASK    0x40  // DTS Mask
#define _USIE       0x80  // SIE owns buffer
#define _UCPU       0x00  // CPU owns buffer

#define EP0_SIZE 8

// must use dual port RAM for endpoint memory
// 18F2450
#define USB_BANK 0x400
// 18F14K50
//#define USB_BANK 0x200

volatile uint8_t *EP0_OUT = (uint8_t *)(USB_BANK);
volatile uint8_t *EP0_IN = (uint8_t *)(USB_BANK + 4);
// user endpoints
volatile uint8_t *EP1_OUT = (uint8_t *)(USB_BANK + 8);
volatile uint8_t *EP1_IN = (uint8_t *)(USB_BANK + 12);
volatile uint8_t *setup_out_packet = (uint8_t *)(USB_BANK + 16);
volatile uint8_t *setup_in_packet = (uint8_t *)(USB_BANK + 24);
// the report packet
volatile uint8_t *hid_in_packet = (uint8_t *)(USB_BANK + 32);

#define USTAT_EP00_OUT    (0x00 << 3) | (0 << 2)
#define USTAT_EP00_IN     (0x00 << 3) | (1 << 2)

// CTRL_TRF_SETUP.RequestType
#define REQUEST_STANDARD 0x00
#define REQUEST_SET_ADR  0x05
#define REQUEST_GET_DSC  0x06
#define REQUEST_SET_CFG  0x09
#define REQUEST_GET_CFG  0x08

// buffer descriptor utilities

#define SET_EP(PTR, STAT, CNT, ADR) \
	*(PTR + 1) = (CNT); \
	*(PTR + 2) = ((uint16_t)(ADR) & 0xff); \
	*(PTR + 3) = ((uint16_t)(ADR) >> 8); \
/* STAT must be updated last */ \
	*(PTR) = (STAT);


#define SET_EP_STAT(PTR, VALUE) \
	*(PTR) = (VALUE);


#define SET_EP_CNT(PTR, VALUE) \
	*(PTR + 1) = (VALUE);

#define SET_EP_ADR(PTR, VALUE) \
	*(PTR + 2) = ((uint16_t)(VALUE) & 0xff); \
	*(PTR + 3) = ((uint16_t)(VALUE) >> 8);


#define GET_EP_CNT(DST, PTR) \
	DST = *(PTR + 1)

#define GET_EP_STAT(DST, PTR) \
	DST = *(PTR)


// if STAT.DTS is 1
#define IF_EP_DTS(PTR) \
	if(*(PTR) & (1 << 6))

// if endpoint is owned by the CPU
#define IF_EP_READY(PTR) \
	if(!(*(PTR) & (1 << 7)))

// if endpoint is not owned by the CPU
#define IF_EP_NOTREADY(PTR) \
	if((*(PTR) & (1 << 7)))



void init_usb()
{
    uint8_t i;
    for(i = 0; i < 8; i++)
    {
        EP0_OUT[i] = 0;
    }

// full speed requires a 48Mhz oscillator (page 134)
    UCFG = 0b00010100;
// slow speed is possible with a 6Mhz oscillator
    //UCFG = 0b00010000;
    UCON = 0b00001000;
    
// After enabling the USB module, it takes some time for the voltage
// on the D+ or D- line to rise high enough to get out of the SE0 condition.
// The USB Reset interrupt should not be unmasked until the SE0 condition is
// cleared. This helps prevent the firmware from misinterpreting this
// unique event as a USB bus reset from the USB host.
    while(1)
    {
        ClrWdt();
        if(!UCONbits.SE0)
        {
            break;
        }
    }

// Clear all USB interrupts
    UIR = 0;
    UIE = 0;
    PIR2bits.USBIF = 0;
    UIEbits.URSTIE = 1;
    UIEbits.STALLIE = 1;
    UIEbits.TRNIE = 1;
    PIE2bits.USBIE = 1;
    usb_state = USB_POWERED_STATE;
}


// forces EP0 OUT to be ready for a new Setup
// transaction, and forces EP0 IN to be owned by CPU.
// USBPrepareForNextSetupTrf
void usb_prepare_setup()
{
    ctrl_trf_state = USB_WAIT_SETUP;
// this is where data from the host goes
//	SET_EP EP0_OUT, _USIE | _DAT0 | _DTSEN, EP0_SIZE, setup_out_packet
	SET_EP(EP0_OUT, _USIE | _DAT0 | _DTSEN | _BSTALL, EP0_SIZE, setup_out_packet)
// EP0 IN buffer initialization
// this is where data to the host goes
    SET_EP_STAT(EP0_IN, _UCPU)
}


void handle_usb_reset()
{
//print_text("handle_usb_reset\n");
//flush_uart();
// Clears all USB interrupts
    UIR = 0;
// Reset to default address
    UADDR = 0;
// Init EP0 as a Ctrl EP
    UEP0 = 0;
    UEP0bits.EPOUTEN = 1;
    UEP0bits.EPINEN = 1;
    UEP0bits.EPHSHK = 1;
// Flush any pending transactions
    while(UIRbits.TRNIF)
    {
        UIRbits.TRNIF = 0;
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
    }
// Make sure packet processing is enabled
    UCONbits.PKTDIS = 0;
    usb_prepare_setup();
// Clear active configuration
    usb_config = 0;
    usb_state = USB_DEFAULT_STATE;
}

void handle_usb_stall()
{
    if(UEP0bits.EPSTALL)
    {
        usb_prepare_setup();
        UEP0bits.EPSTALL = 0;
    }
    
    UIRbits.STALLIF = 0;
}



void user_endpoint_init()
{
    UEP1 = 0;
    UEP1bits.EPCONDIS = 1;
    UEP1bits.EPOUTEN = 1;
    UEP1bits.EPINEN = 1;
    UEP1bits.EPHSHK = 1;

    SET_EP(EP1_IN, _USIE | _DAT1 | _DTSEN, EP1_SIZE, hid_in_packet)
}

void usb_check_std_request()
{
// handle setup data packet
// USBCheckStdRequest
// CTRL_TRF_SETUP.bRequest
	uint8_t request = setup_out_packet[1];

	if(request == REQUEST_SET_ADR)
    {
		ctrl_trf_session_owner = MUID_USB9;
		usb_state = USB_ADR_PENDING_STATE;
		return;
    }
    else
    if(request == REQUEST_GET_DSC)
	{
		ctrl_trf_session_owner = MUID_USB9;
// CTRL_TRF_SETUP.bDscType
		uint8_t type = setup_out_packet[3];

//print_text("A ");
//print_number(type);
//print_text("\n");
		if(type == DESCRIPTOR_DEV)
		{
			data_ptr = usb_descriptor;
			data_count = sizeof(usb_descriptor);
			return;
        }
        else
	    if(type == DESCRIPTOR_CFG)
        {
			data_ptr = usb_config1;
			data_count = sizeof(usb_config1);
			return;
        }
        else
        if(type == DESCRIPTOR_STR)
		{

// CTRL_TRF_SETUP.bDscIndex
			uint8_t index = setup_out_packet[2];

//print_text("C ");
//print_number(index);
//print_text("\n");
			if(index == 2)
			{
// string 2
				data_ptr = sd002;
				data_count = sizeof(sd002);
				return;
            }
            else
			if(index == 1)
			{
// string 1
				data_ptr = sd001;
				data_count = sizeof(sd001);
				return;
            }
            else
            {
// default string
			    data_ptr = sd000;
			    data_count = sizeof(sd000);
			    return;
            }
        }
        else
        if(type == DSC_RPT)
        {
            data_ptr = _hidReportDescriptor;
            data_count = sizeof(_hidReportDescriptor);
        }
        else
        {
// This is required to stall the DEVICE_QUALIFIER request
		    ctrl_trf_session_owner = MUID_NULL;
		    return;
        }
    }
    else
    if(request == REQUEST_SET_CFG)
    {
// USBStdSetCfgHandler
// This routine first disables all endpoints by clearing
// UEP registers. It then configures (initializes) endpoints
// specified in the modifiable section.
		ctrl_trf_session_owner = MUID_USB9;
// CTRL_TRF_SETUP.bCfgValue
        usb_config = setup_out_packet[2];
		if(usb_config == 0)
        {
			usb_state = USB_ADDRESS_STATE;
			return;
        }
        else
        {
			usb_state = USB_CONFIGURED_STATE;

// user configures endpoints here
			user_endpoint_init();

// done configuring.  Yay!
			return;
        }
    }
    else
    if(request == REQUEST_GET_CFG)
    {
		ctrl_trf_session_owner = MUID_USB9;
		data_ptr = &usb_config;
		data_count = 1;
		return;
    }
    else
    {
        print_text("unknown request\n");
        flush_uart();
    }
}


// USBCtrlTrfTxService
// send data to host
void handle_usb_ctrl_in()
{
// print_text("B ");
// print_number(data_count);
// print_text("\n");
// flush_uart();
// clamp fragment
	uint16_t fragment = data_count;
	if(fragment > EP0_SIZE)
    {
        fragment = EP0_SIZE;
    }

	SET_EP_CNT(EP0_IN, fragment)

// copy fragment to USB RAM
    uint8_t i;
    for(i = 0; i < fragment; i++)
    {
        setup_in_packet[i] = data_ptr[i];
    }
    data_ptr += fragment;
	data_count -= fragment;
}

// USBCtrlTrfSetupHandler
void handle_usb_ctrl_setup()
{
	ctrl_trf_state = USB_WAIT_SETUP;
	ctrl_trf_session_owner = MUID_NULL;
	data_count = 0;

// print_text("handle_usb_ctrl_setup ");
// print_hex2(setup_out_packet[0]);
// print_hex2(setup_out_packet[1]);
// print_hex2(setup_out_packet[2]);
// print_hex2(setup_out_packet[3]);
// print_hex2(setup_out_packet[4]);
// print_hex2(setup_out_packet[5]);
// print_hex2(setup_out_packet[6]);
// print_hex2(setup_out_packet[7]);
// print_text("\n");
// flush_uart();

// scan the data received
// USBCheckStdRequest
// CTRL_TRF_SETUP.RequestType
    uint8_t type = setup_out_packet[0];
	type &= 0b01100000;
	if(type == (REQUEST_STANDARD << 5))
    {
    	usb_check_std_request();
    }

// USBCtrlEPServiceComplete
	UCONbits.PKTDIS = 0;

	if(ctrl_trf_session_owner == MUID_NULL)
    {

// If no one knows how to service this request then stall.
// Must also prepare EP0 to receive the next SETUP transaction.
		SET_EP(EP0_OUT, _USIE | _BSTALL, EP0_SIZE, setup_out_packet);
		SET_EP_STAT(EP0_IN,  _USIE | _BSTALL);
		return;
    }
    else
    {
// A module has claimed ownership of the control transfer session.
// CTRL_TRF_SETUP.DataDir
	    if(setup_out_packet[0] & (1 << 7))
        {
// SetupPkt.DataDir == DEV_TO_HOST
// clamp packet size to size in request descriptor
// CTRL_TRF_SETUP.wLength
            uint16_t length = setup_out_packet[6] |
                (((uint16_t)setup_out_packet[7]) << 8);
		    if(length < data_count)
		    {
			    data_count = length;
            }

		    handle_usb_ctrl_in();
		    ctrl_trf_state = USB_WAIT_IN;
// If something goes wrong, prepare for setup packet
		    SET_EP(EP0_OUT, _USIE, EP0_SIZE, setup_out_packet);
// Prepare for input packet
		    SET_EP_ADR(EP0_IN, setup_in_packet);
		    SET_EP_STAT(EP0_IN, _USIE | _DAT1 | _DTSEN);
		    return;


        }
        else
        {
// SetupPkt.DataDir == HOST_TO_DEV
	        ctrl_trf_state = USB_WAIT_OUT;
	        SET_EP_CNT(EP0_IN, 0);
	        SET_EP_STAT(EP0_IN, _USIE | _DAT1 | _DTSEN);

	        SET_EP(EP0_OUT, _USIE | _DAT1 | _DTSEN, EP0_SIZE, setup_in_packet);
	        return;
        }
    }
}

void handle_usb_ctrl_out()
{
// USBCtrlTrfRxService
// read data from host
    uint8_t temp;
	GET_EP_CNT(temp, EP0_OUT);
	
	data_count += temp;
	
// data is now in SETUP_IN_PACKET but ignored
}

// USTAT == EP0_OUT
void handle_usb_ctrl_output()
{
    uint8_t temp;
	GET_EP_STAT(temp, EP0_OUT);
    temp &= 0b00111100;

// print_text("handle_usb_ctrl_output ");
// print_hex2(EP0_OUT[0]);
// print_hex2(EP0_OUT[1]);
// print_hex2(EP0_OUT[2]);
// print_hex2(EP0_OUT[3]);
// print_text("\n");
	if(temp == (SETUP_TOKEN << 2))
    {
    	handle_usb_ctrl_setup();
    }
    else
    {

// USBCtrlTrfOutHandler
		if(ctrl_trf_state == USB_WAIT_OUT)
		{
			handle_usb_ctrl_out();
			
			IF_EP_DTS(EP0_OUT)
			{
// STAT.DTS == 1
				SET_EP_STAT(EP0_OUT, _USIE | _DAT0 | _DTSEN)
            }
// STAT.DTS == 0
            {
			    SET_EP_STAT(EP0_OUT, _USIE | _DAT1 | _DTSEN)
			}
        }
        else
        {
	        usb_prepare_setup();
        }
    }
}





// USBCtrlTrfInHandler
void handle_usb_ctrl_input()
{
// check if in ADR_PENDING_STATE
// mUSBCheckAdrPendingState
	if(usb_state == USB_ADR_PENDING_STATE)
    {
// SetupPkt.bDevADR
		UADDR = *(setup_out_packet + 2);

		if(!UADDR)
		{
            usb_state = USB_DEFAULT_STATE;
        }
        else
        {
			usb_state = USB_ADDRESS_STATE;
        }
    }


    if(ctrl_trf_state == USB_WAIT_IN)
	{
		handle_usb_ctrl_in();

// endpoint 0 descriptor STAT
		IF_EP_DTS(EP0_IN)
		{
// STAT.DTS == 1
			SET_EP_STAT(EP0_IN, _USIE | _DAT0 | _DTSEN)
        }
        else
        {
// STAT.DTS == 0
    		SET_EP_STAT(EP0_IN, _USIE | _DAT1 | _DTSEN)
		}
    }
    else
    {
    	usb_prepare_setup();
    }
}



void handle_usb_transaction()
{
// USBCtrlEPService
// Must do only 1 per interrupt
	if(USTAT == USTAT_EP00_OUT)
	{
    	handle_usb_ctrl_output();
	}
    else
    if(USTAT == USTAT_EP00_IN)
    {
    	handle_usb_ctrl_input();
    }


// must process TRNIF interrupt before clearing it
	UIRbits.TRNIF = 0;
}

// #define HANDLE_USB \
// { \
//     if(UIRbits.URSTIF) \
//     { \
//         handle_usb_reset(); \
//     } \
//  \
//     if(UIRbits.STALLIF) \
//     { \
//         handle_usb_stall(); \
//     } \
//  \
//     if(UIRbits.TRNIF) \
//     { \
//         handle_usb_transaction(); \
//     } \
// }






// Touchpad I/O ----------------------------------------------------------------
// https://www.hackster.io/frank-adams/laptop-touchpad-conversion-to-usb-d70519

// 48Mhz crystal
void delayMicroseconds(uint16_t x)
{
    uint16_t i;
    uint16_t j;
    for(i = 0; i < x; i++)
    {
        asm("nop");
    }
}

// delay in ms
void delay(uint16_t x)
{
    uint16_t i;
    for(i = 0; i < x; i++)
    {
        delayMicroseconds(1000);
    }
}

void tp_write(uint8_t send_data)  
{
  uint16_t timeout = 200; // breakout of loop if over this value in msec
  tick = 0; // zero the watchdog timer clock
  uint8_t odd_parity = 0; // clear parity bit count
// Enable the bus by floating the clock and data
  CLK_TRIS = 1;
  DAT_TRIS = 1;
  delayMicroseconds(250); // wait before requesting the bus
  CLK_TRIS = 0; //   Send the Clock line low to request to transmit data
  delayMicroseconds(100); // wait for 100 microseconds per bus spec
  DAT_TRIS = 0; //  Send the Data line low (the start bit)
  delayMicroseconds(1); //
  CLK_TRIS = 1; //   Release the Clock line so it is pulled high
  delayMicroseconds(1); // give some time to let the clock line go high

  while (CLK_PORT == 1) 
  { // loop until the clock goes low
    if (tick >= timeout) 
    { //check for infinite loop
      break; // break out of infinite loop
    }
  }

// send the 8 bits of send_data 
  for (uint8_t j=0; j<8; j++) 
  {
    if (send_data & 1) 
    {  //check if lsb is set
      DAT_TRIS = 1; // send a 1 to TP
      odd_parity = odd_parity + 1; // keep running total of 1's sent
    }
    else 
    {
      DAT_TRIS = 0; // send a 0 to TP
    }

    delayMicroseconds(1); // delay to let the clock settle out

    while (CLK_PORT == 0) 
    { // loop until the clock goes high
      if (tick >= timeout) 
      { //check for infinite loop
        break; // break out of infinite loop
      }
    }
    delayMicroseconds(1); // delay to let the clock settle out
    while (CLK_PORT == 1) 
    { // loop until the clock goes low
      if (tick >= timeout) 
      { //check for infinite loop
        break; // break out of infinite loop
      }
    }  
    send_data = send_data >> 1; // shift data right by 1 to prepare for next loop
  }

// send the parity bit
  if (odd_parity & 1) 
  {  //check if lsb of parity is set
    DAT_TRIS = 0; // already odd so send a 0 to TP
  }
  else 
  {
    DAT_TRIS = 1; // send a 1 to TP to make parity odd
  }   

  delayMicroseconds(1); // delay to let the clock settle out
  while (CLK_PORT == 0) 
  { // loop until the clock goes high
    if (tick >= timeout) 
    { //check for infinite loop
      break; // break out of infinite loop
    }
  }
  delayMicroseconds(1); // delay to let the clock settle out
  while (CLK_PORT == 1) 
  { // loop until the clock goes low
    if (tick >= timeout) 
    { //check for infinite loop
      break; // break out of infinite loop
    }
  }

  DAT_TRIS = 1; //  Release the Data line so it goes high as the stop bit
  delayMicroseconds(80); // testing shows delay at least 40us 
  while (CLK_PORT == 1) 
  { // loop until the clock goes low
    if (tick >= timeout) 
    { //check for infinite loop
      break; // break out of infinite loop
    }
  }
  delayMicroseconds(1); // wait to let the data settle
  if (DAT_PORT) 
  { // Ack bit s/b low if good transfer
  }
  while ((CLK_PORT == 0) || (DAT_PORT == 0)) 
  { // loop if clock or data are low
    if (tick >= timeout) 
    { //check for infinite loop
      break; // break out of infinite loop
    }
  }
// Inhibit the bus so the tp only talks when we're listening
  CLK_TRIS = 0;
}

//
// Function to get a byte of data from the touchpad
//
uint8_t tp_read()
{
  uint16_t timeout = 200; // breakout of loop if over this value in msec
  tick = 0; // zero the watchdog timer clock
  uint8_t rcv_data = 0; // initialize to zero
  uint8_t mask = 1; // shift a 1 across the 8 bits to select where to load the data
  uint8_t rcv_parity = 0; // count the ones received

  CLK_TRIS = 1; // release the clock
  DAT_TRIS = 1; // release the data
  delayMicroseconds(5); // delay to let clock go high
  while (CLK_PORT == 1) 
  { // loop until the clock goes low
    if (tick >= timeout) 
    { //check for infinite loop
      break; // break out of infinite loop
    }
  }
  if (DAT_PORT) { // Start bit s/b low from tp
  // start bit not correct - put error handler here if desired
  }  
  delayMicroseconds(1); // delay to let the clock settle out
  while (CLK_PORT == 0) 
  { // loop until the clock goes high
    if (tick >= timeout) 
    { //check for infinite loop
      break; // break out of infinite loop
    }
  }
  for (uint8_t k=0; k<8; k++) 
  {  
    delayMicroseconds(1); // delay to let the clock settle out
    while (CLK_PORT == 1) 
    { // loop until the clock goes low
      if (tick >= timeout) 
      { //check for infinite loop
        break; // break out of infinite loop
      }
    }
    if (DAT_PORT) 
    { // check if data is high
      rcv_data = rcv_data | mask; // set the appropriate bit in the rcv data
      rcv_parity++; // increment the parity bit counter
    }
    mask = mask << 1;
    delayMicroseconds(1); // delay to let the clock settle out
    while (CLK_PORT == 0) 
    { // loop until the clock goes high
      if (tick >= timeout) 
      { //check for infinite loop
        break; // break out of infinite loop
      }
    }
  }

// receive parity
  delayMicroseconds(1); // delay to let the clock settle out
  while (CLK_PORT == 1) 
  { // loop until the clock goes low
    if (tick >= timeout) 
    { //check for infinite loop
      break; // break out of infinite loop
    }
  }
  if (DAT_PORT)  
  { // check if received parity is high
    rcv_parity++; // increment the parity bit counter
  }
  rcv_parity = rcv_parity & 1; // mask off all bits except the lsb
  if (rcv_parity == 0) 
  { // check for bad (even) parity
  // bad parity - pass to future error handler
  } 
  delayMicroseconds(1); // delay to let the clock settle out
  while (CLK_PORT == 0) 
  { // loop until the clock goes high
    if (tick >= timeout) 
    { //check for infinite loop
      break; // break out of infinite loop
    }
  }
// stop bit
  delayMicroseconds(1); // delay to let the clock settle out
  while (CLK_PORT == 1) 
  { // loop until the clock goes low
    if (tick >= timeout) 
    { //check for infinite loop
      break; // break out of infinite loop
    }
  }
  if (DAT_PORT == 0) 
  { // check if stop bit is bad (low)
  // send bad stop bit to future error handler
  }
  delayMicroseconds(1); // delay to let the clock settle out
  while (CLK_PORT == 0) 
  { // loop until the clock goes high
    if (tick >= timeout) 
    { //check for infinite loop
      break; // break out of infinite loop
    }
  }
// Inhibit the bus so the tp only talks when we're listening
  CLK_TRIS = 0;

  return rcv_data; // pass the received data back
}



void touchpad_init()
{
  flags.touchpad_error = 0; // start with no error
  CLK_TRIS = 1; // float the clock and data to touchpad
  DAT_TRIS = 1;
  //  Sending reset command to touchpad
  tp_write(0xff);
  if (tp_read() != 0xfa) 
  { // verify correct ack byte
    flags.touchpad_error = 1;
  }
  delay(1000); // wait 1000ms so tp can run its self diagnostic
  ClrWdt();

  //  verify proper response from tp
  if (tp_read() != 0xaa) 
  { // verify basic assurance test passed
    flags.touchpad_error = 1;
  } 
  if (tp_read() != 0x00) 
  { // verify basic assurance test passed
    flags.touchpad_error = 1;
  }
  // increase resolution from 4 counts/mm to 8 counts/mm
  tp_write(0xe8); //  Sending resolution command
  if (tp_read() != 0xfa) 
  { // verify correct ack byte
    flags.touchpad_error = 1;
  } 
  tp_write(0x03); // value of 0x03 = 8 counts/mm resolution (default is 4 counts/mm)
  if (tp_read() != 0xfa) 
  { // verify correct ack byte
    flags.touchpad_error = 1;
  }
  //  Sending remote mode code so the touchpad will send data only when polled
  tp_write(0xf0);  // remote mode 
  if (tp_read() != 0xfa) 
  { // verify correct ack byte
    flags.touchpad_error = 1;
  } 
  //  Sending touchpad enable code (needed for Elan touchpads)
  tp_write(0xf4);  // tp enable 
  if (tp_read() != 0xfa) 
  { // verify correct ack byte
    flags.touchpad_error = 1;
  }
}


void send_hid_packet(uint8_t size)
{
    while(1)
    {
        IF_EP_READY(EP1_IN)
        {
            break;
        }
    }

    IF_EP_DTS(EP1_IN)
    {
        SET_EP(EP1_IN, _USIE | _DAT0 | _DTSEN, size, hid_in_packet)
    }
    else
    {
        SET_EP(EP1_IN, _USIE | _DAT1 | _DTSEN, size, hid_in_packet)
    }

// send an empty packet to avoid "input irq status -75 received"
    while(1)
    {
        IF_EP_READY(EP1_IN)
        {
            break;
        }
    }

    IF_EP_DTS(EP1_IN)
    {
        SET_EP(EP1_IN, _USIE | _DAT0 | _DTSEN, 0, hid_in_packet)
    }
    else
    {
        SET_EP(EP1_IN, _USIE | _DAT1 | _DTSEN, 0, hid_in_packet)
    }
}

void handle_pad()
{
    static uint8_t prev_mstat = 0;
    uint8_t mstat;
    uint8_t mx;
    uint8_t my;

    flags.overflow = 0;
    tp_write(0xeb);  // request data
    if (tp_read() != 0xfa) 
    { // verify correct ack byte
    // bad ack - pass to future error handler
    }
    mstat = tp_read(); // save into status variable
    mx = tp_read(); // save into x variable
    my = tp_read(); // save into y variable
    if (((0x80 & mstat) == 0x80) || ((0x40 & mstat) == 0x40))  
    {   // x or y overflow bits set?
      flags.overflow = 1; // set the overflow flag
    }   
// change the x data from 9 bit to 8 bit 2's complement
    mx = mx & 0x7f; // mask off 8th bit
    if ((0x10 & mstat) == 0x10) 
    {   // move the sign into 
      mx = 0x80 | mx;              // the 8th bit position
    } 
// change the y data from 9 bit to 8 bit 2's complement and then take the 2's complement 
// because y movement on ps/2 format is opposite of touchpad.move function
    my = my & 0x7f; // mask off 8th bit
    if ((0x20 & mstat) == 0x20) 
    {   // move the sign into 
      my = 0x80 | my;              // the 8th bit position
    } 
    my = (~my + 0x01); // change the sign of y data by taking the 2's complement (invert and add 1)
// zero out mx and my if over_flow or touchpad_error is set
    if ((flags.overflow) || (flags.touchpad_error)) 
    { 
      mx = 0x00;       // data is garbage so zero it out
      my = 0x00;
    }

    if(mstat != prev_mstat)
    {
        print_bin(mstat);
        print_text("\n");
        flush_uart();
        prev_mstat = mstat;
    }

    if(mx != 0 || 
        my != 0 || 
        left_button.changed ||
        right_button.changed)
    {
//         print_text("P ");
//          print_number(mx);
//          print_number(my);
//          print_text("\n");
//          flush_uart();

// ID from Arduino HID_::SendReport
        hid_in_packet[0] = 1;

// report example from
// https://eleccelerator.com/tutorial-about-usb-hid-report-descriptors/
// buttons
        hid_in_packet[1] = left_button.value | (right_button.value << 1);
        left_button.changed = 0;
        right_button.changed = 0;
// movement
        hid_in_packet[2] = my;
        hid_in_packet[3] = -mx;
// wheel
        hid_in_packet[4] = 0;
        send_hid_packet(5);
    }
}


#define UPDATE_BUTTON(state, port) \
{ \
    if(!port) \
    { \
        if(state.accum < DEBOUNCE) \
        { \
            state.accum++; \
        } \
        else \
        if(!state.value) \
        { \
            state.changed = 1; \
            state.value = 1; \
        } \
    } \
    else \
    { \
        if(state.accum > 0) \
        { \
            state.accum--; \
        } \
        else \
        if(state.value) \
        { \
            state.changed = 1; \
            state.value = 0; \
        } \
    } \
}


// MANE ------------------------------------------------------------------------

int main(int argc, char** argv) 
{
// digital mode
    ADCON1 = 0b00001111;
    flags.value = 0;
    init_uart();

    print_text("\n\n\n\nWelcome to trackpad\n");
    flush_uart();


    CLK_LAT = 0;
    DAT_LAT = 0;
    CLK_TRIS = 1;
    DAT_TRIS = 1;
// mane timer
// 1:32 prescaler for 48Mhz clock
    T0CON = 0b10000100;
    TMR0 = -TIMER0_PERIOD;
    INTCONbits.TMR0IF = 0;
    INTCONbits.TMR0IE = 1;

    touchpad_init();

    LBUTTON_TRIS = 1;
    RBUTTON_TRIS = 1;
    INTCON2bits.RBPU = 0;

    init_usb();

    PIR1bits.RCIF = 0;
	PIE1bits.RCIE = 1;
    INTCONbits.PEIE = 1;
	INTCONbits.GIE = 1;


// test delays
//     while(1)
//     {
//         ClrWdt();
//         DAT_TRIS = !DAT_TRIS;
//         delayMicroseconds(1000);
//     }

//     while(usb_state != USB_CONFIGURED_STATE)
//     {
// // must do this outside the mane loop until USB_CONFIGURED_STATE to 
// // get the right timing
//         ClrWdt();
//         handle_uart();
//         HANDLE_USB
//     }


    while(1)
    {
        ClrWdt();
        handle_uart();

        handle_pad();
    }
    
    return (EXIT_SUCCESS);
}

void __interrupt(low_priority) isr1()
{
}

void __interrupt(high_priority) isr()
{
    flags.interrupt_complete = 0;
	while(!flags.interrupt_complete)
	{
		flags.interrupt_complete = 1;

// have to do USB in the interrupt handler
        if(PIR2bits.USBIF)
        {
            flags.interrupt_complete = 0;
            PIR2bits.USBIF = 0;
            if(UIRbits.URSTIF)
            {
                handle_usb_reset();
            }

            if(UIRbits.STALLIF)
            {
                handle_usb_stall(); 
            } 

            if(UIRbits.TRNIF) 
            { 
                handle_usb_transaction(); 
            }
        }


        if(INTCONbits.TMR0IF)
        {
            INTCONbits.TMR0IF = 0;
            TMR0 = -TIMER0_PERIOD;
            tick++;
            UPDATE_BUTTON(left_button, LBUTTON_PORT);
            UPDATE_BUTTON(right_button, RBUTTON_PORT);
// test clock
//            DAT_TRIS = !DAT_TRIS;
        }

        if(PIR1bits.RCIF)
        {
            handle_uart_rx();
        }
    }
}



