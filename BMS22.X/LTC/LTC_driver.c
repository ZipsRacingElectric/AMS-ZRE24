
/* 
 * File:   LTC_driver.c
 * Author: Elena
 *
 * Created on January 28, 2022, 5:35 AM
 */

#include "LTC_driver.h"
#include "LTC_utilities.h"
#include "LTC_cmds/LTC_cmds.h"
#include "../fault_handler.h"
#include <stdint.h>
#define FCY 40000000UL // Instruction cycle frequency, Hz - required for __delayXXX() to work
#include <libpic30.h>        // __delayXXX() functions macros defined here

static uint8_t cell_voltage_check(uint16_t* cell_voltages);
static uint8_t pack_temperature_check(uint16_t* pack_temperatures);

// initialize PEC table necessary for LTC68xx interface
void LTC_initialize()
{
    init_PEC15_Table();
}

// send commands to get cell voltages
uint8_t read_cell_voltages(uint16_t* cell_voltages)
{
    start_cell_voltage_adc_conversion();
    poll_adc_status();
    __delay_ms(10); //TODO: is this delay necessary?
    rdcv_register(ADCVA, &cell_voltages[0]);
    rdcv_register(ADCVB, &cell_voltages[3]);
    rdcv_register(ADCVC, &cell_voltages[6]);
    rdcv_register(ADCVD, &cell_voltages[9]);
    rdcv_register(ADCVE, &cell_voltages[12]);
    rdcv_register(ADCVF, &cell_voltages[15]);
    
    return cell_voltage_check(cell_voltages);
}

// send commands to get pack temperatures
uint8_t read_temperatures(uint16_t* pack_temperatures)
{
    start_temperature_adc_conversion();
    poll_adc_status();
    __delay_ms(10); //TODO: is this delay necessary?
    rdaux_register(AUXA, &pack_temperatures[0]);
    rdaux_register(AUXB, &pack_temperatures[3]);
    rdaux_register(AUXC, &pack_temperatures[6]);
    rdaux_register(AUXD, &pack_temperatures[9]);
    
    return pack_temperature_check(pack_temperatures);
}

// check whether sense line overcurrent protection has tripped
uint8_t open_sense_line_check(void)
{
    //see pg 32 of 6813 datasheet for info
    open_wire_check(1); // param: pull dir 0 for down 1 for up
    open_wire_check(1); // param: pull dir 0 for down 1 for up
    uint16_t cell_pu[8*NUM_ICS];
    rdcv_register(ADCVA, &cell_pu[0]);
    rdcv_register(ADCVB, &cell_pu[3]);
    rdcv_register(ADCVC, &cell_pu[6]);
    rdcv_register(ADCVD, &cell_pu[9]);
    rdcv_register(ADCVE, &cell_pu[12]);
    rdcv_register(ADCVF, &cell_pu[15]);
    open_wire_check(0); // param: pull dir 0 for down 1 for up
    open_wire_check(0); // param: pull dir 0 for down 1 for up
    uint16_t cell_pd[8*NUM_ICS];
    rdcv_register(ADCVA, &cell_pd[0]);
    rdcv_register(ADCVB, &cell_pd[3]);
    rdcv_register(ADCVC, &cell_pd[6]);
    rdcv_register(ADCVD, &cell_pd[9]);
    rdcv_register(ADCVE, &cell_pd[12]);
    rdcv_register(ADCVF, &cell_pd[15]);
    
    //TODO: finish this
//    uint8_t i = 0;
//    for(i = 0; i < NUM_CELLS; ++i) // for each ic - 0-35
//    {
//        if()
//    }
//    // edge cases - C0 and C18
//    if()
    
}

static uint8_t cell_voltage_check(uint16_t* cell_voltages) //TODO: implement timeout, or consecutive count of out-of-range samples
{
    uint8_t i = 0;
    uint8_t ret_val = SUCCESS;
    for(i = 0; i < NUM_CELLS; ++i)
    {
        if((cell_voltages[i] > CELL_VOLTAGE_MAX) | (cell_voltages[i] < CELL_VOLTAGE_MIN))
        {
            increment_cell_voltage_fault(i);
            ret_val = FAILURE;
        }
        else
        {
            reset_cell_voltage_fault(i);
        }
    }
    return ret_val;
}

static uint8_t pack_temperature_check(uint16_t* pack_temperatures)
{
    uint8_t i = 0;
    uint8_t ret_val = SUCCESS;
    for(i = 0; i < NUM_TEMP_SENSORS; ++i)
    {
        if((pack_temperatures[i] > CELL_TEMPERATURE_MAX) | (pack_temperatures[i] < CELL_TEMPERATURE_MIN))
        {
            increment_temperature_fault(i);
            ret_val = FAILURE;
        }
        else
        {
            reset_temperature_fault(i);
        }
    }
    return ret_val;
}