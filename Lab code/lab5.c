// IMPORTANT: you must include the following line in all your C files
#include <lcom/lcf.h>
#include <lcom/lab5.h>

#include <stdint.h>
#include <stdio.h>
// Any header files included below this line should have been created by you
#include<windows.h> //for sleep function

extern vbe_mode_info_t mode_info;

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

/************************************************************
 Video Card Configuration - change from text to graph mode
 ************************************************************/
int(video_test_init)(uint16_t mode, uint8_t delay) {
  //Minix boots in text mode, we want in graphics mode to configure to the desired graphics mode -> Use the VESA Video Bios Extension (VBE)
  //How to make a BIOS Call in Minix? Use Minix 3 kernel call SYS_INT86
  
  if (set_vbe_mode(mode) != 0){
    printf("error setting graphics mode\n");
    return 1;
  }

  /*If the program does not introduce a time lapse, it may attempt to display text on the screen before the graphics card has finished 
  resetting, resulting in a corrupted or garbled display --> hence sleep function */
  sleep(delay);

  // vg_exit() é dada pela LCF.
  /*resets the video controller to operate in text mode. Specifically, vg_exit() calls function 0x00 (Set Video Mode) of the BIOS video services (INT 0x10)*/
  if (vg_exit() != 0) return 1;

  return 0;

}

/************************************************************
  Draw a rectangle on the screen in the desired mode
************************************************************/
int(video_test_rectangle)(uint16_t mode, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color) {
  
  /* Purpose: learn how to change the color of pixels on the screen and to use the relevant graphics mode parameters. This function shall:
    1. map the video memory to the process' address space
    2. change the video mode to that in its argument
    3. draw a rectangle
    4. reset the video mode to Minix's default text mode and return, upon receiving the break code of the ESC key (0x81)
Note that actions 1 and 2 could have been swapped. However, the LCF tests are not flexible, and require 1 to be performed before 2

The arguments x and y specify the coordinates of the rectangle's top-left corner. (The pixel at the top-left corner of the screen has coordinates (0,0).)
The arguments width and height specify the width and height of the rectangle in pixels. The color argument specifies the color to fill the rectangle, 
encoded as determined by the first argument, mode.

To facilitate testing, you should implement the function: int vg_draw_hline
Also, with an eye towards the implementation and test of the next test function, video_test_pattern, we suggest that you implement the following function:
int vg_draw_rectangle */

  /* So that your code supports different graphics modes, it should use VBE function 0x01 - Return VBE Mode Information, to get the parameters relevant
for the chosen graphics mode. You can then use these parameters to change the value of the appropriate location of the video RAM (VRAM).
For example, if the mode is 0x105, then the resolution is 1024 x 768, color mode is indexed and each pixel takes 8 bits, i.e. 1 byte; thus the argument
color must not be larger than 255. Furthermore, to change a pixel you must write this byte to the byte in VRAM corresponding to that pixel.
On the other hand, if the mode is 0x115, then the resolution is 800 x 600, the color mode is direct and each pixel takes 24 bits, 8 for each of 
the RGB components. Therefore, you should consider only the 24 least significant bits, i.e. the 3 least significant bytes, of the color
argument. Furthermore, to change a pixel you must write these 3 bytes to the 3 bytes in VRAM corresponding to that pixel
*/

  // 1. map the video memory to the process' address space -> Construção do frame buffer virtual e físico
  if (set_frame_buffer(mode) != 0) return 1;

  //2. change the video mode to that in its argument -> Mudança para o modo gráfico
  if (set_vbe_mode(mode) != 0) return 1;

  //3. draw a rectangle -> Desenha o rectângulo
  if (vg_draw_rectangle(x, y, width, height, color) != 0) return 1;

  //4. reset the video mode to Minix's default text mode and return, upon receiving the break code of the ESC key (0x81)
  if (waiting_ESC_key() != 0) return 1;
  // De regresso ao modo texto
  if (vg_exit() != 0) return 1;

  return 0;

}


