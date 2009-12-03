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


//_____  I N C L U D E S ___________________________________________________

#include "conf_usb.h"


#if USB_DEVICE_FEATURE == ENABLED

#include <avr32/io.h>
#include <stddef.h>
#include <string.h>
#include "compiler.h"
#include "pm.h"
// #include "wdt.h"
#include "flashc.h"
#include "usb_drv.h"
#include "usb_task.h"
#include "usb_descriptors.h"
#include "usb_dfu.h"
#include "boot.h"
#include "conf_isp.h"
#include "isp.h"
#include "led.h"


//_____ M A C R O S ________________________________________________________

//! Convert 16-, 32- or 64-bit data between MCU and Atmel-DFU endianisms.
//! Depending on MCU endianism, swap or not data bytes.
//! @param width    Data width in bits: 16, 32 or 64
//! @param data     16-, 32- or 64-bit data to format
//! @return         Formatted 16-, 32- or 64-bit data
//! @{
#if LITTLE_ENDIAN_MCU
  #define Dfu_format_mcu_to_dfu_data(width, data) (TPASTE2(Swap, width)(data))
  #define Dfu_format_dfu_to_mcu_data(width, data) (TPASTE2(Swap, width)(data))
  #define dfu_format_mcu_to_dfu_data(width, data) (TPASTE2(swap, width)(data))
  #define dfu_format_dfu_to_mcu_data(width, data) (TPASTE2(swap, width)(data))
#else // BIG_ENDIAN_MCU
  #define Dfu_format_mcu_to_dfu_data(width, data) ((TPASTE2(U, width))(data))
  #define Dfu_format_dfu_to_mcu_data(width, data) ((TPASTE2(U, width))(data))
  #define dfu_format_mcu_to_dfu_data(width, data) ((TPASTE2(U, width))(data))
  #define dfu_format_dfu_to_mcu_data(width, data) ((TPASTE2(U, width))(data))
#endif
//! @}


// Command groups
#define CMD_GRP_DNLOAD                    0x01
#define CMD_GRP_UPLOAD                    0x03
#define CMD_GRP_EXEC                      0x04
#define CMD_GRP_SELECT                    0x06

// CMD_GRP_DNLOAD commands
#define CMD_PROGRAM_START                 0x00

// CMD_GRP_UPLOAD commands
#define CMD_READ_MEMORY                   0x00
#define CMD_BLANK_CHECK                   0x01

// CMD_GRP_EXEC commands
#define CMD_ERASE                         0x00
#define CMD_START_APPLI                   0x03

// CMD_GRP_SELECT commands
#define CMD_SELECT_MEMORY                 0x03

// CMD_ERASE arguments
#define CMD_ERASE_ARG_CHIP                0xFF

// CMD_START_APPLI arguments
#define CMD_START_APPLI_ARG_RESET         0x00
#define CMD_START_APPLI_ARG_NO_RESET      0x01

// CMD_SELECT_MEMORY arguments
#define CMD_SELECT_MEMORY_ARG_UNIT        0x00
#define CMD_SELECT_MEMORY_ARG_PAGE        0x01

// Memory units (CMD_SELECT_MEMORY_ARG_UNIT arguments)
#define MEM_FLASH                         0x00
#define MEM_EEPROM                        0x01
#define MEM_SECURITY                      0x02
#define MEM_CONFIGURATION                 0x03
#define MEM_BOOTLOADER                    0x04
#define MEM_SIGNATURE                     0x05
#define MEM_USER                          0x06

// Number of memory units
#define MEM_COUNT                         (MEM_USER + 1)

// Product information addresses
#define PRODUCT_MANUFACTURER_ID_ADDRESS   0x00
#define PRODUCT_FAMILY_ID_ADDRESS         0x01
#define PRODUCT_ID_ADDRESS                0x02
#define PRODUCT_REVISION_ADDRESS          0x03

// Number of product information items
#define PRODUCT_INF_ITEM_COUNT            (PRODUCT_REVISION_ADDRESS + 1)

// ISP information addresses
#define ISP_VERSION_ADDRESS               0x00
#define ISP_ID0_ADDRESS                   0x01
#define ISP_ID1_ADDRESS                   0x02

// Number of ISP information items
#define ISP_INF_ITEM_COUNT                (ISP_ID1_ADDRESS + 1)


