/* ----------------------------------------------------------------------------
 *         SAM Software Package License
 * ----------------------------------------------------------------------------
 * Copyright (c) 2015, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */

/**
 *  \page usart_iso7816 USART ISO7816 Example
 *
 *  \section Purpose
 *  This example sends ISO 7816 commands to a smartcard connected to the
 *  Xplained board on samA5D4x.
 *
 *  \section Requirements
 *  This example can be used on SAMA5D2x Xplained board
 *  PB28 IO smartcard
 *  PB30 CLK smartcard
 *  ** with a Fieldbus shield.
 *  PB29 MODE_VCC
 *  PB31 STOP
 *  PC0  RST
 *  \section Description
 *  The iso7816 software provide in this examples is use to transform APDU
 *  commands to TPDU commands for the smart card.
 *  The iso7816 provide here is for the protocol T=0 only.
 *  The send and the receive of a character is made under polling.
 *  In the file ISO7816_Init is defined all pins of the card. User must have to
 *  change this pins according to his environment.
 *  The driver is compliant with CASE 1, 2, 3 of the ISO7816-4 specification.
 *
 *  \section Usage
 *  -# Build the program and download it inside the evaluation board. Please
 *     refer to the <a href="http://www.atmel.com/dyn/resources/prod_documents/6421B.pdf">SAM-BA User Guide</a>,
 *     the <a href="http://www.atmel.com/dyn/resources/prod_documents/doc6310.pdf">GNU-Based Software Development</a>
 *     application note or to the <a href="http://www.iar.com/website1/1.0.1.0/78/1/">IAR EWARM User and reference guides</a>,
 *     depending on your chosen solution.
 *  -# On the computer, open and configure a terminal application (e.g.
 *     HyperTerminal on Microsoft Windows) with these settings:
 *        - 115200 bauds
 *        - 8 data bits
 *        - No parity
 *        - 1 stop bit
 *        - No flow control
 *  -# Connect the card reader to SAMA5D2 Xplained board:
 *        <table border="1" cellpadding="2" cellspacing="0">
 *        <tr><td> C1: Vcc:   7816_3V5V </td><td> C5: Gnd</td> <td> C4: RFU</td></tr>
 *        <tr><td> C2: Reset: 7816_RST</td> <td>  C6: Vpp</td> <td> C8: RFU</td></tr>
 *        <tr><td> C3: Clock: 7816_CLK</td> <td>  C7: 7816_IO</td> </tr>
 *        </table>
 *     The push button on board (BP1) is used to simulate the smartcard insertion or removal.
 *  -# Start the application. The following traces shall appear on the terminal:
 *     \code
 *      -- USART ISO7816 Example xxx --
 *      -- SAMxxxxx-xx
 *      -- Compiled: xxx xx xxxx xx:xx:xx --
 *      Display the ATR
 *     \endcode
 *
 *   \section References
 *  - usart_iso7816/main.c
 *  - iso7816_4.c
 *  - pio.h
 *  - usart.h
 *
 */

/** \file
 *
 *  This file contains all the specific code for the usart_iso7816 example.
 *
 */


/*------------------------------------------------------------------------------
 *          Headers
 *------------------------------------------------------------------------------*/

#include "board.h"
#include "chip.h"
#include "trace.h"
#include "compiler.h"
#include "timer.h"

#include "peripherals/aic.h"
#include "peripherals/pmc.h"
#include "peripherals/wdt.h"
#include "peripherals/pio.h"
#include "peripherals/pit.h"
#include "peripherals/usart_iso7816_4.h"

#include "misc/led.h"
#include "misc/console.h"

#include "power/act8945a.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/*------------------------------------------------------------------------------
 *         Internal definitions
 *------------------------------------------------------------------------------*/

/** LED0 blink time, LED1 blink half this time, in ms */
#define BLINK_PERIOD        1000

/** Delay for pushbutton debouncing (in milliseconds). */
#define DEBOUNCE_TIME       500

/** Maximum number of handled led */
#define MAX_LEDS            3

