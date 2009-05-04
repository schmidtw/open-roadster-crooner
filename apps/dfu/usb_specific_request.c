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


//_____ I N C L U D E S ____________________________________________________

#include "conf_usb.h"


#if USB_DEVICE_FEATURE == ENABLED

#include "usb_drv.h"
#include "usb_descriptors.h"
#include "usb_standard_request.h"
#include "usb_specific_request.h"
#include "usb_dfu.h"


//_____ M A C R O S ________________________________________________________


//_____ D E F I N I T I O N S ______________________________________________


//_____ P R I V A T E   D E C L A R A T I O N S ____________________________

extern const  void *pbuffer;
extern        U8    data_to_transfer;


//_____ D E C L A R A T I O N S ____________________________________________

//! @brief This function configures the endpoints of the device application.
//! This function is called when the set configuration request has been received.
//!
void usb_user_endpoint_init(U8 conf_nb)
{
}


//! This function is called by the standard USB read request function when
//! the USB request is not supported. This function returns TRUE when the
//! request is processed. This function returns FALSE if the request is not
//! supported. In this case, a STALL handshake will be automatically
//! sent by the standard USB read request function.
//!
Bool usb_user_read_request(U8 type, U8 request)
{
  if ((type & 0x7F) == 0x21)  // Type: Class; Recipient: Interface
  {
    switch (request)
    {
    case DFU_DETACH:
      break;

    case DFU_DNLOAD:
      return usb_dfu_dnload();

    case DFU_UPLOAD:
      return usb_dfu_upload();

    case DFU_GETSTATUS:
      Usb_ack_setup_received_free();
      Usb_reset_endpoint_fifo_access(EP_CONTROL);
      Usb_write_endpoint_data(EP_CONTROL, 8, usb_dfu_status); // bStatus
      Usb_write_endpoint_data(EP_CONTROL, 8, 0);  // bwPollTimeout
      Usb_write_endpoint_data(EP_CONTROL, 8, 0);
      Usb_write_endpoint_data(EP_CONTROL, 8, 0);
      Usb_write_endpoint_data(EP_CONTROL, 8, usb_dfu_state);  // bState
      Usb_write_endpoint_data(EP_CONTROL, 8, 0);  // iString
      Usb_ack_control_in_ready_send();
      while (!Is_usb_control_out_received());
      Usb_ack_control_out_received_free();
      return TRUE;

    case DFU_GETSTATE:
      Usb_ack_setup_received_free();
      Usb_reset_endpoint_fifo_access(EP_CONTROL);
      Usb_write_endpoint_data(EP_CONTROL, 8, usb_dfu_state);  // bState
      Usb_ack_control_in_ready_send();
      while (!Is_usb_control_out_received());
      Usb_ack_control_out_received_free();
      return TRUE;

    case DFU_CLRSTATUS:
    case DFU_ABORT:
      Usb_ack_setup_received_free();
      usb_dfu_status = STATUS_OK;
      usb_dfu_state = STATE_dfuIDLE;
      Usb_ack_control_in_ready_send();
      while (!Is_usb_control_in_ready());
      return TRUE;
    }
  }

  usb_dfu_status = STATUS_errSTALLEDPKT;
  usb_dfu_state = STATE_dfuERROR;
  return FALSE;
}


//! This function returns the size and the pointer on a user information
//! structure
//!
Bool usb_user_get_descriptor(U8 type, U8 string)
{
  pbuffer = NULL;

  switch (type)
  {
  case STRING_DESCRIPTOR:
    switch (string)
    {
    case LANG_ID:
      data_to_transfer = sizeof(usb_user_language_id);
      pbuffer = &usb_user_language_id;
      break;

    case MAN_INDEX:
      data_to_transfer = sizeof(usb_user_manufacturer_string_descriptor);
      pbuffer = &usb_user_manufacturer_string_descriptor;
      break;

    case PROD_INDEX:
      data_to_transfer = sizeof(usb_user_product_string_descriptor);
      pbuffer = &usb_user_product_string_descriptor;
      break;

    case SN_INDEX:
      data_to_transfer = sizeof(usb_user_serial_number);
      pbuffer = &usb_user_serial_number;
      break;

    default:
      break;
    }
    break;

  default:
    break;
  }

  return pbuffer != NULL;
}


#endif  // USB_DEVICE_FEATURE == ENABLED
