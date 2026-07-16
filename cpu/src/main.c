#include <utils/hello.h>
#include <unistd.h> 
#include <thread_ejecucion.h>

t_config * config = NULL;
t_log * logger = NULL;

int socket_kernel_scheduler; 
int socket_kernel_memory; 
int socket_memory_stick; 

char * log_level;
char * ip_kernel_scheduler;
char * puerto_kernek_scheduler;
char * ip_kernel_memory;
char * puerto_kernek_memory;
char * ip_memory_stick;
char * puerto_memory_stick;

int main(int argc, char * argv[]) 
{
    // Validación de argumentos según enunciado (Pág. 19)
    if (argc < 3) {
        printf("Error: Faltan argumentos. Uso correcto: %s [Ruta Config] [Identificador]\n", argv[0]);
        return EXIT_FAILURE;
    }

    char* ruta_config = argv[1];
    char* id_cpu = argv[2]; // Capturamos el identificador dinámico

    config = iniciar_config (ruta_config);
    
    log_level = config_get_string_value (config, "LOG_LEVEL");  
    ip_kernel_scheduler = config_get_string_value (config, "IP_KERNEL_SCHEDULER");
    puerto_kernek_scheduler = config_get_string_value (config, "PUERTO_KERNEL_SCHEDULER");
    ip_kernel_memory = config_get_string_value (config, "IP_KERNEL_MEMORY");
    puerto_kernek_memory = config_get_string_value (config, "PUERTO_KERNEL_MEMORY");
    ip_memory_stick = config_get_string_value (config, "IP_MEMORY_STICK");
    puerto_memory_stick = config_get_string_value (config, "PUERTO_MEMORY_STICK");
    
    // El nombre del archivo log ahora incluye el identificador de la CPU (Pág. 19)
    char nombre_log[50];
    sprintf(nombre_log, "cpu_%s.log", id_cpu);

    logger = iniciar_log ( nombre_log, "CPU", log_level_from_string ( log_level ) );
    log_info ( logger, "MODULO CPU %s HA INICIADO", id_cpu);
    
    socket_kernel_scheduler = crear_socket ( CLIENTE, ip_kernel_scheduler, puerto_kernek_scheduler);
    socket_kernel_memory = crear_socket ( CLIENTE, ip_kernel_memory, puerto_kernek_memory);
    socket_memory_stick = crear_socket ( CLIENTE, ip_memory_stick, puerto_memory_stick);
    
    /* Conexiones iniciales */
    conectar_a_servidor ( socket_kernel_memory, ip_kernel_memory, puerto_kernek_memory);
    t_paquete * paquete_kernel_memory = crear_paquete (NEW_CPU);
    agregar_a_paquete (paquete_kernel_memory, id_cpu, strlen(id_cpu) + 1);
    enviar_paquete (paquete_kernel_memory, socket_kernel_memory);
    
    conectar_a_servidor ( socket_kernel_scheduler, ip_kernel_scheduler, puerto_kernek_scheduler);
    t_paquete * paquete_kernel_scheduler = crear_paquete (NEW_CPU);
    agregar_a_paquete (paquete_kernel_scheduler, id_cpu, strlen(id_cpu) + 1);
    enviar_paquete (paquete_kernel_scheduler, socket_kernel_scheduler);
    
    conectar_a_servidor ( socket_memory_stick, ip_memory_stick, puerto_memory_stick);
    t_paquete * paquete_memory_stick = crear_paquete (NEW_CPU);
    agregar_a_paquete (paquete_memory_stick, id_cpu, strlen(id_cpu) + 1);
    enviar_paquete (paquete_memory_stick, socket_memory_stick);
    
    log_info(logger, "CPU %s lista y esperando procesos del Kernel...\n", id_cpu);

    pthread_t hilo_ejecucion;
    pthread_create(&hilo_ejecucion, NULL, thread_ejecucion, NULL);
    pthread_join(hilo_ejecucion, NULL);
   
    return EXIT_SUCCESS;
}