//_____ D E C L A R A T I O N S ____________________________________________

static void mem_flash_read(void *dst, U32 src, size_t nbytes);
static void mem_flash_write(U32 dst, const void *src, size_t nbytes);
static void mem_security_read(void *dst, U32 src, size_t nbytes);
static void mem_security_write(U32 dst, const void *src, size_t nbytes);
static void mem_configuration_read(void *dst, U32 src, size_t nbytes);
static void mem_configuration_write(U32 dst, const void *src, size_t nbytes);
static void mem_bootloader_read(void *dst, U32 src, size_t nbytes);
static void mem_signature_read(void *dst, U32 src, size_t nbytes);
static void mem_user_read(void *dst, U32 src, size_t nbytes);
static void mem_user_write(U32 dst, const void *src, size_t nbytes);


//_____ D E F I N I T I O N S ______________________________________________

typedef union
{
  U32 long_address;
  struct
  {
    unsigned int page         : 16;
    unsigned int page_offset  : 16;
  };
} address_t;


static U8 PRODUCT_INF[PRODUCT_INF_ITEM_COUNT] =
{
  PRODUCT_MANUFACTURER_ID,
  PRODUCT_FAMILY_ID
};

static const U8 ISP_INF[ISP_INF_ITEM_COUNT] =
{
  ISP_VERSION,
  ISP_ID0,
  ISP_ID1
};

static address_t MEMORY_END_ADDRESS[MEM_COUNT] =
{
  // MEM_FLASH
  {AVR32_FLASH_SIZE - 1},

  // MEM_EEPROM
  {0},

  // MEM_SECURITY
  {0},

  // MEM_CONFIGURATION
  {AVR32_FLASHC_GPF_NUM - 1},

  // MEM_BOOTLOADER
  {sizeof(ISP_INF) - 1},

  // MEM_SIGNATURE
  {sizeof(PRODUCT_INF) - 1},

  // MEM_USER
  {AVR32_FLASHC_USER_PAGE_SIZE - 1}
};

static const struct
{
  void (*read)(void *dst, U32 src, size_t nbytes);
  void (*write)(U32 dst, const void *src, size_t nbytes);
} MEMORY_ACCESS[MEM_COUNT] =
{
  // MEM_FLASH
  {
    mem_flash_read,
    mem_flash_write
  },

  // MEM_EEPROM
  {
    NULL,
    NULL
  },

  // MEM_SECURITY
  {
    mem_security_read,
    mem_security_write
  },

  // MEM_CONFIGURATION
  {
    mem_configuration_read,
    mem_configuration_write
  },

  // MEM_BOOTLOADER
  {
    mem_bootloader_read,
    NULL
  },

  // MEM_SIGNATURE
  {
    mem_signature_read,
    NULL
  },

  // MEM_USER
  {
    mem_user_read,
    mem_user_write
  }
};

static const U8 MEMORY_ERASE_VALUE[MEM_COUNT] =
{
  // MEM_FLASH
  0xFF,

  // MEM_EEPROM
  0xFF,

  // MEM_SECURITY
  0x00,

  // MEM_CONFIGURATION
  0x01,

  // MEM_BOOTLOADER
  0x00,

  // MEM_SIGNATURE
  0x00,

  // MEM_USER
  0xFF
};

static Bool security_active = TRUE;
static S32 length;
static U8 cmd_group = CMD_GRP_EXEC;
static U8 cmd = CMD_START_APPLI;
static U8 memory = MEM_FLASH;
static address_t start_address;
static address_t end_address;
static U32 data_bytes;
static U8 dfu_frame[DFU_FRAME_LENGTH];
static U8 dfu_status = STATUS_OK;

U8 usb_dfu_status = STATUS_OK;
U8 usb_dfu_state = STATE_dfuIDLE;


static Bool get_and_check_mem_range(void)
{
  start_address.page_offset = Usb_read_endpoint_data(EP_CONTROL, 16);
  start_address.page_offset = dfu_format_dfu_to_mcu_data(16, start_address.page_offset);
  end_address.page_offset = Usb_read_endpoint_data(EP_CONTROL, 16);
  end_address.page_offset = dfu_format_dfu_to_mcu_data(16, end_address.page_offset);

  if (end_address.long_address < start_address.long_address ||
      end_address.long_address > MEMORY_END_ADDRESS[memory].long_address)
  {
    dfu_status = STATUS_errADDRESS;
    return FALSE;
  }

  data_bytes = end_address.long_address - start_address.long_address + 1;
  return TRUE;
}


