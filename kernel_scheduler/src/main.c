#include "gestion.h"

t_config * config = NULL;
t_log * logger = NULL;
t_list * recursos_io = NULL;
t_list * recursos_cpu = NULL;
int * recurso_kernel_memory = NULL;


int main(int argc, char* argv[]) 
{
	int * socket_escucha = (int *) malloc (sizeof(int));
	char * log_level;
	char * puerto_escucha;
	char * puerto_kernel_memory, * ip_kernel_memory;
	pthread_t hilo_servidor;
	
	recursos_io = list_create();
	recursos_cpu = list_create ();
	recurso_kernel_memory = (int *) malloc (sizeof(int));
	
	saludar("kernel_scheduler");//sera eliminado
    
	config = iniciar_config ("./kernel_scheduler.config");
	
	log_level = config_get_string_value (config, "LOG_LEVEL");
	logger = iniciar_log (	"kernel_scheduler.log", "KERNEL_SCHEDULER", log_level_from_string ( log_level ) );
	log_info ( logger, "MODULO KERNEL_SCHEDULER HA INICIADO");
	
	/* Inicio Cliente */
	puerto_kernel_memory = config_get_string_value (config, "PUERTO_KERNEL_MEMORY");
	ip_kernel_memory = config_get_string_value (config, "IP_KERNEL_MEMORY");
  *recurso_kernel_memory = crear_socket ( CLIENTE, ip_kernel_memory, puerto_kernel_memory);
  conectar_a_servidor ( *recurso_kernel_memory, ip_kernel_memory, puerto_kernel_memory);
  t_paquete * paquete = crear_paquete (NEW_KERNEL_SCHEDULER);
	enviar_paquete (paquete, *recurso_kernel_memory);
	/* Fin Cliente */
		
	/* Inicio Servidor Multihilos */
  puerto_escucha = config_get_string_value (config, "PUERTO_ESCUCHA");
  *socket_escucha = crear_socket ( SERVIDOR, NULL, puerto_escucha);
	
	pthread_create (&hilo_servidor, NULL, admitir_clientes, (void *)socket_escucha);
	pthread_detach(hilo_servidor);
	/* Fin Servidor Multihilos */
	getchar ();
	
	pthread_exit (NULL);//Para esperar que terminen de procesar los hilos
									
	return 0;
}


