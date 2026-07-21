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

// Lista global de Memory Sticks en la CPU
t_list* lista_sticks_cpu = NULL;

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
// 2. BÚSQUEDA Y FRACCIONAMIENTO EN MEMORY STICKS (NUEVO)
// =================================================================
t_stick_ref* obtener_stick_por_direccion(uint32_t dir_fisica_global) {
    if (lista_sticks_cpu == NULL) return NULL;

    for (int i = 0; i < list_size(lista_sticks_cpu); i++) {
        t_stick_ref* stick = (t_stick_ref*) list_get(lista_sticks_cpu, i);
        uint32_t limite_superior = stick->base_fisica + stick->tamano;

        if (dir_fisica_global >= stick->base_fisica && dir_fisica_global < limite_superior) {
            return stick;
        }
    }
    return NULL;
}

void leer_de_memory_sticks(uint32_t dir_fisica_global, void* destino, size_t tamano_total) {
    size_t bytes_leidos_acumulados = 0;

    while (bytes_leidos_acumulados < tamano_total) {
        uint32_t dir_actual = dir_fisica_global + bytes_leidos_acumulados;
        size_t bytes_restantes = tamano_total - bytes_leidos_acumulados;

        t_stick_ref* stick_destino = obtener_stick_por_direccion(dir_actual);

        if (stick_destino == NULL) {
            // Si no se encuentra un Stick específico, se recurre a Kernel Memory
            t_paquete* p_mem = crear_paquete(MOV_IN);
            char str_dir[12], str_tam[12];
            sprintf(str_dir, "%u", dir_actual);
            sprintf(str_tam, "%zu", bytes_restantes);
            agregar_a_paquete(p_mem, str_dir, strlen(str_dir) + 1);
            agregar_a_paquete(p_mem, str_tam, strlen(str_tam) + 1);
            enviar_paquete(p_mem, socket_kernel_memory);
            destruir_paquete(p_mem);

            t_operacion rta = recibir_operacion(socket_kernel_memory);
            t_list* carga_bytes = recibir_carga_util(socket_kernel_memory);
            void* bytes_recibidos = list_get(carga_bytes, 0);

            if (rta == RES_OK && bytes_recibidos != NULL) {
                memcpy((char*)destino + bytes_leidos_acumulados, bytes_recibidos, bytes_restantes);
            }
            list_destroy_and_destroy_elements(carga_bytes, free);
            break;
        }

        uint32_t offset_local = dir_actual - stick_destino->base_fisica;
        uint32_t espacio_disponible = stick_destino->tamano - offset_local;
        size_t bytes_a_leer = (bytes_restantes < espacio_disponible) ? bytes_restantes : espacio_disponible;

        // Pedido directo al socket del Memory Stick
        t_paquete* p_stick = crear_paquete(MOV_IN);
        char str_off[12], str_tam[12];
        sprintf(str_off, "%u", offset_local);
        sprintf(str_tam, "%zu", bytes_a_leer);
        agregar_a_paquete(p_stick, str_off, strlen(str_off) + 1);
        agregar_a_paquete(p_stick, str_tam, strlen(str_tam) + 1);
        enviar_paquete(p_stick, stick_destino->socket);
        destruir_paquete(p_stick);

        t_operacion rta = recibir_operacion(stick_destino->socket);
        t_list* carga = recibir_carga_util(stick_destino->socket);
        void* fragmento = list_get(carga, 0);

        if (rta == RES_OK && fragmento != NULL) {
            memcpy((char*)destino + bytes_leidos_acumulados, fragmento, bytes_a_leer);
        }

        list_destroy_and_destroy_elements(carga, free);
        bytes_leidos_acumulados += bytes_a_leer;
    }
}

