/* Copyright (C) 2006-2008, Atmel Corporation All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The name of ATMEL may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY AND
 * SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef _USB_DESCRIPTORS_H_
#define _USB_DESCRIPTORS_H_


//_____ I N C L U D E S ____________________________________________________

#include "conf_usb.h"

#if USB_DEVICE_FEATURE == DISABLED
  #error usb_descriptors.h is #included although USB_DEVICE_FEATURE is disabled
#endif


#include "usb_standard_request.h"
#include "usb_task.h"
#include "conf_isp.h"


//_____ M A C R O S ________________________________________________________

#define Usb_unicode(c)                    (Usb_format_mcu_to_usb_data(16, (U16)(c)))
#define Usb_get_dev_desc_pointer()        (&(usb_dev_desc.bLength))
#define Usb_get_dev_desc_length()         (sizeof(usb_dev_desc))
#define Usb_get_conf_desc_pointer()       (&(usb_conf_desc.cfg.bLength))
#define Usb_get_conf_desc_length()        (sizeof(usb_conf_desc))


//_____ U S B    D E F I N E S _____________________________________________

            // USB Device descriptor
#define USB_SPECIFICATION     0x0200
#define DEVICE_CLASS          0                   //! Each configuration has its own class
#define DEVICE_SUB_CLASS      0                   //! Each configuration has its own subclass
#define DEVICE_PROTOCOL       0                   //! Each configuration has its own protocol
#define EP_CONTROL_LENGTH     64
#define VENDOR_ID             ATMEL_VID           //! Atmel vendor ID
#define PRODUCT_ID            ISP_PID
#define RELEASE_NUMBER        CROONER_VERSION
#define MAN_INDEX             0x01
#define PROD_INDEX            0x02
#define SN_INDEX              0x03
#define NB_CONFIGURATION      1

            // CONFIGURATION
#define NB_INTERFACE       1     //! The number of interfaces for this configuration
#define CONF_NB            1     //! Number of this configuration
#define CONF_INDEX         0
#define CONF_ATTRIBUTES    USB_CONFIG_BUSPOWERED
#define MAX_POWER          50    // 100 mA

            // USB Interface descriptor
#define INTERFACE_NB             0                  //! The number of this interface
#define ALTERNATE                0                  //! The alt setting nb of this interface
#define NB_ENDPOINT              0                  //! The number of endpoints this interface has
#define INTERFACE_CLASS          APPLICATION_CLASS  //! Application-Specific Class
#define INTERFACE_SUB_CLASS      DFU_SUBCLASS       //! Device Firmware Upgrade Subclass
#define INTERFACE_PROTOCOL       DFU_MODE_PROTOCOL  //! DFU Mode Protocol
#define INTERFACE_INDEX          0

            // DFU Functional descriptor
#define DFU_WILL_DETACH             TRUE    //! Device will perform a bus detach-attach sequence when it receives a DFU_DETACH request
#define DFU_MANIFESTATION_TOLERANT  TRUE    //! Device is able to communicate via USB after Manifestation phase
#define DFU_CAN_UPLOAD              TRUE    //! Upload capable
#define DFU_CAN_DNLOAD              TRUE    //! Download capable
#define DFU_DETACH_TIMEOUT          0       //! Time, in milliseconds, that the device will wait after receipt of the DFU_DETACH request
#define DFU_TRANSFER_SIZE           0xFFFF  //! Maximal number of bytes that the device can accept per control-write transaction
#define DFU_VERSION                 0x0101  //! BCD numeric expression identifying the version of the DFU Specification release

#define DEVICE_STATUS         SELF_POWERED
#define INTERFACE_STATUS      0x00 // TBD

#define LANG_ID               0x00

#define USB_MN_LENGTH         13
#define USB_MANUFACTURER_NAME \
{\
  Usb_unicode('O'),\
  Usb_unicode('p'),\
  Usb_unicode('e'),\
  Usb_unicode('n'),\
  Usb_unicode(' '),\
  Usb_unicode('R'),\
  Usb_unicode('o'),\
  Usb_unicode('a'),\
  Usb_unicode('d'),\
  Usb_unicode('s'),\
  Usb_unicode('t'),\
  Usb_unicode('e'),\
  Usb_unicode('r') \
}

#define USB_PN_LENGTH         18
#define USB_PRODUCT_NAME \
{\
  Usb_unicode('C'),\
  Usb_unicode('r'),\
  Usb_unicode('o'),\
  Usb_unicode('o'),\
  Usb_unicode('n'),\
  Usb_unicode('e'),\
  Usb_unicode('r'),\
  Usb_unicode(' '),\
  Usb_unicode('D'),\
  Usb_unicode('F'),\
  Usb_unicode('U'),\
  Usb_unicode('-'),\
  Usb_unicode('1'),\
  Usb_unicode('.'),\
  Usb_unicode(((((ISP_VERSION >> 4) & 0x0F) < 0xA) ? '0' : 'A' - 0xA) + ((ISP_VERSION >> 4) & 0x0F)),\
  Usb_unicode('.'),\
  Usb_unicode((((ISP_VERSION & 0x0F) < 0xA) ? '0' : 'A' - 0xA) + (ISP_VERSION & 0x0F)) \
}

#define USB_SN_LENGTH         28
#define USB_SERIAL_NUMBER \
{\
  Usb_unicode('C'),\
  Usb_unicode('R'),\
  Usb_unicode('O'),\
  Usb_unicode('O'),\
  Usb_unicode('N'),\
  Usb_unicode('E'),\
  Usb_unicode('R'),\
  Usb_unicode(':'),\
  Usb_unicode(((((RELEASE_NUMBER >> 12) & 0x0F) < 0xA) ? '0' : 'A' - 0xA) + ((RELEASE_NUMBER >> 12) & 0x0F)),\
  Usb_unicode(((((RELEASE_NUMBER >> 8) & 0x0F) < 0xA) ? '0' : 'A' - 0xA) + ((RELEASE_NUMBER >> 8) & 0x0F)),\
  Usb_unicode(((((RELEASE_NUMBER >> 4) & 0x0F) < 0xA) ? '0' : 'A' - 0xA) + ((RELEASE_NUMBER >> 4) & 0x0F)),\
  Usb_unicode((((RELEASE_NUMBER & 0x0F) < 0xA) ? '0' : 'A' - 0xA) + (RELEASE_NUMBER & 0x0F)),\
  Usb_unicode(':'),\
  Usb_unicode(((((CROONER_SN >> 28) & 0x0F) < 0xA) ? '0' : 'A' - 0xA) + ((CROONER_SN >> 28) & 0x0F)),\
  Usb_unicode(((((CROONER_SN >> 24) & 0x0F) < 0xA) ? '0' : 'A' - 0xA) + ((CROONER_SN >> 24) & 0x0F)),\
  Usb_unicode(((((CROONER_SN >> 20) & 0x0F) < 0xA) ? '0' : 'A' - 0xA) + ((CROONER_SN >> 20) & 0x0F)),\
  Usb_unicode(((((CROONER_SN >> 16) & 0x0F) < 0xA) ? '0' : 'A' - 0xA) + ((CROONER_SN >> 16) & 0x0F)),\
  Usb_unicode(((((CROONER_SN >> 12) & 0x0F) < 0xA) ? '0' : 'A' - 0xA) + ((CROONER_SN >> 12) & 0x0F)),\
  Usb_unicode(((((CROONER_SN >> 8) & 0x0F) < 0xA) ? '0' : 'A' - 0xA) + ((CROONER_SN >> 8) & 0x0F)),\
  Usb_unicode(((((CROONER_SN >> 4) & 0x0F) < 0xA) ? '0' : 'A' - 0xA) + ((CROONER_SN >> 4) & 0x0F)),\
  Usb_unicode((((CROONER_SN & 0x0F) < 0xA) ? '0' : 'A' - 0xA) + (CROONER_SN & 0x0F))\
}

#define LANGUAGE_ID           0x0409


//! USB Request
typedef
#if __ICCAVR32__
#pragma pack(1)
#endif
struct
#if __GNUC__
__attribute__((__packed__))
#endif
{
  U8      bmRequestType;        //!< Characteristics of the request
  U8      bRequest;             //!< Specific request
  U16     wValue;               //!< Field that varies according to request
  U16     wIndex;               //!< Field that varies according to request
  U16     wLength;              //!< Number of bytes to transfer if Data
}
#if __ICCAVR32__
#pragma pack()
#endif
S_UsbRequest;


//! USB Device Descriptor
typedef
#if __ICCAVR32__
#pragma pack(1)
#endif
struct
#if __GNUC__
__attribute__((__packed__))
#endif
{
  U8      bLength;              //!< Size of this descriptor in bytes
  U8      bDescriptorType;      //!< DEVICE descriptor type
  U16     bscUSB;               //!< Binay Coded Decimal Spec. release
  U8      bDeviceClass;         //!< Class code assigned by the USB
  U8      bDeviceSubClass;      //!< Subclass code assigned by the USB
  U8      bDeviceProtocol;      //!< Protocol code assigned by the USB
  U8      bMaxPacketSize0;      //!< Max packet size for EP0
  U16     idVendor;             //!< Vendor ID. ATMEL = 0x03EB
  U16     idProduct;            //!< Product ID assigned by the manufacturer
  U16     bcdDevice;            //!< Device release number
  U8      iManufacturer;        //!< Index of manu. string descriptor
  U8      iProduct;             //!< Index of prod. string descriptor
  U8      iSerialNumber;        //!< Index of S.N.  string descriptor
  U8      bNumConfigurations;   //!< Number of possible configurations
}
#if __ICCAVR32__
#pragma pack()
#endif
S_usb_device_descriptor;


//! USB Configuration Descriptor
typedef
#if __ICCAVR32__
#pragma pack(1)
#endif
struct
#if __GNUC__
__attribute__((__packed__))
#endif
{
  U8      bLength;              //!< Size of this descriptor in bytes
  U8      bDescriptorType;      //!< CONFIGURATION descriptor type
  U16     wTotalLength;         //!< Total length of data returned
  U8      bNumInterfaces;       //!< Number of interfaces for this conf.
  U8      bConfigurationValue;  //!< Value for SetConfiguration resquest
  U8      iConfiguration;       //!< Index of string descriptor
  U8      bmAttributes;         //!< Configuration characteristics
  U8      MaxPower;             //!< Maximum power consumption
}
#if __ICCAVR32__
#pragma pack()
#endif
S_usb_configuration_descriptor;


//! USB Interface Descriptor
typedef
#if __ICCAVR32__
#pragma pack(1)
#endif
struct
#if __GNUC__
__attribute__((__packed__))
#endif
{
  U8      bLength;              //!< Size of this descriptor in bytes
  U8      bDescriptorType;      //!< INTERFACE descriptor type
  U8      bInterfaceNumber;     //!< Number of interface
  U8      bAlternateSetting;    //!< Value to select alternate setting
  U8      bNumEndpoints;        //!< Number of EP except EP 0
  U8      bInterfaceClass;      //!< Class code assigned by the USB
  U8      bInterfaceSubClass;   //!< Subclass code assigned by the USB
  U8      bInterfaceProtocol;   //!< Protocol code assigned by the USB
  U8      iInterface;           //!< Index of string descriptor
}
#if __ICCAVR32__
#pragma pack()
#endif
S_usb_interface_descriptor;


//! USB Endpoint Descriptor
typedef
#if __ICCAVR32__
#pragma pack(1)
#endif
struct
#if __GNUC__
__attribute__((__packed__))
#endif
{
  U8      bLength;              //!< Size of this descriptor in bytes
  U8      bDescriptorType;      //!< ENDPOINT descriptor type
  U8      bEndpointAddress;     //!< Address of the endpoint
  U8      bmAttributes;         //!< Endpoint's attributes
  U16     wMaxPacketSize;       //!< Maximum packet size for this EP
  U8      bInterval;            //!< Interval for polling EP in ms
}
#if __ICCAVR32__
#pragma pack()
#endif
S_usb_endpoint_descriptor;


//! USB Device Qualifier Descriptor
typedef
#if __ICCAVR32__
#pragma pack(1)
#endif
struct
#if __GNUC__
__attribute__((__packed__))
#endif
{
  U8      bLength;              //!< Size of this descriptor in bytes
  U8      bDescriptorType;      //!< Device Qualifier descriptor type
  U16     bscUSB;               //!< Binay Coded Decimal Spec. release
  U8      bDeviceClass;         //!< Class code assigned by the USB
  U8      bDeviceSubClass;      //!< Subclass code assigned by the USB
  U8      bDeviceProtocol;      //!< Protocol code assigned by the USB
  U8      bMaxPacketSize0;      //!< Max packet size for EP0
  U8      bNumConfigurations;   //!< Number of possible configurations
  U8      bReserved;            //!< Reserved for future use, must be zero
}
#if __ICCAVR32__
#pragma pack()
#endif
S_usb_device_qualifier_descriptor;


//! USB Language Descriptor
typedef
#if __ICCAVR32__
#pragma pack(1)
#endif
struct
#if __GNUC__
__attribute__((__packed__))
#endif
{
  U8      bLength;              //!< Size of this descriptor in bytes
  U8      bDescriptorType;      //!< STRING descriptor type
  U16     wlangid;              //!< Language id
}
#if __ICCAVR32__
#pragma pack()
#endif
S_usb_language_id;


//_____ U S B   M A N U F A C T U R E R   D E S C R I P T O R _______________

//! struct usb_st_manufacturer
typedef
#if __ICCAVR32__
#pragma pack(1)
#endif
struct
#if __GNUC__
__attribute__((__packed__))
#endif
{
  U8  bLength;                  //!< Size of this descriptor in bytes
  U8  bDescriptorType;          //!< STRING descriptor type
  U16 wstring[USB_MN_LENGTH];   //!< Unicode characters
}
#if __ICCAVR32__
#pragma pack()
#endif
S_usb_manufacturer_string_descriptor;


//_____ U S B   P R O D U C T   D E S C R I P T O R _________________________

//! struct usb_st_product
typedef
#if __ICCAVR32__
#pragma pack(1)
#endif
struct
#if __GNUC__
__attribute__((__packed__))
#endif
{
  U8  bLength;                  //!< Size of this descriptor in bytes
  U8  bDescriptorType;          //!< STRING descriptor type
  U16 wstring[USB_PN_LENGTH];   //!< Unicode characters
}
#if __ICCAVR32__
#pragma pack()
#endif
S_usb_product_string_descriptor;


//_____ U S B   S E R I A L   N U M B E R   D E S C R I P T O R _____________

//! struct usb_st_serial_number
typedef
#if __ICCAVR32__
#pragma pack(1)
#endif
struct
#if __GNUC__
__attribute__((__packed__))
#endif
{
  U8  bLength;                  //!< Size of this descriptor in bytes
  U8  bDescriptorType;          //!< STRING descriptor type
  U16 wstring[USB_SN_LENGTH];   //!< Unicode characters
}
#if __ICCAVR32__
#pragma pack()
#endif
S_usb_serial_number;


//_____ U S B   D E V I C E   D F U   D E S C R I P T O R ___________________

//! USB DFU Functional Descriptor
typedef
#if __ICCAVR32__
#pragma pack(1)
#endif
struct
#if __GNUC__
__attribute__((__packed__))
#endif
{
  U8      bLength;              //!< Size of this descriptor in bytes
  U8      bDescriptorType;      //!< DFU FUNCTIONAL descriptor type
  U8      bmAttributes;         //!< DFU attributes
  U16     wDetachTimeOut;       //!< Time, in milliseconds, that the device will wait after receipt of the DFU_DETACH request
  U16     wTransferSize;        //!< Maximal number of bytes that the device can accept per control-write transaction
  U16     bcdDFUVersion;        //!< BCD numeric expression identifying the version of the DFU Specification release
}
#if __ICCAVR32__
#pragma pack()
#endif
S_usb_dfu_functional_descriptor;


typedef
#if __ICCAVR32__
#pragma pack(1)
#endif
struct
#if __GNUC__
__attribute__((__packed__))
#endif
{
  S_usb_configuration_descriptor  cfg;
  S_usb_interface_descriptor      ifc;
  S_usb_dfu_functional_descriptor dfu;
}
#if __ICCAVR32__
#pragma pack()
#endif
S_usb_user_configuration_descriptor;


#endif  // _USB_DESCRIPTORS_H_
