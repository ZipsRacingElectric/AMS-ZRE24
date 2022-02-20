/*
 * contains functions for counting faults and reporting them
 */
////////////////includes///////////////////////////////////////////////////////
#include "fault_handler.h"
#include "LTC/LTC_utilities.h"
#include "mcc_generated_files/pin_manager.h"
#include <stdint.h>
////////////////defines////////////////////////////////////////////////////////
#define CELL_VOLTAGE_MAX_FAULTS     20 //TODO make this 10 (50 ms measurement period, 500 ms fault period)
#define OPEN_SENSE_LINE_MAX_FAULTS  10
#define TEMP_FAULTS_MAX             10
#define SELF_TEST_FAULTS_MAX        10

////////////////globals////////////////////////////////////////////////////////
uint8_t cell_voltage_faults[NUM_CELLS];
uint8_t temp_faults[9*NUM_ICS];
uint8_t sense_line_faults[NUM_CELLS];
uint8_t self_test_faults[NUM_ICS];

uint8_t fault_codes = 0;

//////////////// prototypes ///////////////////////////////////////////////////
static void set_voltage_fault_bit(void);
static void set_temperature_fault_bit(void);
static void shutdown_car(void);

//////////////// public functions /////////////////////////////////////////////

// initialize fault tracking arrays, enable BMS shutdown loop relay
void fault_handler_initialize(void)
{
    BMS_RELAY_EN_SetLow();
    
    uint8_t i = 0;
    
    for(i = 0; i < NUM_CELLS; ++i)
    {
        cell_voltage_faults[i] = 0;
        sense_line_faults[i] = 0;
    }
    
    for(i = 0; i < NUM_TEMP_SENSORS; ++i)
    {
        temp_faults[i] = 0;
    }
    
    for(i = 0; i < NUM_ICS; ++i)
    {
        self_test_faults[i] = 0;
    }
    
    BMS_RELAY_EN_SetHigh();
}

uint8_t get_fault_codes(void)
{
    return fault_codes;
}

// iterate through fault arrays to determine whether actionable fault has occurred
void check_for_fault(void)
{
    uint8_t i = 0;
    
    for(i = 0; i < NUM_CELLS; ++i)
    {
        if(cell_voltage_faults[i] > CELL_VOLTAGE_MAX_FAULTS)
        {
            shutdown_car();
            set_voltage_fault_bit();
        }
   
        if(sense_line_faults[i] > OPEN_SENSE_LINE_MAX_FAULTS)
        {
            shutdown_car();
            //TODO SET_SENSE_LINE_FAULT_BIT(1);
        }
    }
    
    for(i = 0; i < 9*NUM_ICS; ++i)
    {
        if(temp_faults[i] > TEMP_FAULTS_MAX)
        {
            shutdown_car();
            set_temperature_fault_bit();
        }
    }
    
    for(i = 0; i < NUM_ICS; ++i)
    {
        if(self_test_faults[i]> SELF_TEST_FAULTS_MAX)
        {
            shutdown_car();
            //TODO set_self_test_fault_bit();)
        }
    }
}

void increment_cell_voltage_fault(uint8_t cell_id)
{
    cell_voltage_faults[cell_id] += 1;
}

void reset_cell_voltage_fault(uint8_t cell_id)
{
    cell_voltage_faults[cell_id] = 0;
}

void increment_temperature_fault(uint8_t temp_sensor_id)
{
    temp_faults[temp_sensor_id] += 1;
}

void reset_temperature_fault(uint8_t temp_sensor_id)
{
    temp_faults[temp_sensor_id] = 0;
}

// TODO cell voltage fault check
// possible faults: over voltage, under voltage, missing measurement

//////////////// private functions ////////////////////////////////////////////

static void set_voltage_fault_bit(void)
{
    fault_codes |= (1 << 1); //TODO magic numbers?
}

static void set_temperature_fault_bit(void)
{
    fault_codes |= (1 << 0); //TODO magic numbers?
}

static void shutdown_car(void)
{
    BMS_RELAY_EN_SetLow();
}