#define PINS_FLEXCOM0_USART_ISO7816_IOS1 {\
	{ PIO_GROUP_B, PIO_PB28C_FLEXCOM0_IO0, PIO_PERIPH_C, PIO_DEFAULT },\
	{ PIO_GROUP_B, PIO_PB30C_FLEXCOM0_IO2, PIO_PERIPH_C, PIO_DEFAULT },\
}

/** NCN4555MN usart pin definition */
/* IO TXD bidirectionnal */
/* SCK	*/
#define PIN_COM2_ISO7816  PINS_FLEXCOM0_USART_ISO7816_IOS1

/** NCN4555MN STOP pin definition */
/* Connected to CTS */
#define PIN_STOP_ISO7816 { PIO_GROUP_B, PIO_PB31C_FLEXCOM0_IO3, PIO_OUTPUT_1, PIO_PULLUP }

/** NCN4555MN MOD VCC pin definition */
/* Connected to RXD */
#define PIN_MODE_VCC_ISO7816 { PIO_GROUP_B, PIO_PB29C_FLEXCOM0_IO1, PIO_OUTPUT_1, PIO_PULLUP }

/** NCN4555MN RST pin definition */
/* Connected to RTS */
#define PIN_RST_ISO7816 { PIO_GROUP_C, PIO_PC0C_FLEXCOM0_IO4, PIO_OUTPUT_0, PIO_DEFAULT }

/** Maximum uc_size in bytes of the smartcard answer to a uc_command. */
#define MAX_ANSWER_SIZE         10

/** Maximum ATR uc_size in bytes. */
#define MAX_ATR_SIZE            55

/** Define the baudrate of ISO7816 mode. */
#define ISO7816_BAUDRATE        9600

/** Define the FI_DI_RATIO filed value. */
#define ISO7816_FI_DI           372

/*------------------------------------------------------------------------------
 *         Internal variables
 *------------------------------------------------------------------------------*/

#ifdef CONFIG_HAVE_PMIC_ACT8945A
	struct _pin act8945a_pins[] = ACT8945A_PINS;

	struct _twi_desc act8945a_twid = {
		.addr = ACT8945A_ADDR,
		.freq = ACT8945A_FREQ,
		.transfert_mode = TWID_MODE_POLLING
	};

	struct _act8945a act8945a = {
		.desc = {
			.pin_chglev = ACT8945A_PIN_CHGLEV,
			.pin_irq = ACT8945A_PIN_IRQ,
			.pin_lbo = ACT8945A_PIN_LBO
		}
	};
#endif

#ifdef PINS_PUSHBUTTONS
/** Pushbutton \#1 pin instance. */
static const struct _pin button_pins[] = PINS_PUSHBUTTONS;
#endif

volatile bool led_status[MAX_LEDS];

/*------------------------------------------------------------------------------
 *         Internal variables ISO7816
 *------------------------------------------------------------------------------*/

/** Test command #1.*/
static const uint8_t testCommand1[] = {0x00, 0x10, 0x00, 0x00};
/** Test command #2.*/
static const uint8_t testCommand2[] = {0x00, 0x20, 0x00, 0x00, 0x02};
/** Test command #3.*/
static const uint8_t testCommand3[] = {0x00, 0x30, 0x00, 0x00, 0x02, 0x0A, 0x0B};

struct _iso7816_desc iso7816_desc = {
	.pin_stop = PIN_STOP_ISO7816,
	.pin_mod_vcc = PIN_MODE_VCC_ISO7816,
	.pin_rst = PIN_RST_ISO7816,

	.addr = USART0,
	.id = ID_USART0,
};

struct _iso7816_opt iso7816_opt = {
	.protocol_type = US_MR_USART_MODE_IS07816_T_0,
	.clock_sel = US_MR_USCLKS_MCK,
	.char_length = US_MR_CHRL_8_BIT,
	.sync = 0,
	.parity_type = US_MR_PAR_EVEN,
	.num_stop_bits = US_MR_NBSTOP_2_BIT,
	.bit_order = 0,
	.inhibit_nack = 0,
	.dis_suc_nack = 0,

