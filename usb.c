#include "settings.h"
#include "linux.h"
#include "uart.h"
#include "usb_core.h"
#include "usb_dcd.h"
#include "usbh_core.h"
#include "usb.h"
#include "usb_hcd_int.h"
#include "stm32f4xx_gpio.h"
#include "cp33.h"




usb_t usb;

// ground camera using USB
USB_OTG_CORE_HANDLE  USB_OTG_dev;

#define USB_MAX_STR_DESC_SIZ       64 
uint8_t USBD_StrDesc[USB_MAX_STR_DESC_SIZ];

#define USB_SIZ_DEVICE_DESC                     18
#define USB_DEVICE_DESCRIPTOR_TYPE              0x01
// vendor ID
#define USBD_VID                     0x04d8
// product ID
#ifdef LEFT_MODE
#define USBD_PID                     0x000e
#else
#define USBD_PID                     0x000b
#endif

#define LOBYTE(x)  ((uint8_t)(x & 0x00FF))
#define HIBYTE(x)  ((uint8_t)((x & 0xFF00) >>8))
#define  USBD_IDX_MFC_STR                               0x01 
#define  USBD_IDX_PRODUCT_STR                           0x02
#define  USBD_IDX_SERIAL_STR                            0x03 
#define USBD_CFG_MAX_NUM           1
#define EP1_IN_ID      0x81
#define EP1_OUT_ID      0x01



/* USB Standard Device Descriptor */
const uint8_t USBD_DeviceDesc_const[USB_SIZ_DEVICE_DESC] __attribute__ ((aligned (4))) =
{
    0x12,                       /*bLength */
    USB_DEVICE_DESCRIPTOR_TYPE, /*bDescriptorType*/
    0x00,                       /*bcdUSB */
    0x02,
    0x00,                       /*bDeviceClass*/
    0x00,                       /*bDeviceSubClass*/
    0x00,                       /*bDeviceProtocol*/
    USB_OTG_MAX_EP0_SIZE,      /*bMaxPacketSize*/
    LOBYTE(USBD_VID),           /*idVendor*/
    HIBYTE(USBD_VID),           /*idVendor*/
    LOBYTE(USBD_PID),           /*idVendor*/
    HIBYTE(USBD_PID),           /*idVendor*/
    0x00,                       /*bcdDevice rel. 2.00*/
    0x02,
    USBD_IDX_MFC_STR,           /*Index of manufacturer  string*/
    USBD_IDX_PRODUCT_STR,       /*Index of product string*/
    USBD_IDX_SERIAL_STR,        /*Index of serial number string*/
    USBD_CFG_MAX_NUM            /*bNumConfigurations*/
}; /* USB_DeviceDescriptor */

static uint8_t USBD_DeviceDesc[USB_SIZ_DEVICE_DESC];

#define USB_SIZ_STRING_LANGID                   4
#define  USB_DESC_TYPE_STRING                              3
#define USBD_LANGID_STRING            0x0409

const uint8_t USBD_LangIDDesc_const[USB_SIZ_STRING_LANGID] __attribute__ ((aligned (4))) =
{
     USB_SIZ_STRING_LANGID,         
     USB_DESC_TYPE_STRING,       
     LOBYTE(USBD_LANGID_STRING),
     HIBYTE(USBD_LANGID_STRING), 
};

uint8_t USBD_LangIDDesc[USB_SIZ_STRING_LANGID];

