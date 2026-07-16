#include <commons/collections/list.h>
#include <commons/string.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <utils/hello.h>
#include <thread_ejecucion.h>

#define SEG_FAULT 99

extern int socket_kernel_scheduler;
extern int socket_kernel_memory; 

// =================================================================
// 1. ENUMS Y ENTRADAS
// =================================================================
typedef enum {
    CMD_SET,
    CMD_SUM,
    CMD_SUB,
    CMD_JNZ,
    CMD_MOV_IN,
    CMD_MOV_OUT,
    CMD_EXIT,
    CMD_INVALIDO
} t_comando_codigo;

t_comando_codigo de_string_a_comando(char* comando) {
    if (strcmp(comando, "SET") == 0)     return CMD_SET;
    if (strcmp(comando, "SUM") == 0)     return CMD_SUM;
    if (strcmp(comando, "SUB") == 0)     return CMD_SUB;
    if (strcmp(comando, "JNZ") == 0)     return CMD_JNZ;
    if (strcmp(comando, "MOV_IN") == 0)  return CMD_MOV_IN;
    if (strcmp(comando, "MOV_OUT") == 0) return CMD_MOV_OUT;
    if (strcmp(comando, "EXIT") == 0)    return CMD_EXIT;
    return CMD_INVALIDO;
}

// =================================================================
// 2. LOGICA DE PUNTEROS
// =================================================================
void* obtener_puntero_registro(t_pcb* pcb, char* nombre_reg, int* tamano) {
    if (strcmp(nombre_reg, "AX") == 0) { *tamano = 1; return &(pcb->contexto.ax); }
    if (strcmp(nombre_reg, "BX") == 0) { *tamano = 1; return &(pcb->contexto.bx); }
    if (strcmp(nombre_reg, "CX") == 0) { *tamano = 1; return &(pcb->contexto.cx); }
    if (strcmp(nombre_reg, "DX") == 0) { *tamano = 1; return &(pcb->contexto.dx); }

    if (strcmp(nombre_reg, "EAX") == 0) { *tamano = 4; return &(pcb->contexto.eax); }
    if (strcmp(nombre_reg, "EBX") == 0) { *tamano = 4; return &(pcb->contexto.ebx); }
    if (strcmp(nombre_reg, "ECX") == 0) { *tamano = 4; return &(pcb->contexto.ecx); }
    if (strcmp(nombre_reg, "EDX") == 0) { *tamano = 4; return &(pcb->contexto.edx); }
    if (strcmp(nombre_reg, "SI") == 0)  { *tamano = 4; return &(pcb->contexto.si); }
    if (strcmp(nombre_reg, "DI") == 0)  { *tamano = 4; return &(pcb->contexto.di); }

    return NULL; 
}

// =================================================================
// 3. FUNCIONES DE LA MMU
// =================================================================
void ejecutar_mov_in_real(t_pcb* pcb, char* reg_nombre, int direccion_logica, int tam_max_segmento) {
    int numero_segmento = direccion_logica / tam_max_segmento;
    int desplazamiento = direccion_logica % tam_max_segmento;
    int tam_registro;
    void* reg_ptr;
    t_paquete* p_mem;
    t_operacion rta;
    t_list* carga_bytes;
    void* bytes_recibidos;
    uint32_t valor_leido;

    char str_pid[12], str_seg[12], str_offset[12], str_tam[12];

    reg_ptr = obtener_puntero_registro(pcb, reg_nombre, &tam_registro);
    if (reg_ptr == NULL) {
        pcb->pc++;
        return;
    }

    if (desplazamiento + tam_registro > tam_max_segmento) {
        printf("[MMU] Error: Segmentation Fault detectado para PID %d\n", pcb->pid);
        t_paquete* p_error = crear_paquete(SEG_FAULT);
        agregar_a_paquete(p_error, pcb, sizeof(t_pcb));
        enviar_paquete(p_error, socket_kernel_scheduler);
        destruir_paquete(p_error);
        return;
    }

    p_mem = crear_paquete(MOV_IN);
    sprintf(str_pid, "%d", pcb->pid);
    sprintf(str_seg, "%d", numero_segmento);
    sprintf(str_offset, "%d", desplazamiento);
    sprintf(str_tam, "%d", tam_registro);

    agregar_a_paquete(p_mem, str_pid, strlen(str_pid) + 1);
    agregar_a_paquete(p_mem, str_seg, strlen(str_seg) + 1);
    agregar_a_paquete(p_mem, str_offset, strlen(str_offset) + 1);
    agregar_a_paquete(p_mem, str_tam, strlen(str_tam) + 1);

    enviar_paquete(p_mem, socket_kernel_memory);
    destruir_paquete(p_mem);

    rta = recibir_operacion(socket_kernel_memory);
    carga_bytes = recibir_carga_util(socket_kernel_memory);
    bytes_recibidos = list_get(carga_bytes, 0);

    if (rta == RES_OK && bytes_recibidos != NULL) {
        memcpy(reg_ptr, bytes_recibidos, tam_registro);
        valor_leido = (tam_registro == 1) ? *((uint8_t*)reg_ptr) : *((uint32_t*)reg_ptr);
        printf("PID: %d Accion: LEER Direccion Fisica: %d Valor: %u\n", pcb->pid, direccion_logica, valor_leido);
    }

    list_destroy_and_destroy_elements(carga_bytes, free);
    pcb->pc++;
}

