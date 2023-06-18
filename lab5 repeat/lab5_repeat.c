// IMPORTANT: you must include the following line in all your C files
#include <lcom/lcf.h>

#include <lcom/lab5.h>

#include <stdint.h>
#include <stdio.h>

// Any header files included below this line should have been created by you

int main(int argc, char *argv[]) {
  // sets the language of LCF messages (can be either EN-US or PT-PT)
  lcf_set_language("EN-US");

  // enables to log function invocations that are being "wrapped" by LCF
  // [comment this out if you don't want/need it]
  lcf_trace_calls("/home/lcom/labs/lab5/trace.txt");

  // enables to save the output of printf function calls on a file
  // [comment this out if you don't want/need it]
  lcf_log_output("/home/lcom/labs/lab5/output.txt");

  // handles control over to LCF
  // [LCF handles command line arguments and invokes the right function]
  if (lcf_start(argc, argv))
    return 1;

  // LCF clean up tasks
  // [must be the last statement before return]
  lcf_cleanup();

  return 0;
}


//Video Card Configuration - change from text to graph mode
int(video_test_init)(uint16_t mode, uint8_t delay) {
  
    //set up graphic mode
    if(vbe_set_mode(mode) !=0){
        return 1;
    }

    sleep(delay);

    //LCF function to exit graphic mode e voltar a text mode;
    if (vg_exit() != 0) return 1;

    return 0;
}

//Draw a rectangle on the screen in the desired mode
int(video_test_rectangle)(uint16_t mode, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color) {
  
    /* para desenhar um rectangulo no ecra, precisamos de escrever na memoria de VRAM,
    em concreto queremos alocar memoria virtual a um frame buffer onde iremos "desenhar";
    Para o efeito, precisamos de mapear a memoria fisica da vram a uma memoria virtual
    especifica do processo, que chamamos frame buffer - framebuffer, is a temporary storage area in computer graphics that holds the complete pixel data for an image or frame being rendered or displayed on a screen. */
    /*
    1. map the video memory to the process' address space
    2. change the video mode to that in its argument
    3. draw a rectangle
    4. reset the video mode to Minix's default text mode and return, upon receiving the break code of the ESC key (0x81) */

    if(set_frame_buffer(mode) !=0){
        return 1;
    }
    if(vbe_set_mode(mode) !=0){
        return 1;
    }
    if(vg_draw_rectangle(x, y, width, height, color) !=0){
        return 1;
    }

    //(...)that esc key thing
    //sair do modo grafico
    if(vg_exit() !=0){
        return 1;
    }

}


int(video_test_pattern)(uint16_t mode, uint8_t no_rectangles, uint32_t first, uint8_t step) {
  /* To be completed */
  printf("%s(0x%03x, %u, 0x%08x, %d): under construction\n", __func__,
         mode, no_rectangles, first, step);

  return 1;
}