static void mem_flash_read(void *dst, U32 src, size_t nbytes)
{
  memcpy(dst, AVR32_FLASH + src, nbytes);
}


static void mem_flash_write(U32 dst, const void *src, size_t nbytes)
{
  flashc_memcpy(AVR32_FLASH + dst, src, nbytes, FALSE);
}


static void mem_security_read(void *dst, U32 src, size_t nbytes)
{
  if (nbytes)
    *(U8 *)dst = flashc_is_security_bit_active();
}


static void mem_security_write(U32 dst, const void *src, size_t nbytes)
{
  if (nbytes && *(U8 *)src)
  {
    security_active = TRUE;
    flashc_activate_security_bit();
  }
}


static void mem_configuration_read(void *dst, U32 src, size_t nbytes)
{
  U8 *dest = dst;
  while (nbytes--)
    *dest++ = flashc_read_gp_fuse_bit(src++);
}


static void mem_configuration_write(U32 dst, const void *src, size_t nbytes)
{
  const U8 *source = src;
  while (nbytes--)
    flashc_set_gp_fuse_bit(dst++, *source++);
}


static void mem_bootloader_read(void *dst, U32 src, size_t nbytes)
{
  memcpy(dst, ISP_INF + src, nbytes);
}


static void mem_signature_read(void *dst, U32 src, size_t nbytes)
{
  memcpy(dst, PRODUCT_INF + src, nbytes);
}


static void mem_user_read(void *dst, U32 src, size_t nbytes)
{
  memcpy(dst, (U8 *)AVR32_FLASHC_USER_PAGE + src, nbytes);
}


static void mem_user_write(U32 dst, const void *src, size_t nbytes)
{
  flashc_memcpy(AVR32_FLASHC_USER_PAGE + dst, src, nbytes, TRUE);
}


static void erase_check_mem(void)
{
  U8 *frame;
  U32 frame_bytes;

  while (data_bytes)
  {
    frame = dfu_frame;
    frame_bytes = min(data_bytes, DFU_FRAME_LENGTH);
    data_bytes -= frame_bytes;

    MEMORY_ACCESS[memory].read(frame, start_address.long_address, frame_bytes);

    while (frame_bytes--)
    {
      if (*frame++ != MEMORY_ERASE_VALUE[memory])
      {
        dfu_status = STATUS_errCHECK_ERASED;
        return;
      }
      start_address.long_address++;
    }
  }
}


static void read_mem(void)
{
  void *frame;
  U32 frame_bytes;

  while (data_bytes)
  {
    frame = dfu_frame;
    frame_bytes = min(data_bytes, DFU_FRAME_LENGTH);
    data_bytes -= frame_bytes;

    MEMORY_ACCESS[memory].read(frame, start_address.long_address, frame_bytes);
    start_address.long_address += frame_bytes;

    while (frame_bytes)
    {
      while (!Is_usb_control_in_ready());

      Usb_reset_endpoint_fifo_access(EP_CONTROL);
      frame_bytes = usb_write_ep_txpacket(EP_CONTROL, frame, frame_bytes, (const void **)&frame);
      Usb_ack_control_in_ready_send();
    }
  }
}


static void write_mem(void)
{
  void *frame;
  U32 frame_bytes, unaligned_frame_bytes;

  data_bytes += Get_align(start_address.long_address, EP_CONTROL_LENGTH);
  length -= EP_CONTROL_LENGTH + Align_up(data_bytes, EP_CONTROL_LENGTH);

  while (data_bytes)
  {
    frame = dfu_frame;
    frame_bytes = min(data_bytes, DFU_FRAME_LENGTH);
    unaligned_frame_bytes = frame_bytes - Get_align(start_address.long_address, EP_CONTROL_LENGTH);
    data_bytes -= frame_bytes;

    while (frame_bytes)
    {
      while (!Is_usb_control_out_received());

      Usb_reset_endpoint_fifo_access(EP_CONTROL);
      frame_bytes = usb_read_ep_rxpacket(EP_CONTROL, frame, frame_bytes, &frame);
      Usb_ack_control_out_received_free();
    }

    MEMORY_ACCESS[memory].write(start_address.long_address,
                                dfu_frame + Get_align(start_address.long_address, EP_CONTROL_LENGTH),
                                unaligned_frame_bytes);
    start_address.long_address += unaligned_frame_bytes;
  }

  while (length > 0)
  {
    while (!Is_usb_control_out_received());
    Usb_ack_control_out_received_free();
    length -= EP_CONTROL_LENGTH;
  }
}


