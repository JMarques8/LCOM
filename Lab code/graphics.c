#include <lcom/lcf.h>
#include "graphics.h"
//from prof's handouts - Minix 3 Notes
vbe_mode_info_t mode_info; // struct vbe_mode_info_t -> para ver info completa ver pdf lab 5 Guião "Minix 3 Notes" pags. 4 & 5
static uint8_t* frame_buffer; // uint8_t* frame_buffer; frame-buffer VM (virtual memory) address -- Process (virtual) address to which VRAM is mapped
static unsigned int h_res; // Horizontal resolution in pixels --> mode_info.XResolution
static unsigned int v_res; // Vertical resolution in pixels --> mode_info.YResolution
static unsigned int bytes_per_pixel;  //(mode_info.BitsPerPixel + 7) / 8;


//0. & 2. Mudança do Minix para modo gráfico c/ as especificações de 'mode'
int set_vbe_mode(uint16_t mode) { /*Fazer kernel call (sys_int86) para dar set up a BIOS c/ as especific do mode desejado, cujo argum é type reg86_t*/

/*reg86_t is a struct with a union of anonymus structs that allow access the IA32 registers as
 8-bit registers - 16-bit registers - 32-bit registers
 The names of the members of the structs are the standard names of IA-32 registers.*/

  reg86_t reg86;
/*In order to avoid unexpected behavior by libx86emu, whenever you make a INT 0x10 call, you should clear the ununsed
registers of the reg86_t struct before passing it to sys_int86(), using for example memset()

*memset(void *s, int c, size_t n);function	fills  the  first  n  bytes of the memory area pointed to by s with the	constant byte c. */

  memset(&reg86, 0, sizeof(reg86)); // inicialização da estrutura com o valor 0 em todos os parâmetros
  //reg86.ax = 0x4F02;  VBE call, function 02 -- set VBE mode
  reg86.ah = 0x4F;  // register 'A' high bits - parte mais significativa de AX
  reg86.al = 0x02; // register 'A' low bits - parte menos significativa de AX
  reg86.bx = mode | BIT(14); //reg86.bx = 1<<14|0x105;  Bit 14 SET for linear framebuffer
  reg86.intno = 0x10; // intno é sempre 0x10  - interrupt number - identifica BIOS video card service
/*Access to BIOS services is via the SW interrupt instruction & Any arguments required are passed via the processor registers*/
  if(sys_int86(&reg86) != 0){
    printf("set_vbe_mode: sys_int86() failed\n");
    return 1;
  }
/*
 ******* TEORETICAL NOTES *******
  Minix 3 executes in protected mode, however VBE requires the invocation of INT 0x10 in real-mode.
  To solve this, Minix 3.1.x offers the SYS_INT86 kernel call - this kernel call switches from 32-bit protected mode to (16-bit) 
  real-mode, and then executes a software interrupt instruction INT. This instruction takes as an argument an 8-bit integer,
  that specifies the interrupt number. As a result, the processor jumps to the corresponding handler, which must be configured
  at boot time. After the execution of that handler, SYS_INT86 switches back to 32-bit protected mode. The main use of this
  call in Minix 3.1.x was to make BIOS calls.

  VBE functions are called using the INT 10h interrupt vector, passing arguments in the 80X86 registers. The INT 10h interrupt handler
  first determines if a VBE function has been requested, and if so, processes that request. Otherwise control is passed to the standard VGA BIOS for completion.

**VBE Return Status**
  The AX register is used to indicate the completion status upon return from VBE functions. If VBE support for the specified function is available,
  the 4Fh value passed in the AH register on entry is returned in the AL register. If the VBE function completed successfully, 00h is returned in the AH
  register. Otherwise the AH register is set to indicate the nature of the failure. */
  return 0;
}