void escribir_en_memory_sticks(uint32_t dir_fisica_global, void* origen, size_t tamano_total) {
    size_t bytes_escritos_acumulados = 0;

    while (bytes_escritos_acumulados < tamano_total) {
        uint32_t dir_actual = dir_fisica_global + bytes_escritos_acumulados;
        size_t bytes_restantes = tamano_total - bytes_escritos_acumulados;

        t_stick_ref* stick_destino = obtener_stick_por_direccion(dir_actual);

        if (stick_destino == NULL) {
            // Si no está registrado en la lista de la CPU, envía la petición a Kernel Memory
            t_paquete* p_mem = crear_paquete(MOV_OUT);
            char str_dir[12], str_tam[12];
            sprintf(str_dir, "%u", dir_actual);
            sprintf(str_tam, "%zu", bytes_restantes);
            agregar_a_paquete(p_mem, str_dir, strlen(str_dir) + 1);
            agregar_a_paquete(p_mem, str_tam, strlen(str_tam) + 1);
            agregar_a_paquete(p_mem, (char*)origen + bytes_escritos_acumulados, bytes_restantes);
            enviar_paquete(p_mem, socket_kernel_memory);
            destruir_paquete(p_mem);

            recibir_operacion(socket_kernel_memory);
            t_list* dummy = recibir_carga_util(socket_kernel_memory);
            list_destroy_and_destroy_elements(dummy, free);
            break;
        }

        uint32_t offset_local = dir_actual - stick_destino->base_fisica;
        uint32_t espacio_disponible = stick_destino->tamano - offset_local;
        size_t bytes_a_escribir = (bytes_restantes < espacio_disponible) ? bytes_restantes : espacio_disponible;

        // Escritura fraccionada enviada al socket del Stick correspondiente
        t_paquete* p_stick = crear_paquete(MOV_OUT);
        char str_off[12], str_tam[12];
        sprintf(str_off, "%u", offset_local);
        sprintf(str_tam, "%zu", bytes_a_escribir);
        agregar_a_paquete(p_stick, str_off, strlen(str_off) + 1);
        agregar_a_paquete(p_stick, str_tam, strlen(str_tam) + 1);
        agregar_a_paquete(p_stick, (char*)origen + bytes_escritos_acumulados, bytes_a_escribir);
        enviar_paquete(p_stick, stick_destino->socket);
        destruir_paquete(p_stick);

        recibir_operacion(stick_destino->socket);
        t_list* dummy = recibir_carga_util(stick_destino->socket);
        list_destroy_and_destroy_elements(dummy, free);

        bytes_escritos_acumulados += bytes_a_escribir;
    }
}

// =================================================================
// 3. LOGICA DE PUNTEROS
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
// 4. FUNCIONES DE LA MMU
// =================================================================
void ejecutar_mov_in_real(t_pcb* pcb, char* reg_nombre, int direccion_logica, int tam_max_segmento) {
    int numero_segmento = direccion_logica / tam_max_segmento;
    int desplazamiento = direccion_logica % tam_max_segmento;
    int tam_registro;
    void* reg_ptr;

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

    // Calcula la Dirección Física Global y realiza la lectura fraccionada en los Sticks
    uint32_t dir_fisica_global = (numero_segmento * tam_max_segmento) + desplazamiento;
    leer_de_memory_sticks(dir_fisica_global, reg_ptr, tam_registro);

    uint32_t valor_leido = (tam_registro == 1) ? *((uint8_t*)reg_ptr) : *((uint32_t*)reg_ptr);
    printf("PID: %d Accion: LEER Direccion Fisica: %u Valor: %u\n", pcb->pid, dir_fisica_global, valor_leido);

    pcb->pc++;
}

void ejecutar_mov_out_real(t_pcb* pcb, int direccion_logica, char* reg_nombre, int tam_max_segmento) {
    int numero_segmento = direccion_logica / tam_max_segmento;
    int desplazamiento = direccion_logica % tam_max_segmento;
    int tam_registro;
    void* reg_ptr;

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

    // Calcula la Dirección Física Global y realiza la escritura fraccionada en los Sticks
    uint32_t dir_fisica_global = (numero_segmento * tam_max_segmento) + desplazamiento;
    escribir_en_memory_sticks(dir_fisica_global, reg_ptr, tam_registro);

    uint32_t valor_escrito = (tam_registro == 1) ? *((uint8_t*)reg_ptr) : *((uint32_t*)reg_ptr);
    printf("PID: %d Accion: ESCRIBIR Direccion Fisica: %u Valor: %u\n", pcb->pid, dir_fisica_global, valor_escrito);

    pcb->pc++;
}

// =================================================================
// 5. SUB-FUNCIONES DE EJECUCIÓN
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
// 6. HILO DE ESCUCHA KERNEL
// =================================================================
void* thread_ejecucion(void* arg) {
    if (lista_sticks_cpu == NULL) {
        lista_sticks_cpu = list_create();
    }

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
// 7. CICLO PRINCIPAL (Con Check Interrupt e integración de Sticks)
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

        // CHECK INTERRUPT (No bloqueante)
        if (mantener_en_cpu) {
            uint8_t buffer_interrupcion;
            int bytes_locos = recv(socket_kernel_scheduler, &buffer_interrupcion, 1, MSG_PEEK | MSG_DONTWAIT);
            if (bytes_locos > 0) {
                t_operacion op_interrupcion = recibir_operacion(socket_kernel_scheduler);
                
                printf("## Interrupción recibida\n");
                mantener_en_cpu = false;

                t_paquete* p_interrumpido = crear_paquete(op_interrupcion);
                agregar_a_paquete(p_interrumpido, pcb, sizeof(t_pcb));
                enviar_paquete(p_interrumpido, socket_kernel_scheduler);
                destruir_paquete(p_interrumpido);
            }
        }
    }
}