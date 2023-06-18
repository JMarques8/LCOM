#include <lcom/lcf.h>
#include <stdint.h>
#include <stdio.h>

vbe_mode_info_t mode_info;
static uint8_t* frame_buffer;
static unsigned int h_res;
static unsigned int v_res;
static unsigned int bytes_per_pixel;

/*struct minix_mem_range {
      phys_bytes mr_base; // Lowest memory address in range 
      phys_bytes mr_limit; // Highest memory address in range 
    };*/

int vbe_set_mode(uint16_t mode){

    //invocar minix kernel call pra aceder a BIOS (sys_int86) e alterar modo
    //sys_int86: tem como argumento estrutura reg86_t reg86 - registro 86bits

    reg86_t reg86;

    /*antes de configurar reg86 com as especificaçoes do modo de input converm
    certificar que a estrututa esta toda a zero para evitar erros --> memset()*/

    memset(&reg86, 0, size_of(reg86));
    //escrever nos resgistos de reg86 as configurações do input mode
    reg86.ax = 0x4F02; //VBE call, function 02 -- set VBE mode
    reg86.bx = mode | BIT(14); //Bit 14 SET for linear framebuffer
    reg86.intno = 0x10;       //interrupt number
    //invocar sys_int c/ argumento reg 86 ja devidamente confifurado como queremos
    if(sys_int86(&reg86) !=0){
        return 1;
    }

    return 0;
}

int set_frame_buffer(uint16_t mode){

    /*map the VRAM to its address space, by using MINIX 3's kernel call: void *vm_map_phys(endpoint_t who, void *phaddr, size_t len)*/
    /*To grant a process the permission to map a given address range you should use MINIX 3 kernel call: 
    int sys_privctl(endpoint_t proc_ep, int request, void *p)
        > endpoint_t proc_ep: SEL;
        > int request: SYS_PRIV_ADD_MEM;.m
        > void *p = memory range to map; */

    /*Before you can write to the frame buffer:
        1. Obtain the physical memory address
	        >> Use vbe_get_mode_info() (provided as part of LCF): This function retrieves information about the input VBE mode, 
            including the physical address of the frame buffer
        2. Map the physical memory region into the process’ (virtual) address space */

   /*para recolher a info que precisamos temos que usar a funçao vbe_get_mode_info() que tem como
   argumentos o modo de input e uma estrutura vbe_mode_info_t onde é escrito a info do mode.
   Portanto vamos começar por garantir q a estrutura vbe_mode_info nao tem "lixo" */

    memset(&mode_info, 0, size_of(mode_info));
    //agora podemos chamar a funçao sem risco de conter info invalida
    if(vbe_get_mode_info(mode, &mode_info) !=0){
        return 1;
    }

    //agora q ja temos mode_info preenchido com os dados do mode, podemos ir buscar a info q queremos
    unsigned int h_res = mode_info.XResolution;
    unsigned int v_res = mode_info.YResolution;
    unsigned int bytes_per_pixel = (mode_info.BitsPerPixel + 7)/8;
    unsigned int buffer_size = h_res*v_res*bytes_per_pixel;

    /* prof from's docs*/
    struct minix_mem_range mr;
    unsigned int vram_base = mode_info.PhysBasePtr; /* VRAM's physical addresss */
    
    //Request Permission to map memory:  int sys_privctl(SELF, SYS_PRIV_ADD_MEM, void *p)
    mr.mr_base = (phys_bytes) vram_base;
    mr.mr_limit = mr.mr_base + buffer_size;

    if(sys_privctl(SELF, SYS_PRIV_ADD_MEM, &mr)!=0){
        return 1;
    }

    //Map Memory void *vm_map_phys(endpoint_t who, void *phaddr, size_t len)
    frame_buffer = vm_map_phys(SELF, (void *)mr.mar_base, buffer_size);
    if(frame_buffer == NULL){
        return 1;
    }
    return 0;

}

int (vg_draw_rectangle)(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color){
    for(int i=0; i<height; i++){
        if(vg_draw_hline(x, y+i, width, color)!=0){
            vg_exit();
            return 1;
        }
    }
    return 0;
}

int vg_draw_hline(uint16_t x, uint16_t y, uint16_t width, uint32_t color){

    for(int j=0; j<width; i++){
        if(vg_draw_pixel(x+j, y, color)!=0){
            return 1;
        }
    }
    return 0;
}

int vg_draw_pixel(uint16_t x, uint16_t y, uint32_t color){

    //para pintar um pixel, temos que copiar para a mem virtual frame buffer a cor e as coord do pixel

    //1o - verificar se as coordenadas são validas
    if(x>=h_res || y>=v_res){
        return 1;
    }

    //vamos escrever na mem virtual frame buffer, que por sua vez é um array e como tal,
    //temos q saber qual o indice do array a que vamos aceder em memoria e q corresponde
    //as coordenas x,y do pixel eu quero pintar -> calculo do offset em memoria

    unsigned int index = (h_res*y+x)*bytes_per_pixel;

    //para 'pintar', uso *void *memcpy(void *dest, const void *src, size_t n) copies n characters from memory area src to memory area dest.
    //area destination is frame_buffer[index] || source of what i want to copy: is the color || size n: bytes_per_pixel
    //vamnos copiar n bytes_per_pixel de color para o pixel localizado em frame_buffer[index]
    if(memcpy(&frame_buffer[index], &color, bytes_per_pixel) == NULL){
        return 1;
    }
    return 0;
}