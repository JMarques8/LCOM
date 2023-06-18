/************** IF TESTE == PLACA GRAFICA ************/



1st - Verificar/Validar Modo dado:

-> descobrir as caracteristicas do modo;

    res_x:
    res_y:
    bits_per_pixel:

(prof deve dar uma estrutura de dados com essa informação e nos temos que  primeiro declarar a estrutura de dados 
(equivalente a vbe_mode_info_t mode_info;) que vai comportar a memoria utilizada e depois chamamos a função com o modo
para q aquela estrutura de dados possua todos os dados do modo dado - vbe_get_mode_info(mode, &mode_info);

ATENTION!!! as funções q não são void como o caso de vbe_get_mode_info temos meter um "if(vbe_get..() != 0) return 1".
Portanto todas as funçoes auxil que chamarmos convem q nao sejam void e q façamos aquela condição para fazer catch caso retornem 
algum erro e assim o programar correr na mesma
Depois ja temos a info que queremos das caract do modo, mode_info.x_res etc;

Depois, mudar o modo do minix do modo texto para o modo grafico;
--> ou fazemos nos uma função que faz isso como nos labs;
--> ou o prof ja da uma para nos usarmos, e ai temos que chamar essa função,
por exemplo: if(vbe_set_mode(mode))... e o minix ja altera para modo grafico

Porque é que temos q fazer o vbe_get_mode_info antes? q é para caso nos seja dado o modo errado, fazemos catch do erro antes!
Mais uma vez, se houver erro ao entrar em modo grafico, fazemos catch de erro tambem.

Depois de estar em modo grafico o que temos de fazer é  manipular a memoria grafica, i.e, temos que alocar memo suficiente para
aquele modo.

Exemplo: modo que usa 15 bits por pixel (5;5;5), masss nao podemos usar estes 15 bits por pixel uma vez que a memoria é alocada em
BYTES entao temos que fazer uma transformação de bits para bytes

solução: ha modos em que é facil, tipo modo (8;8;8) - 24 bits logo 3 bytes
no entanto modos como o de 15 bits ja exigem mais cuidado, pelo que devemos fazer arredondamentos por excesso (nao existe 1 byte e meio..)

fazer algo como:

unint8_t bytes_per_pixel = (mode_info.bits_per_pixel+7)/8;

Depois tendo o numero de BYTES por pixel, o frame buffer (nome dado a memoria alocada por cada modo), o numero 
de bytes unint32_t frame_buffer_size sera igual a res.x*res.y*numero de bytes por pixel;

O prof poderá já dar uma função do tipo unint8_t* vbe_set_frame(size_t SIZE) (retorna um apontador para o inicio da memoria virtulal)
ao inves de usar unint32_t pro frame_buffer_size remos que usar size_t, temos que usar as definições e variaveis que o prof dá.

Supondo que o prof ja da uma função como a do set_frame_buffer (mapear memoria fisica a memoria virtual), e nesse caso so temos que 
meter nos os argumentos em condições, sendo esses argumentos o size do frame buffer e depois chamamos a função, e ele aloca directamente
a memoria certa


unint8_t* frame_buffer -> um array de memoria onde podemos "pintar" os bytes em determinadas coordenadas para que aquela cor seja
colocada ai, devemos declarar como um apontador de 8 bits, sempr de 8 bits, pq so assim é que conseguimos fazer os offsets em condições,
portanto frame_buffer é um apontador de 8 bits, e chamamos a função que o prof talvez de, e ficamos com algo tipo assim:

unint8_t* frame_buffer = vbe_set_frame(size_t frame_buffer_size); // c o size q nos calculamos
assim isto retorna um apontador para essa tal estrutura que vai comportar os nossos pixeis

ATENTION! aquela função como retorna um apontador, se ocorrer algum erro o apontador q ela retorna é nulo, portanto convem antes
de avançar  meter um "if(frame_buffer == NULL) return 1".

TIP!! ate pode ser que na hipotetica função que o prof dê, tipo vbe_set_frame ele possa pedir q coloquemos mais coisas, como por exemplo o endereço fisico (que é um argumento de modo_info), ou o modo tb

O importante no teste é nos nao nos enganarmos nas contas

********************************* PINTAR - manipular memoria *********************************************************

copiar a cor que o prof da como argumento, copiar nas coordernadas x, y

função auxliar: int draw_pixel(x, y, color)

vamos precisar do frame buffer por isso o frame buffer vai passar a ser uma variavel global o modo tambem....

eu tenho de copiar exactamente BYTES per pixel

memcpy(&to, &from, quantidade) //NAO ESQUECER QUE O TO E O FROM SAO REFERENCIAS
to - para onde é q eu quero enviar os dados (p/ o frame buffer)
from - de onde é q eu quero os dados (da variavel color - queremos a zona onde esta colocada a cor)
quantidade - (numero de bytes per pixel)

NOTA!! no argumento "to", nao é meter exactamente o frame buffer, é a zona onde esta colocada o frame buffer, so q dado um x e dado um y,
nos temos q andar com um offset (??) -> nos queremos colocar a cor exactamente naquelas coordenas x e y daquele array, temos que ter aqui
um offset, o frame buffer aponta exactamente para o inicio (vertice 0,0), calculo do offset: x.res*y+x
ouuu melhor (mode_info.x_res * y) + x
Isto estaria quase correcto se o modo permitisse apenas 1 byte, portanto para estar 100% correcto temos que depois multiplicar pelo 
numero de bytes_per_pixel senao as contas nao vao dar certo (por causa do tamanho de cada "celula" do array)
calculo do offset: ((mode_info.x_res * y) + x )*bytes_per_pixel;

pintar um rectangulo -> ciclo for

verificar se ha erro no memcpy tambem!!!
mmcpy retirn apontador, por isso fazemos if memcpy == NULL) return 1;


*/