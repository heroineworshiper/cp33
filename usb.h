#ifndef USB_H
#define USB_H

#include "usb_core.h"
#include "usbh_core.h"

#define EP1_SIZE 0x40

typedef struct
{
	int usb_initialized;
	unsigned char *in_buf;
	int have_beacon;
	int have_response;
} usb_t;


extern usb_t usb;

extern USB_OTG_CORE_HANDLE  USB_OTG_dev;

// Device is all in the interrupt handler
#define handle_usb() ;

void usb_start_transmit(unsigned char *data, int len);
void usb_start_receive();


void init_usb();



#endif