// 1. map the video memory to the process' address space -> Construção do frame buffer virtual e físico
int (set_frame_buffer)(uint16_t mode){
  /* Before you can write to the frame buffer: 
    1. Obtain the physical memory address
      --> Use vbe_get_mode_info(): This function retrieves information about the input VBE mode, including the physical address of the frame buffer
    2. Map the physical memory region into the process’ (virtual) address space
*/

  /*This code defines a function called "set_frame_buffer" whose argument represents a VBE graphics mode. The function then defines a structure called 
"vbe_mode_info_t" which contains various parameters used to set up the graphics mode. The structure is packed with no padding between its members using a
#pragma pack() directive.  Next, the function declares an instance of the "vbe_mode_info_t" structure called "mode_info". It initializes the structure to all 
zeroes using the memset() function. Finally, the function calls another function called "vbe_get_mode_info" to obtain information about the VBE mode specified
by the argument passed to the "set_frame_buffer" function. If the call to "vbe_get_mode_info" fails, the function returns 1.*/

  /* vbe_mode_info_t ----> para ver info completa ver pdf lab 5 Guião "Minix 3 Notes" pags. 4 & 5
  typedef struct {
    uint16_t ModeAttributes;  [...]
    uint16_t XResolution;
    uint16_t YResolution;
    uint8_t BitsPerPixel;  [...]
    uint8_t RedMaskSize;
    uint8_t RedFieldPosition;  [...]
    uint8_t RsvdMaskSize;
    uint8_t RsvdFieldPosition;
    uint32_t PhysBasePtr;
    [...]
    } vbe_mode_info_t;

Os parâmetros importantes de vbe_mode_info_t são os seguintes:

PhysBasePtr - endereço físico do início do frame buffer;
XResolution - resolução horizontal, em píxeis;
YResolution - resolução vertical, em píxeis;
BitsPerPixel - número de bits por píxel;
<COLOR>MaskSize - número de bits da máscara de cores no modo direto. No modo 0x11A GreenMaskSize = 6;
<COLOR>FieldPosition - posição da máscara de cores. No modo 0x11A GreenFieldPosition = 5;
MemoryModel - 0x06 se for em modo direto, 0x105 se for em modo indexado;
Onde <COLOR> pode uma qualquer cor primária do sistema RGB: Red, Green ou Blue.

*/

  // vbe_mode_info_t mode_info
  // memset initialize mode_info structure to all zeros to ensure that all fields of the structure are cleared before obtaining the values from the "vbe_get_mode_info" function
  memset(&mode_info, 0, sizeof(mode_info));
  //provided in LCF: vbe_get_mode_info(): retrieves info about the input VBE mode, including the physical address of the frame buffer (physical memory address)
  if(vbe_get_mode_info(mode, &mode_info)) return 1;
  
  unsigned int h_res = mode_info.XResolution;
  unsigned int v_res = mode_info.YResolution;

  /* The physical address and the size of VRAM for a given graphics mode can be obtained using VBE function 0x01 (the vbe_get_mode_info())  */

  int r;
  /* struct minix_mem_range {
      phys_bytes mr_base; // Lowest memory address in range 
      phys_bytes mr_limit; // Highest memory address in range 
    };
 */ 
  struct minix_mem_range mr; /* physical memory range */
  unsigned int vram_base = mode_info.PhysBasePtr; /* VRAM’s physical addresss */
  //unsigned int vram_size: VRAM’s size, but you can use the frame-buffer size, instead
  
  unsigned int bytes_per_pixel = (mode_info.BitsPerPixel + 7) / 8; //Cálculo dos Bytes per pixel c/arredondamento por excesso pq info ta em bites e queremos BYTES)
  unsigned int frame_size = h_res * v_res * bytes_per_pixel;
  /*The video RAM (VRAM) size and the frame buffer size are related concepts, but they are not always the same thing.
In some cases, the video RAM may be used as the frame buffer, meaning that the amount of VRAM determines the maximum size of the frame buffer.
In this scenario, the frame buffer size would be equal to the VRAM size. This is common in older graphics cards and integrated graphics solutions.
However, modern graphics cards often use dedicated memory for the frame buffer, which is separate from the VRAM.*/

  /******* Allow memory mapping ******/
  mr.mr_base = (phys_bytes) vram_base;
  mr.mr_limit = mr.mr_base + frame_size;  //needed bc of sys_privctl using &mr that is a struct with both base & limit info

  /*To grant a process the permission to map a given address range use MINIX 3 kernel call: int sys_privctl  (pag2 Minix 3 Notes)
  SYS_PRIVCTL: Get a private privilege structure and update a process' privileges. This is used to dynamically start system services.
  For VM (virtual memory), this call also supports querying a memory range for previously added memory permissions.
*/
  if( OK != (r = sys_privctl(SELF, SYS_PRIV_ADD_MEM, &mr))){ //OK: same as ok == 0.
    panic("sys_privctl (ADD_MEM) failed: %d\n", r);
  }

  /******** Map memory *******/
  /*To map the VRAM to its address space use MINIX 3 kernel call: void *vm_map_phys(endpoint_t who, void *phaddr, size_t len)
The first argument is a value that identifies the process on whose address space the physical memory region should be mapped.
Of course phaddr specifies the physical address of the first memory byte in that region and len the region's length. This call
returns the virtual address (of the first physical memory position) on which the physical address range was mapped. Subsequently,
a process can use the returned virtual address to access the contents in the mapped physical address range.*/
  frame_buffer = vm_map_phys(SELF, (void *)mr.mr_base, frame_size); //same as frame_buffer do codigo do fabio
  if(frame_buffer == MAP_FAILED){
    panic("couldnt map video memory");
  }
  
  return 0;
}


// Atualização da cor de um pixel
int (vg_draw_pixel)(uint16_t x, uint16_t y, uint32_t color) {

  // As coordenadas têm de ser válidas
  if(x >= h_res || y >= v_res) return 1;
 
  // Índice (em bytes) da zona do píxel a colorir
  unsigned int index = (h_res * y + x) * bytes_per_pixel;  //calculado como se fosse uma area quase - ver video do monitor fabio

  // A partir da zona frame_buffer[index], copia @Bbytes_per_pixel bytes da @color
  /*void *memcpy(void *dest, const void *src, size_t n) copies n characters from memory area src to memory area dest.*/
   if (memcpy(&frame_buffer[index], &color, bytes_per_pixel)== NULL) return 1;

  return 0;
}


// Desenha uma linha horizontal
int (vg_draw_hline)(uint16_t x, uint16_t y, uint16_t width, uint32_t color) {
  for (unsigned int i = 0 ; i < width ; i++)   
    if (vg_draw_pixel(x+i, y, color) != 0) return 1;
  return 0;
}


//3. draw a rectangle -> Desenha o rectângulo
int (vg_draw_rectangle)(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color) {
  for(unsigned int i = 0; i < height ; i++)
    if (vg_draw_hline(x, y+i, width, color) != 0) {
      vg_exit();
      return 1;
    }
  return 0;
}
