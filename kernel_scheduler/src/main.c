/* 
Ejecucion del modulo con Argumentos en la Linea de Comandos
	./bin/kernel_scheduler <ruta archivo configuracion> <nombre archivo pseudocodigo inicial>
Ejemplo:
	./bin/kernel_scheduler kernel_scheduler.config padre
*/

/* Cantidad de LOG Obligatorios = 2*/
/* Cantidad de LOG Adicionales = 2*/

#include "gestion.h"
#define RUTA_CONFIGURACION 0
#define ARCHIVO_PSEUDOCODIGO 2

t_config * config = NULL;
t_log * logger = NULL;

t_list * recursos_io = NULL;
t_list * recursos_cpu = NULL;
pthread_mutex_t mutex_io;
pthread_mutex_t mutex_cpu;


t_list * cola_new = NULL;
pthread_mutex_t mutex_new;
t_list * cola_ready = NULL;
pthread_mutex_t mutex_ready;
t_list * cola_exec = NULL;
pthread_mutex_t mutex_exec;
t_list * cola_block = NULL;
pthread_mutex_t mutex_block;

pthread_mutex_t mutex_pid;
uint32_t pid = 0;

t_list * instrucciones_padre = NULL;

// Variables para Round Robin
pthread_mutex_t mutex_quantum;

sem_t sem_ready;
sem_t sem_new;
sem_t sem_new_ready;

int * socket_kernel_memory = NULL;

/*Variables de config*/
char * log_level;
char * algoritmo_de_planificacion;
char ** algoritmos_de_las_colas = NULL; //Vector de strings
int rr_quantum;
bool queue_preemption;
long suspension_timeout;
char * puerto_escucha;
char * ip_kernel_memory;
char * puerto_kernel_memory;

t_scheduler short_term_scheduler;//t_scheduler esta definido en hello.h

void validar_cla (int argc, char * argv[]);
void obtener_config (void);
void establecer_planificador_corto_plazo (char * algoritmo);