/**
* @brief  USBD_USR_ManufacturerStrDescriptor 
*         return the manufacturer string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_ManufacturerStrDescriptor( uint8_t speed , uint16_t *length)
{
  USBD_GetString ("Heroine Virtual Ltd.", USBD_StrDesc, length);
  return USBD_StrDesc;
}

#define USBD_PRODUCT_HS_STRING        "HS mode"
#define USBD_SERIALNUMBER_HS_STRING   "1"

#define USBD_PRODUCT_FS_STRING        "FS Mode"
#define USBD_SERIALNUMBER_FS_STRING   "1"

/**
* @brief  USBD_USR_ProductStrDescriptor 
*         return the product string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_ProductStrDescriptor( uint8_t speed , uint16_t *length)
{
    USBD_GetString ("CP33 raw audio", USBD_StrDesc, length);
  return USBD_StrDesc;
}

/**
* @brief  USBD_USR_SerialStrDescriptor 
*         return the serial number string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_SerialStrDescriptor( uint8_t speed , uint16_t *length)
{
    USBD_GetString (USBD_SERIALNUMBER_HS_STRING, USBD_StrDesc, length);
  return USBD_StrDesc;
}


/**
* @brief  USBD_USR_ConfigStrDescriptor 
*         return the configuration string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_ConfigStrDescriptor( uint8_t speed , uint16_t *length)
{
  USBD_GetString ("Config Desc", USBD_StrDesc, length);
  return USBD_StrDesc;  
}

/**
* @brief  USBD_USR_ConfigStrDescriptor 
*         return the configuration string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_InterfaceStrDescriptor( uint8_t speed , uint16_t *length)
{
  USBD_GetString ("Interface Desc", USBD_StrDesc, length);
  return USBD_StrDesc;  
}

/**
* @brief  USBD_USR_DeviceDescriptor 
*         return the device descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_DeviceDescriptor( uint8_t speed , uint16_t *length)
{
  *length = sizeof(USBD_DeviceDesc_const);
  return (uint8_t*)USBD_DeviceDesc;
}


/**
* @brief  USBD_USR_LangIDStrDescriptor 
*         return the LangID string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_LangIDStrDescriptor( uint8_t speed , uint16_t *length)
{
  *length =  sizeof(USBD_LangIDDesc);  
  return (uint8_t*)USBD_LangIDDesc;
}

const USBD_DEVICE USR_desc =
{
  USBD_USR_DeviceDescriptor,
  USBD_USR_LangIDStrDescriptor, 
  USBD_USR_ManufacturerStrDescriptor,
  USBD_USR_ProductStrDescriptor,
  USBD_USR_SerialStrDescriptor,
  USBD_USR_ConfigStrDescriptor,
  USBD_USR_InterfaceStrDescriptor,
  
};


/**
* @brief  USBD_USR_Init 
*         Displays the message on LCD for host lib initialization
* @param  None
* @retval None
*/
void USBD_USR_Init(void)
{   
	TRACE2 
}

/**
* @brief  USBD_USR_DeviceReset 
*         Displays the message on LCD on device Reset Event
* @param  speed : device speed
* @retval None
*/
void USBD_USR_DeviceReset(uint8_t speed )
{
	TRACE2 
}


/**
* @brief  USBD_USR_DeviceConfigured
*         Displays the message on LCD on device configuration Event
* @param  None
* @retval Staus
*/
void USBD_USR_DeviceConfigured (void)
{
	TRACE2 
}

/**
* @brief  USBD_USR_DeviceSuspended 
*         Displays the message on LCD on device suspend Event
* @param  None
* @retval None
*/
void USBD_USR_DeviceSuspended(void)
{
//	TRACE2 
}

/**
* @brief  USBD_USR_DeviceResumed 
*         Displays the message on LCD on device resume Event
* @param  None
* @retval None
*/
void USBD_USR_DeviceResumed(void)
{
	TRACE2 
}

/**
* @brief  USBD_USR_DeviceConnected
*         Displays the message on LCD on device connection Event
* @param  None
* @retval Staus
*/
void USBD_USR_DeviceConnected (void)
{
	TRACE2 
}

/**
* @brief  USBD_USR_DeviceDisonnected
*         Displays the message on LCD on device disconnection Event
* @param  None
* @retval Staus
*/
void USBD_USR_DeviceDisconnected (void)
{
	TRACE2 
}

const USBD_Usr_cb_TypeDef USR_cb =
{
  USBD_USR_Init,
  USBD_USR_DeviceReset,
  USBD_USR_DeviceConfigured,
  USBD_USR_DeviceSuspended,
  USBD_USR_DeviceResumed,
  
  USBD_USR_DeviceConnected,
  USBD_USR_DeviceDisconnected,  
  
  
};

#define USBD_OK 0

/**
  * @brief  class_cb_Init
  *         Initialize the HID interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  class_cb_Init (void  *pdev, 
                               uint8_t cfgidx)
{
  /* Open EP IN */
  DCD_EP_Open(pdev,
              EP1_IN_ID,
              EP1_SIZE,
              USB_OTG_EP_BULK);

  /* Open EP OUT */
  DCD_EP_Open(pdev,
              EP1_OUT_ID,
              EP1_SIZE,
              USB_OTG_EP_BULK);

  DCD_EP_PrepareRx(pdev,
               EP1_OUT_ID,
               (uint8_t*)usb.in_buf,                        
               EP1_SIZE);  

  return USBD_OK;
}

