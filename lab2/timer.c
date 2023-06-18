#include <lcom/lcf.h>
#include <lcom/timer.h>
#include <stdint.h>
#include "i8254.h"

int hook_id = 0;
int counter = 0;

//3. change the frequency of any timer to generate interrupts at a given rate.
int (timer_set_frequency)(uint8_t timer, uint32_t freq) {
/*Parameters: the timer to be modified and the desired frequency. (slide18 - 2.timer.pdf) */
  
//Check if the desired frequency is valid and within the range of TIMER_FREQ_MIN(19) and TIMER_FREQ  
  if (freq > TIMER_FREQ || freq < 19) {
    printf ("Invalid frequency\n");
    return 1;
  }
/* Porque é que o limite é 19? PQ o valor do contador pode ser, no máximo, 2^16 = 65536
TIMER_FREQ para o minix é sempre 1193182, fazendo as contas ao contrário temos que freq = 1193182 / 65536 = 18.2
O Minix não suporta frequências inferiores a 19 pois o contador a partir de um certo ponto dá overflow */
  
//Read the current configuration of the timer before changing it
  uint8_t status;
  if (timer_get_conf(timer, &status) != 0) { //reads the config/status of timer
    printf ("Error reading timer_get_conf\n");
    return 1;
  }

//Calculate the value of the initial count (div) needed to generate the desired frequency
// q ira ser colocado no timer via "envio" do LSB seguido MSB, pq div sao 16 bits e o comando de output no timer são 8 bits
  uint16_t div = TIMER_FREQ / freq; //div: valor inicial de contagem (determina tempo q timer levará p/ gerar uma interrupção)

//Break the divisor (initial count- div) into its MSB & LSB -> usar util_get_LSB & util_get_MSB
  uint8_t msb, lsb;
  if (util_get_MSB(div, &msb) != 0){
    printf ("Could not obtain initial count MSB \n");
    return 1;
  }
  if (util_get_LSB(div, &lsb) != 0){
    printf ("Could not obtain initial count LSB\n");
    return 1;
  }

//Prepare new control word: select timer(bits 7 & 6 da control word) | select counter initializ 'LSB followed by MSB' | preserve 4 LSB of current status 
  uint8_t cw = (timer << 6) | TIMER_LSB_MSB | (status & 15); //15=1111(em bin) - preservar os 4 LSB de status actual
  
//Write the new control word to the timer's control register (0x43)
  if(sys_outb(TIMER_CTRL, cw)) {
    printf("Error in sys_outb\n");
    return 1;   
  } 
  
//Write the LSB of the initial count needed for the freq we want, to timers (0,1 or 2) register port (0x40, 0x41 or 0x42)
  if(sys_outb(TIMER_0 + timer, lsb) != 0) {
    printf("Error in sys_outb\n");
    return 1;
  }

//Use sys_outb to write the MSB of the initial count to the timer's register
  if(sys_outb(TIMER_0 + timer, msb) != 0) {
    printf("Error in sys_outb\n");
    return 1;
  }

}

//4. 
int (timer_subscribe_int)(uint8_t *bit_no) {
/*
Call the sys_irqsetpolicy(int irq_line, int policy, int *hook_id) kernel call;
- irq_line is the IRQ line of the device; (IRQ: interrupt request)
- policy: IRQ_REENABLE to inform the GIH that it can give the EOI command to PIC, enabling further interrupts on the corresponding IRQ line;
- hook_id is both: > input: an id to be used by the kernel on interrupt notification & output: an id to be used by the DD in other kernel calls on
  this interrupt;
- must return via its input argument (*bit_no), the value that it has passed to the kernel in the third argument of sys_irqsetpolicy()-> &hook_id;
- nota: sys_irqsetpolicy() already enables the corresponding interrupt, so you do NOT need to call sys_irqenable();
*/
  if(bit_no == NULL) return 1; // garantir que o val do apontador é valido
  *bit_no = BIT(hook_id);     // o bit de interrupção é definido pelo módulo que gere o próprio dispositivo
  //sys_irqsetpolicy() can be viewed as an interrupt notification subscription
  if (sys_irqsetpolicy(TIMER0_IRQ, IRQ_REENABLE, &hook_id) != 0) return 1; // subscrição das interrupções
  return 0;

/* 
-primeiro argumento descreve a IRQ_LINE a usar, o seu valor é sempre dado no enunciado e pode variar entre 0 e 15 (slide 5 'I/O+Interrupts');
-segundo argumento é, para o caso de i8254, IRQ_REENABLE, uma política que permite recebermos sinais do tipo EOI (End Of Interrupt);
-terceiro argumento é um apontador para um inteiro à escolha do aluno, que pode variar entre 0 e 31. Entre dispositivos diferentes
estes valores têm de ser também diferentes. Este valor deve ser declarado como inteiro e global no ficheiro da implementação do módulo;

 -> o terceiro argumento da system call (&hook_id) funciona de duas formas:
Input: indica ao hardware o bit a ativar na IRQ_LINE assim que ocorrer uma interrupção;
Output: recebe um identificador para ser usado ao desativar as interrupções;
Assim é expectável que depois de invocarmos a system call o valor da variável timer_hook_id já não seja 0 e sim algo aleatório.
*/
}

//4.1
int (timer_unsubscribe_int)() {
/*
need to call sys_irqrmpolicy(int *hook_id) (remove policy)
-> Unsubscribes a previous interrupt notification, by specifying a pointer to the hook_id returned by the kernel in sys_irqsetpolicy()
*/
  if (sys_irqrmpolicy(&hook_id) != 0) return 1; // desligar as interrupções
  return 0;
}

//4.2
void (timer_int_handler)() {
  counter++;
}

