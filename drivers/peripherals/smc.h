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
*  \file
*
*  Definitions and function prototype for smc module
*/

#ifndef SMC_H_
#define SMC_H_

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#include "chip.h"

#include <stdbool.h>
#include <stdint.h>

/*----------------------------------------------------------------------------
 *         Macros
 *----------------------------------------------------------------------------*/

#define smc_pmecc_reset()             {PMECC->PMECC_CTRL = PMECC_CTRL_RST; }
#define smc_pmecc_or_reset()          {PMECC->PMECC_CTRL |= PMECC_CTRL_RST; }
#define smc_pmecc_data_phase()        {PMECC->PMECC_CTRL |= PMECC_CTRL_DATA; }
#define smc_pmecc_enable_write()      {PMECC->PMECC_CFG |= PMECC_CFG_NANDWR;}
#define smc_pmecc_enable_read()       {PMECC->PMECC_CFG &= (~PMECC_CFG_NANDWR);}
 
#define smc_pmecc_error_status()      (PMECC->PMECC_ISR )
#define smc_pmecc_enable()            {PMECC->PMECC_CTRL = PMECC_CTRL_ENABLE;}
#define smc_pmecc_disable()           {PMECC->PMECC_CTRL = PMECC_CTRL_DISABLE;}
#define smc_pmecc_auto_enable()       {PMECC->PMECC_CFG |= PMECC_CFG_AUTO;}
#define smc_pmecc_auto_disable()      {PMECC->PMECC_CFG &= (~PMECC_CFG_AUTO);}
#define smc_pmecc_auto_apare_en()     ((PMECC->PMECC_CFG & PMECC_CFG_SPAREEN) == PMECC_CFG_SPAREEN) 
#define smc_pmecc(i)                  (PMECC->PMECC_ECC[i])

/*----------------------------------------------------------------------------
 *        Exported functions
 *----------------------------------------------------------------------------*/

extern void smc_nand_configure(uint8_t bus_width);

extern void smc_nor_configure(uint8_t cs, uint8_t bus_width);

extern void smc_pmecc_wait_ready(void);

#endif /* SMC_H_ */
