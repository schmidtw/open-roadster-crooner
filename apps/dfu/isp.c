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

#include "preprocessor.h"
#include "compiler.h"
#include "board.h"
#include "pm.h"
#include "flashc.h"
#include "conf_usb.h"
#include "usb_drv.h"
#include "usb_task.h"
#if USB_DEVICE_FEATURE == ENABLED
#include "usb_dfu.h"
#endif
#include "conf_isp.h"
#include "isp.h"


//_____ M A C R O S ________________________________________________________


//_____ D E F I N I T I O N S ______________________________________________

static S8 osc_freq_id = -1;


void wait_10_ms(void)
{
  Set_system_register(AVR32_COUNT, 0);
  while ((U32)Get_system_register(AVR32_COUNT) < (FRCOSC * 10 + 999) / 1000);
}


/*!
 *  Start the generation of system clocks with USB autobaud
 */
void sys_clk_gen_start(void)
{
  #define OSC_FREQ_COUNT  3

  #define MAX_OSC_FREQ    16000000

  static const U8 OSC_PLL_MUL[OSC_FREQ_COUNT] =
  {
    48000000 /  8000000 - 1,
    48000000 / 12000000 - 1,
    48000000 / 16000000 - 1
  };

  volatile avr32_pm_t *const pm = &AVR32_PM;

  Bool sav_glob_int_en;

  if ((sav_glob_int_en = Is_global_interrupt_enabled())) Disable_global_interrupt();

  if (osc_freq_id < 0)
  {
    // Start the oscillator
    pm_enable_osc_crystal(pm, MAX_OSC_FREQ);
    pm_enable_clk_no_wait(pm, ATPASTE3(AVR32_PM_OSCCTRL, ISP_OSC, _STARTUP_16384_RCOSC));

    for (osc_freq_id = 0; !Is_usb_sof(); osc_freq_id++)
    {
      if (osc_freq_id >= OSC_FREQ_COUNT) osc_freq_id = 0;

      Usb_freeze_clock();

      // Set PLL0 VCO @ 96 MHz
      pm_pll_setup(pm, 0,                         // pll
                       OSC_PLL_MUL[osc_freq_id],  // mul
                       0,                         // div
                       ISP_OSC,                   // osc
                       63);                       // lockcount

      // Set PLL0 @ 48 MHz
      pm_pll_set_option(pm, 0,  // pll
                            1,  // pll_freq
                            1,  // pll_div2
                            0); // pll_wbwdisable

      // Enable PLL0
      pm_pll_enable(pm, 0);

      // Wait for PLL0 locked with a 10-ms time-out
      wait_10_ms();
      if (!(pm->poscsr & AVR32_PM_POSCSR_LOCK0_MASK)) continue;

      // Setup USB GCLK
      pm_gc_setup(pm, AVR32_PM_GCLK_USBB, // gc
                      1,                  // osc_or_pll: use Osc (if 0) or PLL (if 1)
                      0,                  // pll_osc: select Osc0/PLL0 or Osc1/PLL1
                      0,                  // diven
                      0);                 // div

      // Enable USB GCLK
      pm_gc_enable(pm, AVR32_PM_GCLK_USBB);

      Usb_unfreeze_clock();

      wait_10_ms();
    }

    // Use 1 flash wait state
    flashc_set_wait_state(1);

    // Switch the main clock to PLL0
    pm_switch_to_clock(pm, AVR32_PM_MCCTRL_MCSEL_PLL0);

    // fPBA: 12 MHz
    // fPBB: 12 MHz
    // fHSB: 12 MHz
    pm_cksel(pm, 1,   // pbadiv
                 1,   // pbasel
                 1,   // pbbdiv
                 1,   // pbbsel
                 1,   // hsbdiv
                 1);  // hsbsel

    // Use 0 flash wait state
    flashc_set_wait_state(0);

    Usb_ack_sof();
  }

  if (sav_glob_int_en) Enable_global_interrupt();
}


/*!
 *  Stop the generation of system clocks and switch to RCOsc
 */
void sys_clk_gen_stop(void)
{
  volatile avr32_pm_t *const pm = &AVR32_PM;

  pm_gc_disable(pm, AVR32_PM_GCLK_USBB);
  pm_gc_setup(pm, AVR32_PM_GCLK_USBB, 0, 0, 0, 0);
  flashc_set_wait_state(1);
  pm_cksel(pm, 0, 0, 0, 0, 0, 0);
  pm_switch_to_clock(pm, AVR32_PM_MCCTRL_MCSEL_SLOW);
  flashc_set_wait_state(0);
  pm_pll_disable(pm, 0);
  pm_pll_set_option(pm, 0, 0, 0, 0);
  pm_pll_setup(pm, 0, 0, 0, 0, 0);
  pm_enable_clk_no_wait(pm, ATPASTE3(AVR32_PM_OSCCTRL, ISP_OSC, _STARTUP_0_RCOSC));
  pm_disable_clk(pm);
  pm_enable_osc_ext_clock(pm);
}


int main(void)
{
  wait_10_ms();

  usb_task_init();
#if USB_DEVICE_FEATURE == ENABLED
  usb_dfu_init();
#endif

  while (TRUE)
  {
    usb_task();
  }
}