//1. reads the configuration/status of a timer
int (timer_get_conf)(uint8_t timer, uint8_t *status) {
/*Sends read-back command to control register of specified timer & reads the configuration from the timers port.
If both ops are successful, function stores the configuration in the location pointed to by st and returns 0. */

/* The | (bitwise OR) operator combines the binary digits of each constant to produce the final result. The resulting RBCommand
variable contains the necessary bits to issue a read-back command to the chip to retrieve the status of the specified timer. */
  uint8_t RBCommand = TIMER_RB_CMD | TIMER_RB_COUNT_ | TIMER_RB_SEL(timer); //set up read-back command via bitwise OR (|)
/*Format of the Read-Back command:
    bit 1, 2 & 3 - (1) Select Counter 0, 1 or 2
    bit 4 - (0) Read Programmed mode (STATUS) - bits ACTIVE LOW, action occurs when bits value is 0 (instead of 1)
    bit 5 - (0) Read counter value (COUNT) - bit ACTIVE LOW
    bit 6 & 7 - (11) Read-Back Command */
  if(sys_outb(TIMER_CTRL, RBCommand)) { //escreve em Control Register port 0x43 (TIMER_CTRL) o RBCommand
/* sys_outb(port_t port, u8_t byte) performs a byte output to I/O port parameter, using 8-bit value specified by byte parameter. */
    printf("Error sending read-back command");
    return 1;
  }
  uint8_t timer_port = TIMER_0+timer;
  if(util_sys_inb(timer_port, status)){
  //util_sys_inb(int port, uint8_t *byte) lê 1 byte do I/O port e guarda na mem location pointed by byte (st)
    printf("Error reading configuration of timer's port");
    return 1;
  }
  return 0; //both functions were successful   
}

// 2. prints the specified fields of a timer's configuration/status
int (timer_display_conf)(uint8_t timer, uint8_t st, enum timer_status_field field) { 
/* print the config in friendly way invoke: timer_print_config(uint8_t timer, enum timer_status_field field, union timer_status_field_val conf)
Parse the value passed in conf, the configuration of the timer specified in argument timer, as returned by the read-back
command. The last argument specifies which field of the i8254 status word must be displayed

enum timer_status_field {  //enum: each field corresponds to a number; enum timer_status_field=0 -> tsf_all; etc
  tsf_all, // configuration in hexadecimal     tsf: timer status field
  tsf_initial, // timer initialization mode
  tsf_mode, // timer counting mode
  tsf_base // timer counting base (BCD ou binario?)
};
union timer_status_field_val { ---> union that can store any of the configuration fields.
  uint8_t byte; // dispalys the status, in hexadecimal
  enum timer_init in_mode; // dispalys the initialization mode
  uint8_t count_mode; // dispalys the counting mode: 0, 1, ..., 5
  bool bcd; // true, if counting in BCD
}; */  

union timer_status_field_val config;
/* Format of a timer's STATUS BYTE: (bits 0 to 5 contain the corresponding bits of the last control word)
Bit 0 - BCD || Bit 1, 2 & 3 - Programmed/Counting Mode || Bit 4 & 5 - Counter Initialization (Type of Access, MSB, LSB)
Bit 6 - Null Count (related to the reading of the counting value) || Bit 7 - Output (current value of the timer's OUT line)
*/
  switch (field){
    case 0: //field 0 = tsf_all: configuration in hexadecimal     
      config.byte = st;  //uint8_t byte: The status byte > Full Status Byte
      break; 

    case 1: //tsf_initial: timer initialization mode > Counter Initial Value Loading Mode (MSB, LSB, etc)
    //Extracts a certain part of the st value{initialization mode (LSB, MSB) bits (4 & 5)} w/ bitwise AND (&)
    //right-shifts the result by 4 bits and assigns the final value to the config.in_mode variable (enum type)
      config.in_mode = (st & TIMER_LSB_MSB) >> 4; //enum timer_init in_mode; result sera 0(0000),1(0001),2(0010) ou 3(0011)
/* enum timer_init{            NOTE: if enum timer_init=0 -> invalid mode; if enum = 1-> LSB; if enum=2 -> MSB etc
	  INVAL_val, 	  (0) Invalid initialization mode 
	  LSB_only,		  (1) Initialization only of the LSB 
	  MSB_only,		  (2) Initialization only of the MSB 
	  MSB_after_LSB	(3) Initialization of LSB and MSB, in this order
  }; */
      break;

    case 2: //tsf_mode: timer counting mode (bits 3,2,1 of status byte)
    /* 000 - mode 0 || 001 - mode 1 || x10 - mode 2 || x11- mode 3 || 100 - mode 4 || 101 - mode 5  */
    {
      uint8_t count_mode = (st & (BIT(1) | BIT(2) | BIT(3))) >> 1;  //ou: count_mode = (st >> 1) & 7(0111)
      if(count_mode == 6) //6=0110 (x10 - mode 2)
        config.count_mode = 2;
      else if(count_mode == 7) //7=0111 (x11- mode 3)
        config.count_mode = 3;
      else config.count_mode = count_mode; //os restantes modos o seu valor em binario = valor decimal (mode 1 = 001 bin = 1 dec)
    }
    break;

    case 3: //tsf_base: timer counting base (BCD (bit=1) ou binary (bit=0)) - bit 0 of status byte
      config.bcd = st & 1; //config.bcd é bool, ou seja ou tem val 1(true) ou val 0 (false)
      break;
    default:
      printf("Error: Status field not recognized \n");
      return 1;
  }

  if(timer_print_config(timer, field, config)) {
    printf("Could not print configuration\n");
    return 1;
  }
  return 0;
}
