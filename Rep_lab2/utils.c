#include <lcom/lcf.h>

#include <stdint.h>

int(util_get_LSB)(uint16_t val, uint8_t *lsb) {
  unint8_t lsb_val = val&255;
  *lsb = lsb_val;
  return 0;
}

int(util_get_MSB)(uint16_t val, uint8_t *msb) {
  unint8_t msb_val = val >>8;
  *msb = msb_val;
  return 0;
}

int (util_sys_inb)(int port, uint8_t *value) {
  
  uinte32_t aux;
  sys_inb(port, &aux);
  *value = (uint8_t) aux;

  return 0;
}