int main(int argc, char* argv[]) 
{
	int * socket_escucha = (int *) malloc (sizeof(int));
	pthread_t hilo_servidor;
	int pid_proceso_principal;	
	pthread_t hilo_planificador_corto_plazo;
	pthread_t hilo_planificador_largo_plazo;
	
	validar_cla (argc, argv);
	
	/*Se crean la listas de recursos*/
	recursos_io = list_create();
	recursos_cpu = list_create ();
	/*Se crean las listas de procesos*/
	cola_new = list_create ();
	cola_ready = list_create();
	cola_exec = list_create();
	cola_block = list_create();
	

	/*Se inicializan los mutex de las colas*/
	pthread_mutex_init (&mutex_new, NULL);
	pthread_mutex_init (&mutex_ready, NULL);
	pthread_mutex_init (&mutex_exec, NULL);
	pthread_mutex_init (&mutex_block, NULL);
	pthread_mutex_init (&mutex_io, NULL);
	pthread_mutex_init (&mutex_cpu, NULL);
	pthread_mutex_init (&mutex_pid, NULL);
	pthread_mutex_init(&mutex_quantum, NULL);	
		
	sem_init (	&sem_ready, 0/*semaforo compartido entre hilos de este modulo*/, 0/*valor inicial del semaforo*/);
	sem_init (	&sem_new, 0/*semaforo compartido entre hilos de este modulo*/, 0/*valor inicial del semaforo*/);
	sem_init (	&sem_new_ready, 0/*semaforo compartido entre hilos de este modulo*/, 0/*valor inicial del semaforo*/);
	
	socket_kernel_memory = (int *) malloc (sizeof(int));
	
	/*Obtiene la informacion del archivo de configuracion*/    
	config = iniciar_config ("./kernel_scheduler.config");
	
	obtener_config ();
		
	/*	Crea el log en el nivel indicado en el archivo de configuracion*/
	logger = iniciar_log (	"kernel_scheduler.log", "KERNEL_SCHEDULER", log_level_from_string ( log_level ) );
	
	/********************** LOG 15 ADICIONAL **********************/
	log_info ( logger, "## Inicio: Modulo kernel Scheduler");
	
	/*	scheduler es un puntero a funcion que dependiendo el algoritmo de planificacion definido en el archivo de configuracion
	asigna a una funcion este puntero a funcion, que posteriormente es ejecutado en su respectivo hilo.	*/
	establecer_planificador_corto_plazo (algoritmo_de_planificacion);
		
	/* Solicita a Kernel Memory que cree la imagen del proceso principal*/	
	socket_kernel_memory = (int *) malloc (sizeof(int));
  	*socket_kernel_memory = crear_socket ( CLIENTE, ip_kernel_memory, puerto_kernel_memory);
  	conectar_a_servidor ( *socket_kernel_memory, ip_kernel_memory, puerto_kernel_memory);
  	t_paquete * paquete = crear_paquete (NEW_KERNEL_SCHEDULER);
  	char * str_pid;
  	pthread_mutex_lock(&mutex_pid);
  		str_pid = uint32_to_string (pid);
  	pthread_mutex_unlock(&mutex_pid);
  	agregar_a_paquete (paquete, str_pid, strlen(str_pid) + 1);
  	printf ("Se enviara: %s\n", argv[ARCHIVO_PSEUDOCODIGO]);
  	agregar_a_paquete (paquete, argv[ARCHIVO_PSEUDOCODIGO], strlen(argv[ARCHIVO_PSEUDOCODIGO]) + 1);
	enviar_paquete (paquete, *socket_kernel_memory);
  	free (str_pid);
  	
  	/*Esperar respuesta del kernel memory*/
  	int respuesta_km = recibir_operacion(*socket_kernel_memory);
  	t_list * parametro_respuesta = NULL;
  	parametro_respuesta = recibir_carga_util (*socket_kernel_memory);
  	printf ("Respuesta del kernel memory: %s\n", (char *) list_get (parametro_respuesta, 0));
  	printf ("Operacion respuesta: %d\n", respuesta_km);
  	
  	
  	
  	
  	/*Crea el PCB del proceso inicial*/
  	
  	
  	
	
	// Cargar pseudocódigo del archivo->los pseudocodigos estan en kernel_memory
	/*char archivo_pseudocodigo[256];
	sprintf(archivo_pseudocodigo, "./pseudocodigos/%s", argv[ARCHIVO_PSEUDOCODIGO]);
	instrucciones_padre = parsear_instrucciones(archivo_pseudocodigo);
	log_info(logger, "## Se cargaron %d instrucciones del archivo %s", list_size(instrucciones_padre), argv[ARCHIVO_PSEUDOCODIGO]);*/
	
	// Enviar instrucciones a kernel_memory
	/*t_paquete * paq_instrucciones = crear_paquete(EXECUTE_PROCESS);
	agregar_a_paquete(paq_instrucciones, argv[ARCHIVO_PSEUDOCODIGO], strlen(argv[ARCHIVO_PSEUDOCODIGO]) + 1);
	for (int i = 0; i < list_size(instrucciones_padre); i++) {
		char* instr = (char*) list_get(instrucciones_padre, i);
		agregar_a_paquete(paq_instrucciones, instr, strlen(instr) + 1);
	}
	enviar_paquete(paq_instrucciones, *recurso_kernel_memory);
	destruir_paquete(paq_instrucciones);*/
	
	t_pcb * nuevo_pcb = NULL;
	pthread_mutex_lock(&mutex_pid);
  		pid_proceso_principal = pid;
  	pthread_mutex_unlock(&mutex_pid);
  	nuevo_pcb = crear_pcb (pid_proceso_principal, 0/*prioridad*/);
  	pthread_mutex_lock(&mutex_new);
  		list_add (cola_new, nuevo_pcb);
  	pthread_mutex_unlock(&mutex_new);
  	
  	
  	/********************** LOG 03 Obligatorio **********************/
  	log_info (logger, "## (%u) Se crea el proceso - Estado: NEW", nuevo_pcb->pid);
  	pthread_mutex_lock(&mutex_pid);
  		pid++;
  	pthread_mutex_unlock(&mutex_pid);
  	signal (&sem_new);//Habilita al planificador de largo plazo a transicionar el pcb de new a ready
  	
  	
  	
  	
  	
  	/*Inicia servidor multihilos para admitir IO y CPU*/
  	*socket_escucha = crear_socket ( SERVIDOR, NULL, puerto_escucha);
	pthread_create (&hilo_servidor, NULL, admitir_clientes, (void *)socket_escucha);
	
	/*Inicia el planificador de largo plazo*/
	pthread_create ( &hilo_planificador_largo_plazo, NULL, long_term_scheduler, NULL );
	
	/*Inicia el planificador de corto plazo*/
	pthread_create ( &hilo_planificador_corto_plazo,	NULL, short_term_scheduler, NULL );
	
	
	pthread_join (hilo_servidor, NULL);
	pthread_join (hilo_planificador_largo_plazo, NULL);
	pthread_join (hilo_planificador_corto_plazo, NULL);
	
	if (nuevo_pcb != NULL)
		free (nuevo_pcb);
		
	log_info (logger, "##Queue_preemption: %d\n", queue_preemption);
	log_info (logger, "##Algoritmo de planificacion: %s\n", algoritmo_de_planificacion);
	log_info (logger, "##Suspensio timeout: %ld\n", suspension_timeout);	
						
	return EXIT_SUCCESS;
}

void validar_cla (int argc, char * argv[])
{
	/*Validar Argumentos en la Linea de Comandos*/
	if (argc != 1/*Por el ejecutable*/ + 1/*Por archivo de configuracion*/+ 1/*Por archivo de pseudocodigos*/)
	{
		fprintf (	stderr, 
					"%s\n%s\n", 
					"Error: debe ingresar 2 argumentos junto al ejecutable", 
					"Ejemplo: ./bin/kernel_scheduler kernel_scheduler.config padre");
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
	log_level = config_get_string_value ( config, "LOG_LEVEL");
	algoritmo_de_planificacion = config_get_string_value( config, "PLANIFICATION_ALGORITHM");
	algoritmos_de_las_colas = 	config_get_array_value (config, "QUEUES_ALGORITHMS");
	rr_quantum = config_get_int_value ( config, "RR_QUANTUM");
	queue_preemption = (strcmp (config_get_string_value (config, "QUEUE_PREEMPTION"),"TRUE") == 0) ? true : false;
	suspension_timeout = config_get_long_value (config, "SUSPENSION_TIMEOUT");
	puerto_escucha = config_get_string_value (config, "PUERTO_ESCUCHA");
	ip_kernel_memory = config_get_string_value (config, "IP_KERNEL_MEMORY");
	puerto_kernel_memory = config_get_string_value (config, "PUERTO_KERNEL_MEMORY");

	return;
}

void establecer_planificador_corto_plazo (char * algoritmo)
{
	if (!strcmp (algoritmo, "FIFO") )
		short_term_scheduler = fifo;
	else if (!strcmp (algoritmo, "RR"))
		short_term_scheduler = round_robin;
	else if (!strcmp (algoritmo, "CMN"))
		short_term_scheduler =colas_multinivel;

	return;
}