void ejecutar_mov_out_real(t_pcb* pcb, int direccion_logica, char* reg_nombre, int tam_max_segmento) {
    int numero_segmento = direccion_logica / tam_max_segmento;
    int desplazamiento = direccion_logica % tam_max_segmento;
    int tam_registro;
    void* reg_ptr;
    t_paquete* p_mem;
    t_operacion rta;
    uint32_t valor_escrito;

    char str_pid[12], str_seg[12], str_offset[12], str_tam[12];

    reg_ptr = obtener_puntero_registro(pcb, reg_nombre, &tam_registro);
    if (reg_ptr == NULL) {
        pcb->pc++;
        return;
    }

    if (desplazamiento + tam_registro > tam_max_segmento) {
        printf("[MMU] Error: Segmentation Fault detectado para PID %d\n", pcb->pid);
        t_paquete* p_error = crear_paquete(SEG_FAULT);
        agregar_a_paquete(p_error, pcb, sizeof(t_pcb));
        enviar_paquete(p_error, socket_kernel_scheduler);
        destruir_paquete(p_error);
        return;
    }

    p_mem = crear_paquete(MOV_OUT);
    sprintf(str_pid, "%d", pcb->pid);
    sprintf(str_seg, "%d", numero_segmento);
    sprintf(str_offset, "%d", desplazamiento);
    sprintf(str_tam, "%d", tam_registro);

    agregar_a_paquete(p_mem, str_pid, strlen(str_pid) + 1);
    agregar_a_paquete(p_mem, str_seg, strlen(str_seg) + 1);
    agregar_a_paquete(p_mem, str_offset, strlen(str_offset) + 1);
    agregar_a_paquete(p_mem, str_tam, strlen(str_tam) + 1);
    agregar_a_paquete(p_mem, reg_ptr, tam_registro); 

    enviar_paquete(p_mem, socket_kernel_memory);
    destruir_paquete(p_mem);

    rta = recibir_operacion(socket_kernel_memory);
    if (rta == RES_OK) {
        valor_escrito = (tam_registro == 1) ? *((uint8_t*)reg_ptr) : *((uint32_t*)reg_ptr);
        printf("PID: %d Accion: ESCRIBIR Direccion Fisica: %d Valor: %u\n", pcb->pid, direccion_logica, valor_escrito);
    }
    
    pcb->pc++;
}

// =================================================================
// 4. SUB-FUNCIONES DE EJECUCIÓN
// =================================================================
void ejecutar_set(t_pcb* pcb, char* reg_nombre, int valor) {
    int tamano;
    void* reg_ptr = obtener_puntero_registro(pcb, reg_nombre, &tamano);
    if (reg_ptr != NULL) {
        if (tamano == 1)      *((uint8_t*)reg_ptr) = (uint8_t)valor;
        else if (tamano == 4) *((uint32_t*)reg_ptr) = (uint32_t)valor;
    }
    pcb->pc++;
}

void ejecutar_jnz(t_pcb* pcb, char* reg_nombre, int proximo_pc) {
    int tamano;
    void* reg_ptr = obtener_puntero_registro(pcb, reg_nombre, &tamano);
    if (reg_ptr != NULL) {
        uint32_t valor = (tamano == 1) ? *((uint8_t*)reg_ptr) : *((uint32_t*)reg_ptr);
        if (valor != 0) pcb->pc = proximo_pc;
        else pcb->pc++;
    } else {
        pcb->pc++;
    }
}

// =================================================================
// 5. HILO DE ESCUCHA KERNEL
// =================================================================
void* thread_ejecucion(void* arg) {
    while (1) {
        t_operacion op = recibir_operacion(socket_kernel_scheduler);

        if (op == EXECUTE_PROCESS) {
            t_list* carga = recibir_carga_util(socket_kernel_scheduler);
            t_pcb* pcb = (t_pcb*) list_get(carga, 0); 
            
            ejecutar_ciclo_de_instruccion(pcb, 5);
            
            list_destroy_and_destroy_elements(carga, free);
        } else {
            break;
        }
    }
    return NULL;
}