	.max_iterations = 3,
	.iso7816_hz = ISO7816_BAUDRATE * ISO7816_FI_DI,
	.fidi_ratio = ISO7816_FI_DI,
	.time_guard = 5,
};

static const struct _pin pins_com2[] = PIN_COM2_ISO7816;

uint8_t smartcard = 0;

/*------------------------------------------------------------------------------
 *         Internal functions
 *------------------------------------------------------------------------------*/

/**
 *  \brief Configure the Leds
 *
 */
static void _configure_leds(void)
{
	uint8_t i = 0;
	for (i = 0; i<MAX_LEDS; ++i) {
		led_configure(i);
		led_status[i] = 0;
	}
}

/**
 *  \brief Process Buttons Events
 *
 */
static void process_button_evt(uint8_t bt)
{
	if ( smartcard == 0) {
		printf( "-I- Smartcard inserted\n\r" ) ;
	} else {
		printf( "-I- Smartcard removed\n\r" ) ;
	}
	smartcard = ~smartcard;
}

/**
 *  \brief Handler for Buttons rising edge interrupt.
 *
 */
static void push_button_handler(uint32_t mask, uint32_t status, void* user_arg)
{
	int i = 0;
	(void)user_arg;
	for (i = 0; i < ARRAY_SIZE(button_pins); ++i) {
		if (status & button_pins[i].mask)
			process_button_evt(i);
	}
}

/**
 *  \brief Configure the Pushbuttons
 *
 *  Configure the PIO as inputs and generate corresponding interrupt when
 *  pressed or released.
 */
static void _configure_buttons(void)
{
	int i = 0;
	for (i = 0; i <  ARRAY_SIZE(button_pins); ++i)
	{
		/* Configure pios as inputs. */
		pio_configure(&button_pins[i], 1);
		/* Adjust pio debounce filter parameters, uses 10 Hz filter. */
		pio_set_debounce_filter(&button_pins[i], 10);
		/* Initialize pios interrupt with its handlers */
		pio_configure_it(&button_pins[i]);
		pio_add_handler_to_group(button_pins[i].group,
					 button_pins[i].mask,
					 push_button_handler,
					 NULL);
		pio_enable_it(button_pins);
	}
}

/*------------------------------------------------------------------------------
 *         Optional smartcard detection
 *------------------------------------------------------------------------------*/

/**
 * Displays a menu which enables the user to send several commands to the
 * smartcard and check its answers.
 */
static void _send_receive_commands( const struct _iso7816_desc* iso7816 )
{
    uint8_t pMessage[MAX_ANSWER_SIZE];
    uint8_t ucSize ;
    uint8_t ucKey ;
    uint8_t command;
    uint8_t i;

    /*  Clear message buffer */
    memset( pMessage, 0, sizeof( pMessage ) ) ;

    /*  Display menu */
    printf( "-I- Choose the command to send:\n\r" ) ;
    printf( "  1. " ) ;
    for ( i=0 ; i < sizeof( testCommand1 ) ; i++ ) {
        printf( "0x%X ", testCommand1[i] ) ;
    }
    printf( "\n\r  2. " ) ;
    for ( i=0 ; i < sizeof( testCommand2 ) ; i++ ) {
        printf( "0x%X ", testCommand2[i] ) ;
    }
    printf( "\n\r  3. " ) ;
    for ( i=0 ; i < sizeof( testCommand3 ) ; i++ ) {
        printf( "0x%X ", testCommand3[i] ) ;
    }
    printf( "\n\r" ) ;

    /*  Get user input */
    ucKey = 0 ;
    while ( ucKey != 'q' ) {
        printf( "\r                        " ) ;
        printf( "\rChoice ? (q to quit): " ) ;
        ucKey = console_get_char() ;
        printf( "%c", ucKey ) ;
        command = ucKey - '0';

        /*  Check user input */
        ucSize = 0 ;
        if ( command == 1 ) {
            printf( "\n\r-I- Sending command " ) ;
            for ( i=0 ; i < sizeof( testCommand1 ) ; i++ ) {
                printf( "0x%02X ", testCommand1[i] ) ;
            }
            printf( "...\n\r" ) ;
            ucSize = iso7816_xfr_block_TPDU_T0(iso7816, testCommand1, pMessage, sizeof( testCommand1 ) ) ;
        }
        else {
            if ( command == 2 ) {
                printf( "\n\r-I- Sending command " ) ;
                for ( i=0 ; i < sizeof( testCommand2 ) ; i++ ) {
                    printf("0x%02X ", testCommand2[i] ) ;
                }
                printf( "...\n\r" ) ;
                ucSize = iso7816_xfr_block_TPDU_T0(iso7816, testCommand2, pMessage, sizeof( testCommand2 ) ) ;
            }
			else {
                if ( command == 3 ) {
                    printf( "\n\r-I- Sending command " ) ;
                    for ( i=0 ; i < sizeof( testCommand3 ) ; i++ ) {
                        printf( "0x%02X ", testCommand3[i] ) ;
                    }
                    printf( "...\n\r" ) ;
                    ucSize = iso7816_xfr_block_TPDU_T0(iso7816, testCommand3, pMessage, sizeof( testCommand3 ) ) ;
                }
            }
       }

        /*  Output smartcard answer */
        if ( ucSize > 0 ) {
            printf( "\n\rAnswer: " ) ;
            for ( i=0 ; i < ucSize ; i++ ) {
                printf( "0x%02X ", pMessage[i] ) ;
            }
            printf( "\n\r" ) ;
        }
    }
    printf( "Exit ...\n\r" ) ;
}