//color rectangles
int(video_test_pattern)(uint16_t mode, uint8_t no_rectangles, uint32_t first, uint8_t step) {
  /*The purpose of this function is that you learn further how to use the graphics mode parameter, namely to
handle the different components of a pixel color in direct mode. This function shall change the video mode to that in its argument.
Then it should draw a pattern of colored (filled) rectangles on the screen. Finally, upon receiving the break code of the ESC key (0x81),
it should reset the video mode to Minix's default text mode and return

The pattern to be drawn is a matrix of no_rectangles by no_rectangles. All rectangles shall have the same size, i.e. the same width 
and the same height. If the horizontal (vertical) resolution is not divisible by no_rectangles then you should leave a black stripe
with minimum width (height) on the right (bottom) of the screen. The color of each rectangle depends on its coordinates in the rectangle
matrix, (row, col), and on all the arguments. If the color mode is indexed (or packed pixel, in VBE jargon), then the index of the color 
to fill the rectangle with coordinates (row, column) is given by the expression:
index(row, col) = (first + (row * no_rectangles + col) * step) % (1 << BitsPerPixel)

If color mode is direct,the RGB components of the color to fill the rectangle with coordinates (row, column) are given by:
R(row, col) = (R(first) + col * step) % (1 << RedMaskSize)
G(row, col) = (G(first) + row * step) % (1 << GreenMaskSize)
B(row, col) = (B(first) + (col + row) * step) % (1 << BlueMaskSize)
where RedMaskSize, GreenMaskSize and BlueMaskSize, are the values of the members of the VBEInfoBlock struct with the same name
*/  
  
  // Construção do frame buffer virtual e físico
  if (set_frame_buffer(mode) != 0) return 1;
  // Mudança para o modo gráfico
  if (set_vbe_mode(mode) != 0) return 1;
  // Cálculo do número inteiro de rectângulos em cada eixo
  int height = mode_info.YResolution / no_rectangles; //vertical
  int width = mode_info.XResolution / no_rectangles; // horizontal

  /* first:	Color to be used in the first rectangle (the rectangle at the top-left corner of the screen).
  uint8_t RedMaskSize --> size of direct color red mask in bits
  uint8_t RedFieldPosition;  --> bit position of lsb of red mask

  //extrai a componente vermelha da cor 
  uint8_t red_f =  (first >> info.RedFieldPosition) & (BIT(info.RedMaskSize) - 1); // R(f)
  uint8_t green_f = (first >> info.GreenFieldPosition) & (BIT(info.GreenMaskSize) - 1); // G(f)
  uint8_t blue_f = (first >> info.BlueFieldPosition) & (BIT(info.BlueMaskSize) - 1); // B(f)
*/

  //extrai a componente vermelha da cor 
  uint32_t (R)(uint32_t first){
  return ((1 << mode_info.RedMaskSize) - 1) & (first >> mode_info.RedFieldPosition);
}

  uint32_t (G)(uint32_t first){
  return ((1 << mode_info.GreenMaskSize) - 1) & (first >> mode_info.GreenFieldPosition);
}

  uint32_t (B)(uint32_t first){
  return ((1 << mode_info.BlueMaskSize) - 1) & (first >> mode_info.BlueFieldPosition);
}


  /*Calculates the number of rectangles needed in both vertical and horizontal axis based on the resolution of the screen and the
desired number of rectangles. It then proceeds to iterate through every rectangle and assign a color to it based on the memory model used
by the graphics mode. If the mode is direct color, it assigns a color constructed as a combination of Red, Green and Blue (RGB) values to
the rectangle. Otherwise, if the mode is an indexed mode, it assigns a color to the rectangle using the corresponding index, as determined
by the j and i indices and other parameters. Finally, 'color' variable holds the calculated color for each rectangle.*/
  for (int i = 0 ; i < no_rectangles ; i++) {    // i: row (Y)
    for (int j = 0 ; j < no_rectangles ; j++) { // j: col  (X)
      uint32_t color;
      if (mode_info.MemoryModel == DIRECT_COLOR) {
        //Direct mode: R(row, col) = (R(first) + col * step) % (1 << RedMaskSize)
        uint32_t R = (R(first) + j * step) % (1 << mode_info.RedMaskSize);
        //Direct mode: G(row, col) = (G(first) + row * step) % (1 << GreenMaskSize)
        uint32_t G = (G(first) + i * step) % (1 << mode_info.GreenMaskSize);
        //Direct mode: B(row, col) = (B(first) + (col + row) * step) % (1 << BlueMaskSize)
        uint32_t B = (B(first) + (i + j) * step) % (1 << mode_info.BlueMaskSize);
        color = (R << mode_info.RedFieldPosition) | (G << mode_info.GreenFieldPosition) | (B << mode_info.BlueFieldPosition);
        // uint8_t RedFieldPosition;    /* bit position of lsb of red mask */
      }
      else { //j: col,  i:row
        color = (first+(i*no_rectangles + j)*step) % (1 << mode_info.BitsPerPixel);
      }

      if (vg_draw_rectangle(j * width, i * height, width, height, color)) return 1;
    }
  }
  
  // Função que retorna apenas quando ESC é pressionado
  if (waiting_ESC_key()) return 1;
  // De regresso ao modo texto
  if (vg_exit() != 0) return 1;

  return 0;

}



/************************************************** NOT CHECKED **********************************************************************/

int(video_test_xpm)(xpm_map_t xpm, uint16_t x, uint16_t y) {
  /* To be completed */
  printf("%s(%8p, %u, %u): under construction\n", __func__, xpm, x, y);

  return 1;
}

int(video_test_move)(xpm_map_t xpm, uint16_t xi, uint16_t yi, uint16_t xf, uint16_t yf, int16_t speed, uint8_t fr_rate) {
  /* To be completed */
  printf("%s(%8p, %u, %u, %u, %u, %d, %u): under construction\n",
         __func__, xpm, xi, yi, xf, yf, speed, fr_rate);

  return 1;
}

int(video_test_controller)() {
  /* To be completed */
  printf("%s(): under construction\n", __func__);

  return 1;
}
