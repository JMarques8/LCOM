#include <lcom/lcf.h>

#include <stdint.h>

int(util_get_LSB)(uint16_t val, uint8_t *lsb) {
  if(lsb==NULL) return 1; //verificar se o apontador é valido
  *lsb= val & 255; //255 = 1111 1111 em bin
  return 0;
}


int(util_get_MSB)(uint16_t val, uint8_t *msb) {
  if(msb==NULL) return 1; //certificar que o apontador tem valor valido
  *msb= val >> 8;
  return 0;
}


int (util_sys_inb)(int port, uint8_t *value) {
  
uint32_t temp;
sys_inb(port, &temp);
*value = (uint8_t) temp;

return 0;

/*
The util_sys_inb function takes two arguments: port, which is an integer representing the I/O port to read from, and value,
which is a pointer to an unsigned 8-bit integer. The function reads a 32-bit value from the specified port using
the sys_inb function and stores the least significant byte of the value in the location pointed to by value.
The function then returns 0.
*/

/*
used to read a single byte from the I/O port specified by port and store it in the memory location pointed to by byte
- Return value of this system call is an integer that indicates whether the operation was successful or not
*/

}