// =================================================================
// 6. CICLO PRINCIPAL (Con Check Interrupt integrado)
// =================================================================
void ejecutar_ciclo_de_instruccion(t_pcb* pcb, int delay) {
    bool mantener_en_cpu = true;
    char str_pid[12], str_pc[12];
    t_paquete* p_fetch;
    t_operacion rta_memoria;
    t_list* carga_instruccion;
    char* instruccion;
    char** partes;
    char* comando;
    t_comando_codigo cmd_codigo;

    while (mantener_en_cpu) {
        // FETCH
        printf("## PID: %d FETCH Program Counter: %d\n", pcb->pid, pcb->pc);

        sprintf(str_pid, "%d", pcb->pid); 
        sprintf(str_pc, "%d", pcb->pc);

        p_fetch = crear_paquete(MOV_IN); 
        agregar_a_paquete(p_fetch, str_pid, strlen(str_pid) + 1);
        agregar_a_paquete(p_fetch, str_pc, strlen(str_pc) + 1);
        enviar_paquete(p_fetch, socket_kernel_memory);
        destruir_paquete(p_fetch);

        rta_memoria = recibir_operacion(socket_kernel_memory);
        if (rta_memoria != RES_OK) { mantener_en_cpu = false; break; }

        carga_instruccion = recibir_carga_util(socket_kernel_memory);
        instruccion = (char*) list_get(carga_instruccion, 0);

        if (instruccion == NULL) {
            list_destroy_and_destroy_elements(carga_instruccion, free);
            mantener_en_cpu = false; break;
        }

        // DECODE
        partes = string_split(instruccion, " ");
        comando = *(partes + 0);

        printf("## PID: %d Ejecutando: %s\n", pcb->pid, instruccion);

        usleep(delay * 1000); 

        // EXECUTE
        cmd_codigo = de_string_a_comando(comando);

        switch (cmd_codigo) {
            case CMD_SET:
                ejecutar_set(pcb, *(partes + 1), atoi(*(partes + 2)));
                break;

            case CMD_SUM:
                pcb->contexto.ax = pcb->contexto.ax + pcb->contexto.bx;
                pcb->pc++;
                break;

            case CMD_SUB:
                pcb->contexto.ax = pcb->contexto.ax - pcb->contexto.bx;
                pcb->pc++;
                break;

            case CMD_JNZ:
                ejecutar_jnz(pcb, *(partes + 1), atoi(*(partes + 2)));
                break;

            case CMD_MOV_IN:
                ejecutar_mov_in_real(pcb, *(partes + 1), pcb->contexto.si, 256);
                break;

            case CMD_MOV_OUT:
                ejecutar_mov_out_real(pcb, pcb->contexto.di, *(partes + 1), 256);
                break;

            case CMD_EXIT:
                mantener_en_cpu = false; 
                t_paquete* p_vuelta = crear_paquete(EXECUTE_PROCESS);
                agregar_a_paquete(p_vuelta, pcb, sizeof(t_pcb)); 
                enviar_paquete(p_vuelta, socket_kernel_scheduler);
                destruir_paquete(p_vuelta);
                break;

            default:
                mantener_en_cpu = false;
                break;
        }

        string_array_destroy(partes);
        list_destroy_and_destroy_elements(carga_instruccion, free);

        // CHECK INTERRUPT (No bloqueante - Pág. 22)
        if (mantener_en_cpu) {
            uint8_t buffer_interrupcion;
            // Miramos de forma no bloqueante si el Kernel mandó algo por el socket
            int bytes_locos = recv(socket_kernel_scheduler, &buffer_interrupcion, 1, MSG_PEEK | MSG_DONTWAIT);
            if (bytes_locos > 0) {
                // Si había bytes, capturamos la interrupción real
                t_operacion op_interrupcion = recibir_operacion(socket_kernel_scheduler);
                
                printf("## Interrupción recibida\n"); // Log obligatorio (Pág. 23)
                mantener_en_cpu = false;

                // Devolvemos el PCB actualizado al Kernel por interrupción/desalojo
                t_paquete* p_interrumpido = crear_paquete(op_interrupcion);
                agregar_a_paquete(p_interrumpido, pcb, sizeof(t_pcb));
                enviar_paquete(p_interrumpido, socket_kernel_scheduler);
                destruir_paquete(p_interrumpido);
            }
        }
    }
}