void usb_dfu_init(void)
{
  PRODUCT_INF[PRODUCT_ID_ADDRESS] =
    Rd_bitfield(Get_system_register(AVR32_CONFIG0), AVR32_CONFIG0_PROCESSORID_MASK);
  PRODUCT_INF[PRODUCT_REVISION_ADDRESS] =
    Rd_bitfield(Get_system_register(AVR32_CONFIG0), AVR32_CONFIG0_PROCESSORREVISION_MASK);

  MEMORY_END_ADDRESS[MEM_FLASH].long_address = flashc_get_flash_size() - 1;

  security_active = flashc_is_security_bit_active();
}


static void usb_dfu_stop(void)
{
  Usb_ack_control_out_received_free();
  Usb_ack_control_in_ready_send();

  while (!Is_usb_setup_received());
  Usb_ack_setup_received_free();
  Usb_ack_control_in_ready_send();

  while (!Is_usb_control_in_ready());

  Disable_global_interrupt();
  Usb_disable();
  (void)Is_usb_enabled();
  Enable_global_interrupt();
  Usb_disable_otg_pad();
}


/*! \brief Management of the DFU_DNLOAD requests.
 */
Bool usb_dfu_dnload(void)
{
  static U8 tmp_memory;
  static U16 tmp_page;

  dfu_status = STATUS_OK;

  Usb_read_endpoint_data(EP_CONTROL, 16); // Dummy read: wValue.
  Usb_read_endpoint_data(EP_CONTROL, 16); // Dummy read: wIndex.
  length = Usb_read_endpoint_data(EP_CONTROL, 16);
  length = usb_format_usb_to_mcu_data(16, length);
  Usb_ack_setup_received_free();

  if (length)
  {
    while (!Is_usb_control_out_received());
    Usb_reset_endpoint_fifo_access(EP_CONTROL);

    cmd_group = Usb_read_endpoint_data(EP_CONTROL, 8);
    cmd = Usb_read_endpoint_data(EP_CONTROL, 8);
    switch (cmd_group)
    {
    case CMD_GRP_DNLOAD:
      if (security_active)
      {
        goto unsupported_request;
      }
      led_active();
      if (!get_and_check_mem_range()) {
        break;
      }
      if (!MEMORY_ACCESS[memory].write)
      {
        dfu_status = STATUS_errWRITE;
        break;
      }
      switch (cmd)
      {
      case CMD_PROGRAM_START:
        Usb_ack_control_out_received_free();
        write_mem();
        break;

      default:
        goto unsupported_request;
      }
      break;

    case CMD_GRP_UPLOAD:
      if (security_active)
      {
        goto unsupported_request;
      }
      led_active();
      if (!get_and_check_mem_range()) {
        break;
      }
      if (!MEMORY_ACCESS[memory].read)
      {
        dfu_status = STATUS_errVERIFY;
        break;
      }
      switch (cmd)
      {
      case CMD_READ_MEMORY:
        break;

      case CMD_BLANK_CHECK:
        led_active();
        erase_check_mem();
        led_normal();
        break;

      default:
        goto unsupported_request;
      }
      break;

    case CMD_GRP_EXEC:
      switch (cmd)
      {
      case CMD_ERASE:
        switch (Usb_read_endpoint_data(EP_CONTROL, 8))
        {
        case CMD_ERASE_ARG_CHIP:
          led_active();
          memory = MEM_FLASH;
          end_address.long_address = start_address.long_address = 0;
          flashc_lock_all_regions(FALSE);
          flashc_erase_all_pages(FALSE);
          security_active = FALSE;
          led_normal();
          break;

        default:
          goto unsupported_request;
        }
        break;

      case CMD_START_APPLI:
        switch (Usb_read_endpoint_data(EP_CONTROL, 8))
        {
        case CMD_START_APPLI_ARG_RESET:
          usb_dfu_stop();
          Disable_global_interrupt();
          AVR32_WDT.ctrl = AVR32_WDT_CTRL_EN_MASK |
                           (10 << AVR32_WDT_CTRL_PSEL_OFFSET) |
                           (AVR32_WDT_KEY_VALUE << AVR32_WDT_CTRL_KEY_OFFSET);
          AVR32_WDT.ctrl = AVR32_WDT_CTRL_EN_MASK |
                           (10 << AVR32_WDT_CTRL_PSEL_OFFSET) |
                           ((~AVR32_WDT_KEY_VALUE << AVR32_WDT_CTRL_KEY_OFFSET) & AVR32_WDT_CTRL_KEY_MASK);
          while (1);

        case CMD_START_APPLI_ARG_NO_RESET:
          usb_dfu_stop();
          sys_clk_gen_stop();
          wait_10_ms();
          boot_program();
          break;

        default:
          goto unsupported_request;
        }
        break;

      default:
        goto unsupported_request;
      }
      break;

    case CMD_GRP_SELECT:
      if (security_active)
      {
        goto unsupported_request;
      }
      switch (cmd)
      {
      case CMD_SELECT_MEMORY:
        switch (Usb_read_endpoint_data(EP_CONTROL, 8))
        {
        case CMD_SELECT_MEMORY_ARG_UNIT:
          switch (tmp_memory = Usb_read_endpoint_data(EP_CONTROL, 8))
          {
          case MEM_FLASH:
          case MEM_EEPROM:
          case MEM_SECURITY:
          case MEM_CONFIGURATION:
          case MEM_BOOTLOADER:
          case MEM_SIGNATURE:
          case MEM_USER:
            memory = tmp_memory;
            end_address.long_address = start_address.long_address = 0;
            break;

          default:
            dfu_status = STATUS_errADDRESS;
          }
          break;

        case CMD_SELECT_MEMORY_ARG_PAGE:
          MSB(tmp_page) = Usb_read_endpoint_data(EP_CONTROL, 8);
          LSB(tmp_page) = Usb_read_endpoint_data(EP_CONTROL, 8);
          if (tmp_page <= MEMORY_END_ADDRESS[memory].page)
          {
            end_address.page = start_address.page = tmp_page;
          }
          else dfu_status = STATUS_errADDRESS;
          break;

        default:
          goto unsupported_request;
        }
        break;

      default:
        goto unsupported_request;
      }
      break;

    default:
unsupported_request:
      dfu_status = STATUS_errSTALLEDPKT;
    }

    Usb_ack_control_out_received_free();
  }

  usb_dfu_status = dfu_status;
  if (usb_dfu_status != STATUS_OK &&
      usb_dfu_status != STATUS_errCHECK_ERASED) return FALSE;

  Usb_ack_control_in_ready_send();
  while (!Is_usb_control_in_ready());
  return TRUE;
}


