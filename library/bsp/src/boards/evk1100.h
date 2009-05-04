/*
 * Open Roadster Crooner
 *
 * Copyright (c) 2008  Weston Schmidt
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * In other words, you are welcome to use, share and improve this program.
 * You are forbidden to forbid anyone else to use, share and improve
 * what you give them.   Help stamp out software-hoarding!
 */

#ifndef __EVK100_H__
#define __EVK100_H__

#include <avr32/abi.h>
#include <avr32/io.h>

/*----------------------------------------------------------------------------*/
/* Oscillator Settings                                                        */
/*----------------------------------------------------------------------------*/
#define FOSC0           12000000
#define OSC0_STARTUP    PM__2048_cycles

/*----------------------------------------------------------------------------*/
/* PLL Settings (96,000,000 Hz)                                               */
/*----------------------------------------------------------------------------*/
#define CPU_PLL_ENABLED             0
#define CPU_PLL_MULTIPLIER          7
#define CPU_PLL_DIVIDER             1
#define CPU_PLL_DIVIDE_OUTPUT_BY_2  false
#define CPU_PLL_STARTUP             16
#define INITIAL_CPU_SPEED           48000000
#define INITIAL_PBA_SPEED           24000000
#define INITIAL_PBB_SPEED           24000000

/*----------------------------------------------------------------------------*/
/* USB Interface                                                              */
/*----------------------------------------------------------------------------*/
#define USB_ID  AVR32_USBB_USB_ID_0_0

/*----------------------------------------------------------------------------*/
/* Attached LEDs                                                              */
/*----------------------------------------------------------------------------*/
#define LED_BLUE    AVR32_PIN_PB19
#define LED_GREEN   AVR32_PIN_PB20
#define LED_RED     AVR32_PIN_PB21

/*----------------------------------------------------------------------------*/
/* iBus Interface                                                             */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/* Debug In/Out Interface                                                     */
/*----------------------------------------------------------------------------*/
#define DEBUG_USART         (&AVR32_USART0)
#define DEBUG_ISR_GROUP     5
#define DEBUG_RX_PDCA_PID   AVR32_PDCA_PID_USART0_RX
#define DEBUG_RX_PIN        AVR32_USART0_RXD_0_PIN
#define DEBUG_RX_FUNCTION   AVR32_USART0_RXD_0_FUNCTION
#define DEBUG_TX_PDCA_PID   AVR32_PDCA_PID_USART0_TX
#define DEBUG_TX_PIN        AVR32_USART0_TXD_0_PIN
#define DEBUG_TX_FUNCTION   AVR32_USART0_TXD_0_FUNCTION
#define DEBUG_BAUD_RATE     57600

/*----------------------------------------------------------------------------*/
/* SD/MMC Interface                                                           */
/*----------------------------------------------------------------------------*/
#define MC_WP_PIN                   AVR32_PIN_PA07
#define MC_CD_PIN                   AVR32_PIN_PA02
#define MC_WP_ACTIVE_LOW            1
#define MC_CD_ACTIVE_LOW            0
#define MC_CS                       0
#define MC_SPI                      (&AVR32_SPI1)
#define MC_CS_PIN                   AVR32_SPI1_NPCS_1_PIN
#define MC_CS_FUNCTION              AVR32_SPI1_NPCS_1_FUNCTION
#define MC_MOSI_PIN                 AVR32_SPI1_MOSI_0_PIN
#define MC_MOSI_FUNCTION            AVR32_SPI1_MOSI_0_FUNCTION
#define MC_MISO_PIN                 AVR32_SPI1_MISO_0_PIN
#define MC_MISO_FUNCTION            AVR32_SPI1_MISO_0_FUNCTION
#define MC_SCK_PIN                  AVR32_SPI1_SCK_0_PIN
#define MC_SCK_FUNCTION             AVR32_SPI1_SCK_0_FUNCTION
#define MC_PDCA_RX_PERIPHERAL_ID    AVR32_PDCA_PID_SPI0_RX
#define MC_PDCA_TX_PERIPHERAL_ID    AVR32_PDCA_PID_SPI0_TX

/*----------------------------------------------------------------------------*/
/* DAC Interface                                                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/* PDCA Interface                                                             */
/*----------------------------------------------------------------------------*/
#define PDCA_CHANNEL_ID_MC_RX   0
#define PDCA_CHANNEL_ID_MC_TX   1
#define PDCA_CHANNEL_ID_DAC     2

/*----------------------------------------------------------------------------*/
/* SDRAM Interface                                                            */
/*----------------------------------------------------------------------------*/
#define SDRAM_PRESENT   1

#define SDRAM_BANK_BITS 2       /* bits */
#define SDRAM_ROW_BITS  13      /* bits */
#define SDRAM_COL_BITS  9       /* bits */
#define SDRAM_CAS       2
#define SDRAM_tWR       14      /* ns */
#define SDRAM_tRC       60      /* ns */
#define SDRAM_tRP       15      /* ns */
#define SDRAM_tRCD      15      /* ns */
#define SDRAM_tRAS      37      /* ns */
#define SDRAM_tXSR      67      /* ns */
#define SDRAM_tREFRESH  7812    /* ns */
#define SDRAM_tRFC      66      /* ns */
#define SDRAM_tMRD      2       /* tCK */
#endif