/**
  * @brief  class_cb_Init
  *         DeInitialize the HID layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  class_cb_DeInit (void  *pdev, 
                                 uint8_t cfgidx)
{
  /* Close HID EPs */
  DCD_EP_Close (pdev , EP1_IN_ID);
  DCD_EP_Close (pdev , EP1_OUT_ID);
  
  
  return USBD_OK;
}

/**
  * @brief  class_cb_Setup
  *         Handle the HID specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t  class_cb_Setup (void  *pdev, 
                                USB_SETUP_REQ *req)
{

  return USBD_OK;
}

/**
  * @brief  class_cb_DataIn
  *         handle data IN Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t  class_cb_DataIn (void  *pdev, 
                              uint8_t epnum)
{

  /* Ensure that the FIFO is empty before a new transfer, this condition could 
  be caused by  a new transfer before the end of the previous transfer */

#ifndef USE_CP33
  DCD_EP_Flush(pdev, EP1_IN_ID);
#endif

/*
 * int pin_state = (GPIOA->IDR & GPIO_Pin_15);
 * if(pin_state != (GPIOA->IDR & GPIO_Pin_15))
 * TRACE2
 */


  usb.have_response = 1;
  return USBD_OK;
}

static uint8_t  class_cb_DataOut (void  *pdev, 
                              uint8_t epnum)
{
//print_buffer(usb.in_buf, 16);
	usb.have_beacon = 1;
	cp33.fragment_size = usb.in_buf[0] |
		(usb.in_buf[1] << 8) |
		(usb.in_buf[2] << 16) |
		(usb.in_buf[3] << 24);
	TRACE2
	print_text("fragment_size=");
	print_number(cp33.fragment_size);



  return USBD_OK;
}

void usb_start_receive()
{
//TRACE
	usb.have_beacon = 0;
	DCD_EP_PrepareRx(&USB_OTG_dev,
               EP1_OUT_ID,
               (uint8_t*)usb.in_buf,                        
               EP1_SIZE);  
}

void usb_start_transmit(unsigned char *data, int len)
{
//TRACE2
//print_number(len);
	usb.have_response = 0;
	DCD_EP_Tx ( &USB_OTG_dev,
                     EP1_IN_ID,
                     data,
                     len);
}


#define USB_HID_CONFIG_DESC_SIZ       34
#define USB_CONFIGURATION_DESCRIPTOR_TYPE       0x02
#define USB_INTERFACE_DESCRIPTOR_TYPE           0x04
#define HID_DESCRIPTOR_TYPE           0x21
#define HID_MOUSE_REPORT_DESC_SIZE    74
#define USB_ENDPOINT_DESCRIPTOR_TYPE            0x05

/* USB HID device Configuration Descriptor */
const uint8_t USBD_HID_CfgDesc_const[USB_HID_CONFIG_DESC_SIZ] __attribute__ ((aligned (4))) =
{
  0x09, /* bLength: Configuration Descriptor size */
  USB_CONFIGURATION_DESCRIPTOR_TYPE, /* bDescriptorType: Configuration */
  USB_HID_CONFIG_DESC_SIZ,
  /* wTotalLength: Bytes returned */
  0x00,
  0x01,         /*bNumInterfaces: 1 interface*/
  0x01,         /*bConfigurationValue: Configuration value*/
  0x00,         /*iConfiguration: Index of string descriptor describing
  the configuration*/
  0xE0,         /*bmAttributes: bus powered and Support Remote Wake-up */
  0x32,         /*MaxPower 100 mA: this current is used for detecting Vbus*/
  
  /************** Descriptor of Joystick Mouse interface ****************/
  /* 09 */
  0x09,         /*bLength: Interface Descriptor size*/
  USB_INTERFACE_DESCRIPTOR_TYPE,/*bDescriptorType: Interface descriptor type*/
  0x00,         /*bInterfaceNumber: Number of Interface*/
  0x00,         /*bAlternateSetting: Alternate setting*/
  0x02,         /*bNumEndpoints*/
  0x00,         /*bInterfaceClass: */
  0x00,         /*bInterfaceSubClass : 1=BOOT, 0=no boot*/
  0x00,         /*nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse*/
  0,            /*iInterface: Index of string descriptor*/

  /******************** Endpoint ********************/
  0x07,          /*bLength: Endpoint Descriptor size*/
  USB_ENDPOINT_DESCRIPTOR_TYPE, /*bDescriptorType:*/
  
  EP1_OUT_ID,     /*bEndpointAddress: Endpoint Address (IN)*/
  USB_OTG_EP_BULK,          /*bmAttributes: */
  EP1_SIZE, /*wMaxPacketSize: 4 Byte max */
  0x00,
  0x00,          /*bInterval: Polling Interval (10 ms)*/

  /******************** Endpoint ********************/
  0x07,          /*bLength: Endpoint Descriptor size*/
  USB_ENDPOINT_DESCRIPTOR_TYPE, /*bDescriptorType:*/
  
  EP1_IN_ID,     /*bEndpointAddress: Endpoint Address (IN)*/
  USB_OTG_EP_BULK,          /*bmAttributes: */
  EP1_SIZE, /*wMaxPacketSize: 4 Byte max */
  0x00,
  0x00,          /*bInterval: Polling Interval (10 ms)*/

} ;


