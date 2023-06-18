#include <lcom/lcf.h>
#include <lcom/lab2.h>

#include <stdbool.h>
#include <stdint.h>

extern int counter;

int main(int argc, char *argv[]) {
  // sets the language of LCF messages (can be either EN-US or PT-PT)
  lcf_set_language("EN-US");

  // enables to log function invocations that are being "wrapped" by LCF
  // [comment this out if you don't want/need it]
  lcf_trace_calls("/home/lcom/labs/lab2/trace.txt");

  // enables to save the output of printf function calls on a file
  // [comment this out if you don't want/need it]
  lcf_log_output("/home/lcom/labs/lab2/output.txt");

  // handles control over to LCF
  // [LCF handles command line arguments and invokes the right function]
  if (lcf_start(argc, argv))
    return 1;

  // LCF clean up tasks
  // [must be the last statement before return]
  lcf_cleanup();

  return 0;
}

//purpose: read and to display the specified field of status/configuration of a timer.
int(timer_test_read_config)(uint8_t timer, enum timer_status_field field) {
/* Variable status is declared at the beginning because it is later used in function timer_display_conf, which takes it as input.
timer_get_conf updates the value of status using a pointer to the status variable, which is passed to it as input parameter.
Declaring status at the beginning ensures that it's in scope for both timer_get_conf & timer_display_conf, allowing these functions
to work correctly together */
  uint8_t status;
  //call function that reads the configuration/status of a specified timer
  if(timer_get_conf(timer, &status)) {  //if timer_get_conf yields 1 (true) 
    printf("Could not retrieve timer's configuration\n");
    return 1;
  }
  //if config is successfull, call function to display the specified fields of a timer's configuration/status
  if(timer_display_conf(timer, status, field)) { //if timer_display_conf 1 (true) 
    printf("Could not display timer's configuration \n");
    return 1;
   }

  return 0;
/*
1. Write read-back command to read input timer configuration:
  ▶ Make sure 2 MSBs are both 1 ▶ Select only the status (not the counting value)
2. Read the timer port
3. Parse the configuration read
4. Call the function timer_print_config() that we provide you */
}

//purpose: configure the specified timer (0, 1 or 2) to generate a time base with a given frequency in Hz.
int(timer_test_time_base)(uint8_t timer, uint32_t freq) {
/* What to do? Change the rate at which a timer 0 generates interrupts -> Write control word to configure Timer 0:
 How do we know it works? Use the date command. */
  	if(timer_set_frequency (timer, freq)) {
      printf("Could not change the frequency of timer \n");
      return 1;
    }
    return 0;
}

//purpose: print one message per second, for a time interval whose duration is specified in its argument
int(timer_test_int)(uint8_t time) {
/* 1. Subscribe i8254 Timer 0 interrupts || 2. Print message at 1 second intervals, by calling the LCF function: void timer_print_elapsed_time()
   3. Unsubscribe Timer 0 at the end
How to design it?
-> Implement int timer_subscribe_int(): it returns, via its argumens, the bit number that will be set in msg.m_notify.interrupts upon a TIMER 0 interrupt
-> Implement the interrupt handler || -> Implement the “interrupt loop” in timer_test_int()
How does the DD receive the notification of the GIH? use the IPC mechanism (standard interprocess communication) used to communicate:
    - between processes; - between the (micro) kernel and a process  */

  //The function begins by declaring some variables for use in the interrupt loop:
  int ipc_status, r; //ipc_status (ipc: standard interprocess communication) and r (receive) are used to receive messages from the kernel
  message msg; //msg is a message structure used to receive notifications of hardware interrupts
  uint8_t irq_set; //irq_set is used to store the bit number of the interrupt associated with the timer subscription

  //subscribe timer interrupts
  if(timer_subscribe_int(&irq_set) != 0) return 1;
  /*- Implement int timer_subscribe_int() to hide from other code i8254 related details, such as the IRQ line used
  It returns the bit number associated with the timer interrupt (&irq_set), which is used later in the interrupt handling code.
  (will be set in msg.m_notify.interrupts upon a TIMER 0 interrupt) */

  //The “interrupt loop” (slide 13 'I/O + Interrupts')
  while(time > 0) { /* time é o input da função: time interval whose duration is specified   */
    /* Get a request message. */
    if ( (r = driver_receive(ANY, &msg, &ipc_status)) != 0 ) {
    /*driv_receiv é uma função q permite receber mensagens, quer de outros processos quer do kernell, incluindo notificações
      - 1st argument: é o end point, i.e, a origem das mensagens pretendidas, usamos um valor especial que é o ANY q significa que estamos
      interessados em todas as mensagens independentemente da sua origem;
      - 2nd argument: é o endereço de 1a variaveldo tipo message, este arg é inicializado por driver_receive c/ a mensagem recebida
      - 3rd argument: endereço da variavel ipc_status e usado para reportar se a operação de recepção foi bem sucedida e nesse caso o tipo
      de mensagem recebida */
      printf("driver_receive failed with: %d", r);
      continue;
    }
    if (is_ipc_notify(ipc_status)) { // received notification
    /* testa se a mensagem recebida é uma notificação (outro tipo de mensagens devem ser ignoradas) */
      switch (_ENDPOINT_P(msg.m_source)) {
      /*só estamos interessados nas mensagens c/ origem no HW, caso sejam, executamos o codigo escrito em seguida */
        case HARDWARE: // hardware interrupt notification
          if (msg.m_notify.interrupts & irq_set) { // subscribed interrupt
          /*este if procura identificar quais foram as interrup q ocorreram */
            timer_int_handler(); 
              if(counter%60==0){
                timer_print_elapsed_time();
                  time--;
              }
          }
          break;
        default:
          break; /* no other notifications expected: do nothing */    
        }
     } else { /* received a standard message, not a notification */
         /* no standard messages expected: do nothing */
     }
  }

  if (timer_unsubscribe_int() != 0) return 1;
  return 0;

}
