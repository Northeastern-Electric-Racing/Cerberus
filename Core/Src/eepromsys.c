#include "eepromsys.h"
#include "m24c32.h"


void eepromInit(){


   //Need to setup a Telemetry Section in Memory
   //Establish size for 10 values of size 32 bytes
   eeprom_data[0].key = (char*)("DATA");
  
   eeprom_data[0].size = 1 + (32 * NUM_EEPROM_TELEM);


   //Need to setup a Faults Section in Memory
   //Establish Size for 5 values of size 4 bytes
   eeprom_data[1].key = (char*)("FAULTS");
   eeprom_data[1].size = 1 + (4 * NUM_EEPROM_FAULTS);




   //Initialize Address of the Data which will cover memory addresses starting at 0
   eeprom_data[0].address = EEPROM_BASE_ADD;


   int offset = 0;
   int i = 1;
   //While Loop Initializes the Sections After the Initial Section at Index 0
   while(eeprom_data[i].key != NULL){
       offset += eeprom_data[i-1].size;
       eeprom_data[i].address = offset;
       i++;
   }


}


//Use by Writes/Reads with the EEPROM key
uint16_t eeprom_get_index(char* key){
   int i = 0;
   while(eeprom_data[i].key != NULL){
       if(eeprom_data[i].key == key){
           return eeprom_data[i].address;
       }
       i++;
   }
   return -1;
}


bool eeprom_write_key(char *key, void *data, uint16_t size){
   if (!data) {
       return false;
   }


   int address = eeprom_get_index(key);
   eeprom_write(address, data, size);
   return true;
}


bool eeprom_read_key(char *key, void *data, uint16_t size){


   if (!data) {
       return false;
   }


   int address = eeprom_get_index(key);
   eeprom_read(address, data, size);
   return true;
}



//Using the addresses to get
//Calls the driver function to read from EEPROM
bool eeprom_read_data_address(uint16_t address, void *data, uint16_t size)
{
   if (!data) {
       return false;
   }
   /* read data from eeprom given index */
   eeprom_read(address, data, size);
   return true;
}


//Calls the driver function to write to EEPROM
bool eeprom_write_data_address(uint16_t address, void *data, uint16_t size)
{
   if (!data) {
       return false;
   }
   /* write data to eeprom given page, offset, and size of data */
   eeprom_write(address, data, size);
   return true;
}


void write_fault(uint32_t fault_code){
   //THIS CODE ASSUMES THE FAULT IS 4 BYTES -> AN INTEGER


   //Copy fault into new value
   uint32_t fault = fault_code;

   uint8_t reg_to_write;


   //Get's the data address to be written to next (8 bit number, up to 128)
   eeprom_read_data_address(eeprom_get_index((char *)("FAULTS")), &reg_to_write, 1);


   //This will get the initial address the EEPROM "FAULTS" began with
   uint8_t startInd = eeprom_data[eeprom_get_index((char *)("FAULTS"))].address;


   //This will get the size of the EEPROM "FAULTS" section to determine where to put the new fault
   uint8_t size = eeprom_data[eeprom_get_index((char*)("FAULTS"))].size;


   uint8_t available_space = (startInd + size) - reg_to_write;
   if(available_space < 4){
       //If there's only 3 bytes available for the data
       reg_to_write = startInd + 1;
   }
   else{
       //Else increment it to the next open place in memory
       reg_to_write += 4;
   }


   eeprom_write_data_address(reg_to_write, &fault, 4);
}


void read_faults(){


   //Iterating with the current register
   uint8_t curr_reg;


   //This will get the initial address the EEPROM "FAULTS" began with
   uint8_t startAdd = eeprom_data[eeprom_get_index((char *)("FAULTS"))].address;


   //This will get the size of the EEPROM "FAULTS" section to determine where to put the new fault
   uint8_t size = eeprom_data[eeprom_get_index((char*)("FAULTS"))].size;


   int numFaults = 0;
   while(numFaults < NUM_EEPROM_FAULTS){
       eeprom_read_data_address(curr_reg, &eeprom_faults[numFaults], 4);
       numFaults++;


       if(curr_reg == size + startAdd - 3){
           curr_reg = startAdd + 1;
       }
       else{
           curr_reg += 4;
       }
   }
  
}


void write_data(uint32_t data_point){
  
  


   //Copy fault into new value
   uint32_t data = data_point;


   uint8_t reg_to_write;


   //Get's the data address to be written to next (8 bit number, up to 128)
   eeprom_read_data_address(eeprom_get_index((char *)("DATA")), &reg_to_write, 1);


   //This will get the initial address the EEPROM "FAULTS" began with
   uint8_t startInd = eeprom_data[eeprom_get_index((char *)("DATA"))].address;


   //This will get the size of the EEPROM "FAULTS" section to determine where to put the new fault
   uint8_t size = eeprom_data[eeprom_get_index((char*)("DATA"))].size;


   uint8_t available_space = (startInd + size) - reg_to_write;
   if(available_space < 32){
       //If there's only 3 bytes available for the data
       reg_to_write = startInd + 1;
   }
   else{
       //Else increment it to the next open place in memory
       reg_to_write += 32;
   }


   eeprom_write_data_address(reg_to_write, &data, 32);
}


void read_data(){
  
   //Iterating with the current register
   uint8_t curr_reg;


   //This will get the initial address the EEPROM "FAULTS" began with
   uint8_t startAdd = eeprom_data[eeprom_get_index((char *)("DATA"))].address;


   //This will get the size of the EEPROM "FAULTS" section to determine where to put the new fault
   uint8_t size = eeprom_data[eeprom_get_index((char*)("DATA"))].size;


   int numPts = 0;
   while(numPts < NUM_EEPROM_TELEM){
       eeprom_read_data_address(curr_reg, &eeprom_pts[numPts], 32);
       numPts++;


       if(curr_reg + 32 >= size + startAdd){
           curr_reg = startAdd + 1;
       }
       else{
           curr_reg += 32;
       }
   }
}
