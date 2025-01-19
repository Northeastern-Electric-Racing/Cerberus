#ifndef EEPROMSYS_H
#define EEPROMSYS_H

#include <stdint.h>
#include <stdbool.h>

// EEPROM's Root Address
#define EEPROM_BASE_ADD 0
// Number of Faults EEPROM will Store
#define NUM_EEPROM_FAULTS 5
// Number of Telemetry points to store
#define NUM_EEPROM_TELEM 10
// Number of Sections (2)
#define NUM_EEPROM_SECS 2

/**
 * @brief EEPROM
 *
 */
struct eeprom {
  char *key;
  uint16_t size;
  uint16_t address;
};


/**
 * @brief This struct is used to organize the two types of data for the EEPROM to store
 *
 */
struct eeprom eeprom_data[NUM_EEPROM_SECS];

/**
 * @brief This array will collect all the faults when read_faults() is called
 *
 */
static uint32_t eeprom_faults[NUM_EEPROM_FAULTS];

/**
 * @brief This array will collect all the data when read_data() is called
 *
 */
static uint32_t eeprom_pts[NUM_EEPROM_TELEM];

/**
 * @brief Partitions eeprom addresses given table of data and size
 *
 */
void eepromInit();

/**
 * @brief Fetches the latest memory address saved to the first byte of the eeprom partition
 *
 * @param key Name of the eeprom partition
 * @return uint16_t The memory address to update and write to next
 */
uint16_t eeprom_get_index(char *key);

/**
 * @brief This function writes the data to the newest memory address of one of the EEPROM partitions
 *
 * @param key The EEPROM partition to write to
 * @param data Undetermined datatype
 * @param size Expected size of data
 * @return true If HAL writes properly
 * @return false If HAL fails to write
 */
bool eeprom_write_key(char *key, void *data, uint16_t size);

/**
 * @brief This function reads the data at the most recently used memory address of 
 * a specific EEPROM partition
 *
 * @param key Name of the EEPROM partition
 * @param data Data of undetermined type
 * @param size Expected size of data
 * @return true If this reads properly using HAL
 * @return false If this fails to read with HAL
 */
bool eeprom_read_key(char *key, void *data, uint16_t size);

/**
 * @brief Function to read data from EEPROM based on the latest address stored in the partition
 *
 * @param id The address associated with the section of EEPROM memory
 * @param data Variable to store collected data
 * @param size Size of expected data
 * @return true Successful
 * @return false Unsuccessful
 */
bool eeprom_read_data_address(uint16_t address, void *data, uint16_t size);

/**
 * @brief Function to write data to EEPROM with updated address
 *
 * @param address The next available address in the memory based on its size
 * @param data Data being written
 * @param size Size of data write
 * @return true Successful
 * @return false Unsuccessful
 */
bool eeprom_write_data_address(uint16_t address, void *data, uint16_t size);

/**
 * @brief Accepts a fault, updates the register saved in the eeprom partition, and 
 * writes a new fault to the EEPROM
 *
 * @param fault_code The code to be written to the EEPROM fault partition
 */
void write_fault(uint32_t fault_code);

/**
 * @brief Iterates through the memory in the EEPROM based on its initial index and 
 * stores the extracted faults in an array
 *
 */
void read_faults();

/**
 * @brief Accepts a data point, updates the register saved in the eeprom partition, and 
 * writes a new data point to the EEPROM
 *
 * @param data_point The code to be written to the EEPROM fault partition
 */
void write_data(uint32_t data_point);

/**
 * @brief Iterates through the memory in the EEPROM based on its initial index and 
 * stores the extracted data in an array
 *
 */
void read_data();

#endif // EEPROMSYS_H