/*------------------------------------------------------------------------------
 *         Exported functions
 *------------------------------------------------------------------------------*/

/**
 * Initializes the DBGU and ISO7816 driver, and starts some tests.
 * \return Unused (ANSI-C compatibility)
 */
extern int main( void )
{
    uint8_t Atr[MAX_ATR_SIZE] ;
    uint8_t size ;

	/* Disable watchdog */
	wdt_disable();

	/* Disable all PIO interrupts */
	pio_reset_all_it();

	/* Initialize console */
	console_configure(CONSOLE_BAUDRATE);
	console_clear_screen();
	console_reset_cursor();

	/* Configure PIT. Must be always ON, used for delay */
	printf("Configure PIT \n\r");
	timer_configure(BLINK_PERIOD);

    printf( "-- USART ISO7816 Example %s --\n\r", SOFTPACK_VERSION ) ;
    printf( "-- %s\n\r", BOARD_NAME ) ;
    printf( "-- Compiled: %s %s --\n\r", __DATE__, __TIME__ ) ;

#ifdef CONFIG_HAVE_PMIC_ACT8945A
	pio_configure(act8945a_pins, ARRAY_SIZE(act8945a_pins));
	if (act8945a_configure(&act8945a, &act8945a_twid)) {
		act8945a_set_regulator_voltage(&act8945a, 6, 2500);
		act8945a_enable_regulator(&act8945a, 6, true);
	} else {
		printf("--E-- Error initializing ACT8945A PMIC\n\r");
	}
#endif

	/* PIO configuration for LEDs */
	printf("Configure LED PIOs.\n\r");
	_configure_leds();
	led_set(LED_GREEN);
	timer_wait(500);
	led_clear(LED_GREEN);
	led_status[LED_BLUE] = 1;

	/* PIO configuration for Button, use to simulate card detection. */
	printf("Configure buttons with debouncing.\n\r");
	_configure_buttons();

	/* Configure Pios usart*/
	pio_configure(&pins_com2[0], ARRAY_SIZE(pins_com2));

	/* Init ISO7816 interface */
    iso7816_init(&iso7816_desc, &iso7816_opt);

	/* Warm reset */
    iso7816_warm_reset(&iso7816_desc);
	/*  Read ATR */
    memset( Atr, 0, sizeof(Atr) ) ;
    iso7816_get_data_block_ATR(&iso7816_desc, Atr, &size );
    /*  Decode ATR */
    iso7816_decode_ATR(Atr);

    /*  Allow user to send some commands */
    _send_receive_commands(&iso7816_desc);

	printf("\n\r Exit App \n\r");
    while (1) ;
}
