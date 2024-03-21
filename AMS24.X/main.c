/**
  Generated main.c file from MPLAB Code Configurator

  @Company
    Microchip Technology Inc.

  @File Name
    main.c

  @Summary
    This is the generated main.c using PIC24 / dsPIC33 / PIC32MM MCUs.

  @Description
    This source file provides main entry point for system initialization and application code development.
    Generation Information :
        Product Revision  :  PIC24 / dsPIC33 / PIC32MM MCUs - 1.170.0
        Device            :  dsPIC33EP128GS804
    The generated drivers are tested against the following:
        Compiler          :  XC16 v1.61
        MPLAB 	          :  MPLAB X v5.45
*/

/*
    (c) 2020 Microchip Technology Inc. and its subsidiaries. You may use this
    software and any derivatives exclusively with Microchip products.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
    WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
    PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION
    WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
    BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
    FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
    ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
    THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.

    MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE
    TERMS.
*/

/**
  Section: Included Files
*/
#include "mcc_generated_files/system.h"
#include <stdint.h>
#include "mcc_generated_files/pin_manager.h"
#include "can_driver.h"
#include "soc_fns.h"
#include "mcc_generated_files/spi1.h"
#include "LTC/LTC_driver.h"
#include "LTC/LTC_utilities.h"
#include "fault_handler.h"
#include "cell_balancing.h"
#include "global_constants.h"
#include "eeprom.h"

//TODO why are these globals?
uint16_t cell_voltages[NUM_CELLS];
uint16_t pack_temperatures[NUM_TEMP_SENSORS];
uint32_t sense_line_status[NUM_ICS];

/*
                         Main application
 */
int main(void)
{
    // initialize the device
    SYSTEM_Initialize();
    CS_6820_SetHigh();
    
    uint8_t i = 0;
    for(i = 0; i < NUM_CELLS; ++i)
    {
        cell_voltages[i] = 0;
    }
    for(i = 0; i < NUM_TEMP_SENSORS; ++i)
    {
        pack_temperatures[i] = 0;
    }
    for(i = 0; i < NUM_ICS; ++i)
    {
        sense_line_status[i] = 0;
    }

    MSTR_SetHigh();

    eeprom_initialize(); // TODO call this in soc init?
    soc_initialize();
    can_initialize();
    LTC_initialize();
    fault_handler_initialize();
    balance_timer_initialize();
    
    while (1)
    {
        ClrWdt();
        LED1_HEARTBEAT_Toggle();
        // WARN: don't put all the CAN output back to back to back here, 
        //       the transmit buffers will overflow
        calc_soc();
        
        read_cell_voltages(cell_voltages);
        report_cell_voltages(cell_voltages);
        
        update_cell_balance_array(cell_voltages);
        uint32_t* cell_needs_balanced = get_cell_balance_array();
        update_config_A_and_B(); // sends cell balance bits to 6813s
        report_balancing(cell_needs_balanced);

        read_temperatures(pack_temperatures);
        report_pack_temperatures(pack_temperatures);
        
        open_sense_line_check(sense_line_status);
        report_sense_line_status(sense_line_status);

        self_test();
        
        check_for_fault();
        uint16_t pack_voltage = 0;
        uint8_t i = 0;
        for(i = 0; i < NUM_CELLS; ++i)
        {
            pack_voltage += (cell_voltages[i] / 1000);
        }
        uint16_t low_div_output = 0xFFFF;
        for(i = 0; i < NUM_TEMP_SENSORS; ++i)
        {
            if(pack_temperatures[i] < low_div_output)
            {
                low_div_output = pack_temperatures[i];
            }
        }
        uint8_t high_temp = (uint8_t)((-0.0021933) * low_div_output + 81.297);
        report_status(pack_voltage / 10, high_temp);
    }
    return 1; 
}
