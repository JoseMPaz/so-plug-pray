#ifndef THREAD_EJECUCION_H_
#define THREAD_EJECUCION_H_

#include <utils/hello.h> 

typedef struct {
    int id;               
    int socket;           
    uint32_t tamano;       
    uint32_t base_fisica;  
} t_stick_ref;


extern t_list* lista_sticks_cpu;

t_stick_ref* obtener_stick_por_direccion(uint32_t dir_fisica_global);

void leer_de_memory_sticks(uint32_t dir_fisica_global, void* destino, size_t tamano_total);
void escribir_en_memory_sticks(uint32_t dir_fisica_global, void* origen, size_t tamano_total);

void* thread_ejecucion(void* arg);
void ejecutar_ciclo_de_instruccion(t_pcb* pcb, int delay);

#endif