/*! \brief Management of the DFU_UPLOAD requests.
 */
Bool usb_dfu_upload(void)
{
  Usb_ack_setup_received_free();
  Usb_reset_endpoint_fifo_access(EP_CONTROL);

  if (cmd_group == CMD_GRP_UPLOAD &&
      (dfu_status == STATUS_OK ||
       dfu_status == STATUS_errCHECK_ERASED))
  {
    switch (cmd)
    {
    case CMD_READ_MEMORY:
      led_active();
      read_mem();
      led_normal();
      break;

    case CMD_BLANK_CHECK:
      Usb_write_endpoint_data(EP_CONTROL, 16, dfu_format_mcu_to_dfu_data(16, start_address.page_offset));
      break;

    default:
      goto unsupported_request;
    }
  }
  else
  {
unsupported_request:
    dfu_status = STATUS_errSTALLEDPKT;
  }

  usb_dfu_status = dfu_status;
  if (usb_dfu_status != STATUS_OK &&
      usb_dfu_status != STATUS_errCHECK_ERASED) return FALSE;

  if (Usb_byte_count(EP_CONTROL)) Usb_ack_control_in_ready_send();
  while (!Is_usb_control_out_received());
  Usb_ack_control_out_received_free();
  return TRUE;
}


#endif  // USB_DEVICE_FEATURE == ENABLED
