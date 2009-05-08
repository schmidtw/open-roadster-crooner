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

#ifndef __CROONER_2_0_H__
#define __CROONER_2_0_H__

#include <avr32/abi.h>
#include <avr32/io.h>

/*----------------------------------------------------------------------------*/
/* Oscillator Settings                                                        */
/*----------------------------------------------------------------------------*/
#define FOSC0           12000000
#define OSC0_STARTUP    PM__2048_cycles

/*----------------------------------------------------------------------------*/
/* PLL Settings (132,000,000 Hz)                                              */
/*----------------------------------------------------------------------------*/
#define CPU_PLL_ENABLED             1
#define CPU_PLL_MULTIPLIER          10
#define CPU_PLL_DIVIDER             1
#define CPU_PLL_DIVIDE_OUTPUT_BY_2  false
#define CPU_PLL_STARTUP             16
#define INITIAL_CPU_SPEED           66000000
#define INITIAL_PBA_SPEED           33000000
#define INITIAL_PBB_SPEED           66000000

/*----------------------------------------------------------------------------*/
/* USB Interface                                                              */
/*----------------------------------------------------------------------------*/
#define USB_ID  AVR32_USBB_USB_ID_0_2

/*----------------------------------------------------------------------------*/
/* Attached LEDs                                                              */
/*----------------------------------------------------------------------------*/
#define LED_BLUE    AVR32_PIN_PB19
#define LED_GREEN   AVR32_PIN_PB20
#define LED_RED     AVR32_PIN_PB21

/*----------------------------------------------------------------------------*/
/* iBus Interface                                                             */
/*----------------------------------------------------------------------------*/
#define IBUS_USART          (&AVR32_USART1)
#define IBUS_USART_ISR      ISR__USART_1
#define IBUS_RX_PDCA_PID    3
#define IBUS_RX_PIN         AVR32_USART1_RXD_0_0_PIN
#define IBUS_RX_FUNCTION    AVR32_USART1_RXD_0_0_FUNCTION
#define IBUS_TX_PDCA_PID    11
#define IBUS_TX_PIN         AVR32_USART1_TXD_0_0_PIN
#define IBUS_TX_FUNCTION    AVR32_USART1_TXD_0_0_FUNCTION
#define IBUS_CTS_PIN        AVR32_USART1_CTS_0_0_PIN
#define IBUS_CTS_FUNCTION   AVR32_USART1_CTS_0_0_FUNCTION
#define IBUS_RTS_PIN        AVR32_USART1_RTS_0_0_PIN
#define IBUS_RTS_FUNCTION   AVR32_USART1_RTS_0_0_FUNCTION

/*----------------------------------------------------------------------------*/
/* Debug In/Out Interface                                                     */
/*----------------------------------------------------------------------------*/
#define DEBUG_USART         (&AVR32_USART2)
#define DEBUG_USART_ISR     ISR__USART_2
#define DEBUG_RX_PDCA_PID   4
#define DEBUG_RX_PIN        AVR32_USART2_RXD_0_0_PIN
#define DEBUG_RX_FUNCTION   AVR32_USART2_RXD_0_0_FUNCTION
#define DEBUG_TX_PDCA_PID   12
#define DEBUG_TX_PIN        AVR32_USART2_TXD_0_0_PIN
#define DEBUG_TX_FUNCTION   AVR32_USART2_TXD_0_0_FUNCTION
#define DEBUG_BAUD_RATE     460800

/*----------------------------------------------------------------------------*/
/* SD/MMC Interface                                                           */
/*----------------------------------------------------------------------------*/
#define MC_WP_PIN                   AVR32_PIN_PB17
#define MC_CD_PIN                   AVR32_PIN_PB18
#define MC_CD_ISR                   ISR__GPIO_6
#define MC_WP_ACTIVE_LOW            0
#define MC_CD_ACTIVE_LOW            1
#define MC_CS                       0
#define MC_SPI                      (&AVR32_SPI0)
#define MC_SPI_ISR                  ISR__SPI0
#define MC_CS_PIN                   AVR32_SPI0_NPCS_0_0_PIN
#define MC_CS_FUNCTION              AVR32_SPI0_NPCS_0_0_FUNCTION
#define MC_MOSI_PIN                 AVR32_SPI0_MOSI_0_0_PIN
#define MC_MOSI_FUNCTION            AVR32_SPI0_MOSI_0_0_FUNCTION
#define MC_MISO_PIN                 AVR32_SPI0_MISO_0_0_PIN
#define MC_MISO_FUNCTION            AVR32_SPI0_MISO_0_0_FUNCTION
#define MC_SCK_PIN                  AVR32_SPI0_SCK_0_0_PIN
#define MC_SCK_FUNCTION             AVR32_SPI0_SCK_0_0_FUNCTION
#define MC_PDCA_RX_PERIPHERAL_ID    AVR32_PDCA_PID_SPI0_RX
#define MC_PDCA_TX_PERIPHERAL_ID    AVR32_PDCA_PID_SPI0_TX

/*----------------------------------------------------------------------------*/
/* DAC Interface                                                              */
/*----------------------------------------------------------------------------*/
#define AUDIO_DAC_PRESENT       1

#define AUDIO_DAC               (&AVR32_ABDAC)
#define AUDIO_DAC_ISR           ISR__DAC
#define AUDIO_DAC_MUTE_PIN      AVR32_PIN_PB07
#define AUDIO_DAC_R_PIN         AVR32_ABDAC_DATA_0_0_PIN
#define AUDIO_DAC_R_FUNCTION    AVR32_ABDAC_DATA_0_0_FUNCTION
#define AUDIO_DAC_L_PIN         AVR32_ABDAC_DATA_1_0_PIN
#define AUDIO_DAC_L_FUNCTION    AVR32_ABDAC_DATA_1_0_FUNCTION

/*----------------------------------------------------------------------------*/
/* PDCA Interface                                                             */
/*----------------------------------------------------------------------------*/
#define PDCA_CHANNEL_ID_MC_RX   0
#define PDCA_CHANNEL_ID_MC_TX   1
#define PDCA_CHANNEL_ID_DAC     2
#define PDCA_CHANNEL_ID_IBUS_TX 3

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
