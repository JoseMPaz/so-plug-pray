#include "gestion.h"

t_log * logger = NULL;
t_config * config = NULL;
int * recurso_swap = NULL;
int * recurso_kernel_scheduler = NULL;
t_list * recursos_memory_stick = NULL;
t_list * recursos_cpu = NULL;
t_list * instrucciones_proceso = NULL;  // Almacenar instrucciones por proceso
pthread_mutex_t mutex_instrucciones;

int main(int argc, char* argv[]) 
{
	int * socket_escucha = (int *) malloc (sizeof(int));
	char * log_level;
	char * puerto_escucha;
	pthread_t hilo_servidor;
	
	recursos_memory_stick = list_create ();
	recursos_cpu = list_create ();
	instrucciones_proceso = list_create();
	pthread_mutex_init(&mutex_instrucciones, NULL);
	
	saludar("kernel_memory");//sera eliminado
	config = iniciar_config ("./kernel_memory.config");
	
	log_level = config_get_string_value (config, "LOG_LEVEL");
	logger= iniciar_log (	"kernel_memory.log", "KERNEL_MEMORY", log_level_from_string ( log_level ) );
	log_info ( logger, "MODULO KERNEL_MEMORY HA INICIADO");
   
  puerto_escucha = config_get_string_value (config, "PUERTO_ESCUCHA");
  *socket_escucha = crear_socket ( SERVIDOR, NULL, puerto_escucha);
  log_info(logger, "Escuchando en puerto %s", puerto_escucha);
  
	pthread_create (&hilo_servidor, NULL, admitir_clientes, (void *)socket_escucha);
	pthread_detach(hilo_servidor);
	
	// Mantener el proceso principal ejecutándose
	while(1) {
		sleep(1);
	}

	t_list* mis_instrucciones = parsear_instrucciones("/home/utnso/scripts/prueba.txt");

	if(mis_instrucciones != NULL){
		log_info(logger, "Se leyeron %d instrucciones del archivo.", list_size(mis_instrucciones));
	}
  
	return 0;
}
