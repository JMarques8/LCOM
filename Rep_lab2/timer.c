#include <lcom/lcf.h>
#include <lcom/timer.h>

#include <stdint.h>

#include "i8254.h"

int hook_id=0;
int counter =0;


//3. change the frequency of any timer to generate interrupts at a given rate.
int (timer_set_frequency)(uint8_t timer, uint32_t freq) {
  // Write control word to configure Timer:  Preferably, LSB followed by MSB
  // do not change the 4 least significant bits (i.e. the counting mode and BCD vs. binary couting) of the control world
  // read the input timer configuration before you change it
 
  if (freq > TIMER_FREQ || freq < 19) {
    printf ("Invalid frequency\n");
    return 1;
  }
  
  //1. obter config/status do Timer via timer_get_conf
  unint8_t st;
  if(timer_get_conf(timer, &st)){
    printf("error reading config in timer set freq function\n");
    return 1;
  }

  //2. build control word: sem alterar os 4 LSB: (timer | LSB_follwed_MSB | unchanged 4 LSB)
  unint8_t cw = (timer<<6) | TIMER_LSB_MSB | (st & 15);
  //3. escrever new control word no reg de controlo
  if(sys_outb(TIMER_CTRL, cw)){ //performs a byte output to I/O port (0x43), using 8-bit value specified (cw).
    printf("Could not write control word in control register\n");
    return 1;
  }

  //4. obter valor do initial count q é necessario para gerar a freq desejada
  unint16_t count = TIMER_FREQ/freq;
  // count ira ser colocado no timer via "envio" do LSB seguido MSB, pq div sao 16 bits e o comando de output no timer são 8 bits
  unint8_t MSB, LSB;  
  if(util_get_LSB(count, &LSB)){
    return 1;
  }
  if(util_get_MSB(count, &MSB)){
    return 1;
  }

  //5. carregar o valor inicial de contagem(div) no registo do Timer (via envio de lsb 1o, seguido de msb)
  if(sys_outb(TIMER_0+timer, lsb)){
    return 1;
  }
  if(sys_outb(TIMER_0+timer, msb)){
    return 1;
  }

} 

int (timer_subscribe_int)(uint8_t *bit_no) {
  /*
  Call the sys_irqsetpolicy(int irq_line, int policy, int *hook_id) kernel call;
- irq_line is the IRQ line of the device; (IRQ: interrupt request)
- policy: IRQ_REENABLE to inform the GIH that it can give the EOI command to PIC, enabling further interrupts on the corresponding IRQ line;
- hook_id is both: > input: an id to be used by the kernel on interrupt notification & output: an id to be used by the DD in other kernel calls on
  this interrupt;
  */
    *bit_no = BIT(hook_id);
    //sys_irqsetpolicy():interrupt notification subscription
    if(sys_irqsetpolicy(TIMER0_IRQ, IRQ_REENABLE, &hook_id)) return 1;
    return 0;

}

int (timer_unsubscribe_int)() {
  /*Unsubscribes a previous interrupt notification, by specifying a pointer to the hook_id returned by the kernel in sys_irqsetpolicy() */
  if(sys_irqrmpolicy(&hook_id)) return 1;
  return 0;
}

void (timer_int_handler)() {
  counter++;
}


//1. read the configuration of a time
int (timer_get_conf)(uint8_t timer, uint8_t *st) {
//ReadBack command: allows to read the configuration of a timer
//the Read-Back command is also written to the control register
//after writing the appropriate Read-Back command to the control register, the configuration can be obtained by reading from that timer

  //1. fazer um Read Back command para obter a config/status do timer: 
  uint8_t rb = TIMER_RB_CMD | TIMER_RB_COUNT_ | TIMER_RB_SEL(timer);

  //2. escrever o RB command no control register (0x43)
  if(sys_outb(TIMER_CTRL, rb) != 0){ //performs a byte output to I/O port (0x43), using 8-bit value specified (rb).
    printf("Could not write to control register the RB command");
    return 1;
  }

  //3. ler a config/staus do timer
  int timer_port = TIMER_0 + timer;
  if(util_sys_inb(timer_port, st)){ //lê 1 byte do I/O port e guarda na mem location pointed by byte (st)
    printf("Could not read from timer port");
    return 1;
  }
  return 0;
}

//2. prints the specified fields of a timer's configuration/status
int (timer_display_conf)(uint8_t timer, uint8_t st, enum timer_status_field field) {
  /*should parse the value passed in its second argument, conf, which is assumed to be the configuration of the timer,
  as returned by the read-back command. The third and last argument specifies which field of the i8254 status word must be displayed 
  must invoke friendly way: int timer_print_config(uint8_t timer, enum timer_status_field field, union timer_status_field_val conf)*/
  /*
  enum timer_status_field {
	  tsf_all, /* Display status byte, in hexadecimal 
	  tsf_initial, /* Display the initialization mode, only
	  tsf_mode, /* Display the counting mode, only 
	  tsf_base /* Display the counting base, only
};
  union timer_status_field_val {
	  uint8_t byte; The status byte
	  enum timer_init in_mode;  The initialization mode 
	  uint8_t count_mode; The counting mode: 0, 1,.., 5
	  bool bcd; The counting base, true if BCD
}; */
union timer_status_field_val config;
/* TIMER STATUS BYTE: (bits 0 to 5 contain the corresponding bits of the last control word)
Bit 0 - BCD || Bit 1, 2 & 3 - Programmed/Counting Mode || Bit 4 & 5 - Counter Initialization (Type of Access, MSB, LSB)
Bit 6 - Null Count (related to the reading of the counting value) || Bit 7 - Output (current value of the timer's OUT line) */
  switch(field){
    case(0): //tsf all
      config.byte = st; //friendly config will display the full status byte w/ all timer config info
      break;
    case(1): //tsf_initial -> initialization mode
      config.in_mode = (st & TIMER_LSB_MSB) >> 4; // result sera 0(0000),1(0001),2(0010) ou 3(0011)
      /*enum timer_init{            NOTE: if enum timer_init=0 -> invalid mode; if enum = 1-> LSB; if enum=2 -> MSB etc
	        INVAL_val, 	  (0) Invalid initialization mode 
	        LSB_only,		  (1) Initialization only of the LSB 
	        MSB_only,		  (2) Initialization only of the MSB 
	        MSB_after_LSB	(3) Initialization of LSB and MSB, in this order}; */
      break;
    case(2): // tsf_mode -> Display the counting mode; BITS(1,2,3) do status byte
    /* 000 - mode 0 || 001 - mode 1 || x10 - mode 2 || x11- mode 3 || 100 - mode 4 || 101 - mode 5  */
      unint8_t mode = (st & (BIT(1)|BIT(2)|BIT(3))) >> 1; //ou = (st>>1) & 7(111 em binario);
      if(mode==6){ //mode = 110 (6 em dec)
        config.count_mode = 2;}
      else if(mode==7){
        config.count_mode = 3;}
      else{
        config.count_mode = mode;}
      break;
    case(3): // tsf_base -> Display the counting base (BCD ou binario)
      config.bcd = (st&1); //bcd is a bool value (either 1 true or 0 false)
      break;
  }

  if(timer_print_config(timer, field, config)){
    printf("Unable to print timer's staus/configuration\n");
    return 1;
  }
  return 0;
}