static uint8_t USBD_HID_CfgDesc[USB_HID_CONFIG_DESC_SIZ];


/**
  * @brief  class_cb_GetCfgDesc 
  *         return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *class_cb_GetCfgDesc (uint8_t speed, uint16_t *length)
{
  *length = sizeof (USBD_HID_CfgDesc_const);
  return (uint8_t*)USBD_HID_CfgDesc;
}


uint8_t class_cb_EP0_TxSent(void  *pdev)
{
	return 0;
}

uint8_t class_cb_EP0_RxReady(void  *pdev)
{
	return 0;
}

uint8_t  class_cb_sof (void *pdev)
{
}


static uint8_t  class_cb_OUT_Incplt (void  *pdev)
{
  return USBD_OK;
}


static uint8_t  class_cb_IN_Incplt (void  *pdev)
{
  return USBD_OK;
}




const USBD_Class_cb_TypeDef  USBD_class_cb = 
{
  class_cb_Init,   // Init
  class_cb_DeInit,  // DeInit
// control endpoint
  class_cb_Setup, // Setup
  class_cb_EP0_TxSent, /*EP0_TxSent*/  
  class_cb_EP0_RxReady, /*EP0_RxReady*/
// my endpoints
  class_cb_DataIn, /*DataIn*/
  class_cb_DataOut, /*DataOut*/
  class_cb_sof, /*SOF */
  class_cb_IN_Incplt,   // IsoINIncomplete
  class_cb_OUT_Incplt,    // IsoOUTIncomplete
  class_cb_GetCfgDesc,  // GetConfigDescriptor
#ifdef USB_OTG_HS_CORE  
  class_cb_GetCfgDesc, /* GetOtherConfigDescriptor */
#endif  
};

/* USB Standard Device Descriptor */
#define  USB_LEN_DEV_QUALIFIER_DESC                     0x0A
const uint8_t USBD_DeviceQualifierDesc_const[USB_LEN_DEV_QUALIFIER_DESC] __attribute__ ((aligned (4))) =
{
  USB_LEN_DEV_QUALIFIER_DESC,
  USB_DESC_TYPE_DEVICE_QUALIFIER,
  0x00,
  0x02,
  0x00,
  0x00,
  0x00,
  0x40,
  0x01,
  0x00,
};

uint8_t USBD_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC];


void init_usb()
{


	bzero((unsigned char*)&usb, sizeof(usb));
  	bzero((unsigned char*)&USB_OTG_dev, sizeof(USB_OTG_dev));

	memcpy(USBD_DeviceQualifierDesc, USBD_DeviceQualifierDesc_const, USB_LEN_DEV_QUALIFIER_DESC);
	memcpy(USBD_LangIDDesc, USBD_LangIDDesc_const, sizeof(USBD_LangIDDesc_const));
	memcpy(USBD_DeviceDesc, USBD_DeviceDesc_const, sizeof(USBD_DeviceDesc_const));
	memcpy(USBD_HID_CfgDesc, USBD_HID_CfgDesc_const, sizeof(USBD_HID_CfgDesc_const));
	
	usb.in_buf = kmalloc(EP1_SIZE, 1);
	usb.have_response = 1;

 	USBD_Init(&USB_OTG_dev,
            USB_OTG_HS_CORE_ID,
            &USR_desc, 
            &USBD_class_cb, 
            &USR_cb);




}




