#include "gestion.h"

#define RUTA_CONFIGURACION 1
/*
Modulo Kernel Memory
Compilar:
./bin/kernel_memory ./kernel_memory.config
*/


t_log * logger = NULL;
t_config * config = NULL;
int * recurso_swap = NULL;
int * socket_kernel_scheduler = NULL;
t_list * recursos_memory_stick = NULL;
t_list * recursos_cpu = NULL;
t_list * instrucciones_proceso = NULL;  // Almacenar instrucciones por proceso
pthread_mutex_t mutex_instrucciones;

/*Variables de config*/
char * log_level;
long segment_max_size;
char * allocation_strategy;
long instruction_delay;
long compaction_delay;
char * scripts_basepath;
char * puerto_escucha;

void validar_cla (int argc, char * argv[]);
void obtener_config (void);

int main(int argc, char* argv[]) 
{
	int * socket_escucha = (int *) malloc (sizeof(int));
	pthread_t hilo_servidor;
	recursos_memory_stick = list_create ();
	recursos_cpu = list_create ();
	socket_kernel_scheduler = (int *) malloc (sizeof(int));
	instrucciones_proceso = list_create();
	pthread_mutex_init(&mutex_instrucciones, NULL);
	
	/*Valida los argumentos en la linea de comandos*/
	validar_cla (argc, argv);
	
	config = iniciar_config ( argv[RUTA_CONFIGURACION] );
	
	obtener_config ();
	
	logger = iniciar_log (	"kernel_memory.log", "KERNEL_MEMORY", log_level_from_string ( log_level ) );
	
	log_info ( logger, "## Inicio: Modulo kernel Memory");
   

	
	*socket_escucha = crear_socket ( SERVIDOR, NULL, puerto_escucha);
	
  
	pthread_create (&hilo_servidor, NULL, admitir_clientes, (void *)socket_escucha);
	pthread_join (hilo_servidor, NULL);


	/*t_list* mis_instrucciones = parsear_instrucciones("/home/utnso/scripts/prueba.txt");

	if(mis_instrucciones != NULL){
		log_info(logger, "Se leyeron %d instrucciones del archivo.", list_size(mis_instrucciones));
	}*/
	  
	return 0;
}

void validar_cla (int argc, char * argv[])
{
	/*Validar Argumentos en la Linea de Comandos*/
	if (argc != 1/*Por el ejecutable*/ + 1/*Por la ruta al archivo de configuracion*/)
	{
		fprintf (	stderr, 
					"%s\n%s\n", 
					"Error: debe ingresar 1 argumento junto al ejecutale", 
					"Ejemplo: ./bin/kernel_memory kernel_memory.config");
		exit (EXIT_FAILURE);
	}
	else
	{
		FILE * pf;	
		if ( (pf = fopen (argv[RUTA_CONFIGURACION] , "r")) == NULL)
		{
			fprintf (	stderr, 
						"%s\n", 
						"Error: Ruta archivo de configuracion no se pudo abrir");
			exit (EXIT_FAILURE);
		}		
		fclose (pf);
	}
	return;
}

void obtener_config (void)
{
	log_level = config_get_string_value (config, "LOG_LEVEL");
	segment_max_size = 	config_get_long_value (config, "SEGMENT_MAX_SIZE");
	allocation_strategy = config_get_string_value (config, "ALLOCATION_STRATEGY");
	instruction_delay = config_get_long_value (config, "INSTRUCTION_DELAY");
	compaction_delay = config_get_long_value (config, "COMPACTION_DELAY");
	scripts_basepath = config_get_string_value (config, "SCRIPTS_BASEPATH");
	puerto_escucha = config_get_string_value (config, "PUERTO_ESCUCHA");

	return;
}
