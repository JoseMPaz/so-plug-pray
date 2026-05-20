#include "gestion.h"

t_log * logger = NULL;
t_config * config = NULL;
int * recurso_swap = NULL;
int * recurso_kernel_scheduler = NULL;
t_list * recursos_memory_stick = NULL;
t_list * recursos_cpu = NULL;

int main(int argc, char* argv[]) 
{
	int * socket_escucha = (int *) malloc (sizeof(int));
	char * log_level;
	char * puerto_escucha;
	pthread_t hilo_servidor;
	
	recursos_memory_stick = list_create ();
	recursos_cpu = list_create ();
	
	saludar("kernel_memory");//sera eliminado
	config = iniciar_config ("./kernel_memory.config");
	
	log_level = config_get_string_value (config, "LOG_LEVEL");
	logger= iniciar_log (	"kernel_memory.log", "KERNEL_MEMORY", log_level_from_string ( log_level ) );
	log_info ( logger, "MODULO KERNEL_MEMORY HA INICIADO");
   
  puerto_escucha = config_get_string_value (config, "PUERTO_ESCUCHA");
  *socket_escucha = crear_socket ( SERVIDOR, NULL, puerto_escucha);
  
	pthread_create (&hilo_servidor, NULL, admitir_clientes, (void *)socket_escucha);
	pthread_detach(hilo_servidor);
		
	pthread_exit (NULL);//Para esperar que terminen de procesar los hilos
  
	return 0;
}
