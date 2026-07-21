#include "gestion.h"

char* pid_fin_global = NULL;

t_list* parsear_instrucciones(char* path_archivo){
	t_list* lista_instrucciones = list_create();

	FILE* archivo = fopen(path_archivo,"r");

	if(archivo == NULL){
		printf("Error: No se pudo abrir el archivo %s\n", path_archivo);
		return lista_instrucciones;
	}

	char buffer_linea[256];

	while (fgets(buffer_linea, sizeof(buffer_linea),archivo)!= NULL){
		char* instruccion = string_duplicate(buffer_linea);
		string_trim(&instruccion);
		if(string_length(instruccion) > 0){
			list_add(lista_instrucciones, instruccion);
		} else{
			free(instruccion);
		}
	}
	fclose(archivo);
	return lista_instrucciones;
}

void * admitir_clientes (void * socket_escucha)
{
	int socket_temporal;
	int socket = *(int*) socket_escucha;
	free(socket_escucha);  // liberar memoria
	
	while(true)
	{
   	pthread_t hilo_de_atencion; //Por cada solicitud de atencion genera un hilo nuevo  	
    	
   	socket_temporal = accept (socket, NULL, NULL);//Se queda aca esperando peticiones, ya que accept es bloqueante
    	
   	if (socket_temporal >= 0)
   	{
   		//La funcion dentro del hilo_de_atencion debe liberar esta peticion de memoria
   		int * socket_de_atencion = (int *) malloc (sizeof(int)); 
    		
   		*socket_de_atencion = socket_temporal;
    		
   		pthread_create (&hilo_de_atencion, NULL, admitir, (void *) socket_de_atencion);
   		pthread_detach (hilo_de_atencion);
   	}   	
  }
  pthread_exit(NULL);

	return NULL;

}

bool _es_el_pid(void* elemento)
{
	return strcmp((char*) elemento, pid_fin_global) == 0;
}
				
void * admitir (void * socket_de_atencion)
{
	t_recurso * recurso;
	char * id ;
	int socket = *(int*) socket_de_atencion;
	free(socket_de_atencion);  // liberar memoria
	printf ("%d\n", socket);//sera eliminado
	t_list * parametros_new_io = NULL;
	t_list * parametros_new_cpu = NULL;
	
	while(1)
	{
	t_operacion operacion = recibir_operacion(socket);
    
	switch (operacion) //Selecciona el tipo de operacion
  {
		case NEW_IO: //Se encolar el socket io en la lista de ios
			parametros_new_io = recibir_carga_util (socket); //Recibe tipo de io
			
			recurso = (t_recurso *) malloc (sizeof(t_recurso));
			id = list_get (parametros_new_io, 0/*Posicion de tipo de io*/);
			recurso->id = strdup ( id );
			recurso->socket = socket;
			recurso->disponible = true;
			
			
			pthread_mutex_lock(&mutex_io);

			list_add(recursos_io, (void*) recurso);

			pthread_mutex_unlock(&mutex_io);
			
			log_info ( logger, "NUEVO IO CONECTADO A KERNEL_SCHEDULER");

			t_paquete* paquete_sleep = crear_paquete(IO_SLEEP);

			char* pid = "1";
			char* tiempo = "5";

			agregar_a_paquete(paquete_sleep, pid, strlen(pid) + 1);
			agregar_a_paquete(paquete_sleep, tiempo, strlen(tiempo) + 1);

			char* pid_block = strdup(pid);

			pthread_mutex_lock(&mutex_block);

			list_add(cola_block, pid_block);

			pthread_mutex_unlock(&mutex_block);

			log_info(logger,
				"PID %s agregado a BLOCK",
				pid_block
			);

			recurso->disponible = false;

			enviar_paquete(paquete_sleep, socket);

			destruir_paquete(paquete_sleep);

			list_destroy_and_destroy_elements (parametros_new_io, free);
    	
			break;


		case NEW_CPU://Se encolar el socket cpu en la lista de cpus
			parametros_new_cpu = recibir_carga_util (socket); //Recibe identificador de cpu
			if (parametros_new_cpu == NULL || list_size(parametros_new_cpu) == 0) 
			{
    		log_error(logger, "Error: parametros_new_cpu inválido");
    		return NULL;
			}
			recurso = (t_recurso *) malloc (sizeof(t_recurso));

			id = list_get (parametros_new_cpu, 0);
			recurso->id = strdup(id);
			recurso->socket = socket;
			recurso->disponible = true;
			
			pthread_mutex_lock(&mutex_cpu);
				list_add(recursos_cpu, (void*) recurso);
			pthread_mutex_unlock(&mutex_cpu);
			
			signal (&sem_ready);// Posibilita a un pcb en redy pasar a exec
			
			log_info ( logger, "NUEVO CPU CONECTADO A KERNEL_SCHEDULER");
			list_destroy(parametros_new_cpu);
			break;

		case IO_FINISHED:

			{
				t_list* parametros_fin = recibir_carga_util(socket);

				char* pid_fin = list_get(parametros_fin, 0);

				log_info(logger,
					"IO finalizada para PID: %s",
					pid_fin
				);

				

				pthread_mutex_lock(&mutex_block);

				char* pid_bloqueado = list_remove_by_condition(
					cola_block,
					_es_el_pid
				);

				pthread_mutex_unlock(&mutex_block);

				if(pid_bloqueado != NULL)
				{

					recurso->disponible = true;

					log_info(logger,
						"PID %s removido de BLOCK",
						pid_bloqueado
					);

					free(pid_bloqueado);

					pthread_mutex_lock(&mutex_ready);

					list_add(cola_ready, strdup(pid_fin));

					pthread_mutex_unlock(&mutex_ready);

					sem_post(&sem_ready);

					log_info(logger,
						"PID %s agregado a READY",
						pid_fin
					);
				}

					list_destroy_and_destroy_elements(parametros_fin, free);

					break;
			}

		case OPERACION_DESCONOCIDA:

			log_warning(logger, "Cliente desconectado");

			close(socket);

			return NULL;
		default:
			; /*operación desconocida*/
	}		    
	}      			


	return NULL;
}

/*Si un CPU esta libre, permite que se transicione un PCB desde cola de Ready a la cola de Execute*/
void * fifo (void * arg)
{
	while(true)
	{
		wait (&sem_new_ready);//Bloqueado hasta que hayan procesos en ready
		wait (&sem_ready);//Bloqueado hasta que se conecte un nuevo cpu o un proceso libere un cpu
		
		pthread_mutex_lock(&mutex_ready);
			t_pcb * un_pcb = list_remove (cola_ready, 0);//Por ser FIFO saca a primer elemento de la cola de ready
		pthread_mutex_unlock(&mutex_ready);

		t_recurso * cpu_libre = buscar_cpu_libre ();

		if (cpu_libre != NULL)
		{
			printf ("ID Recurso cpu libre: %s\n", cpu_libre->id);
			
			// Cambiar estado a EXEC
			un_pcb->estado = EXEC;
			
			pthread_mutex_lock (&mutex_exec);
				list_add (cola_exec, (void *) un_pcb);
			pthread_mutex_unlock (&mutex_exec);
			
			/********************** LOG 05 Obligatorio **********************/
  			log_info (logger, "## (%u) Pasa del estado %s al estado %s", un_pcb->pid, "READY", "EXEC");	
  			
  			// Enviar PCB al CPU para que lo ejecute
  			t_paquete* paquete_pcb = crear_paquete(EXECUTE_PROCESS);
  			agregar_a_paquete(paquete_pcb, un_pcb, sizeof(t_pcb));
  			
  			// Enviar también las instrucciones
  			if (instrucciones_padre != NULL && list_size (instrucciones_padre) > 0) {
  				int num_instrucciones = list_size(instrucciones_padre);
  				agregar_a_paquete(paquete_pcb, &num_instrucciones, sizeof(int));
  				
  				for (int i = 0; i < num_instrucciones; i++) {
  					char* instr = (char*) list_get(instrucciones_padre, i);
  					agregar_a_paquete(paquete_pcb, instr, strlen(instr) + 1);
  				}
  			}
  			
  			// Enviar al CPU a través del socket
  			enviar_paquete(paquete_pcb, cpu_libre->socket);
  			destruir_paquete(paquete_pcb);
  			
  			log_info(logger, "## PCB enviado a CPU %s para ejecución", cpu_libre->id);
		}	
	}

	return NULL;
}

void * round_robin (void * arg) 
{
	while(true)
	{
		wait (&sem_new_ready);//Bloqueado hasta que hayan procesos en ready
		wait (&sem_ready);//Bloqueado hasta que se conecte un nuevo cpu o un proceso libere un cpu
		
		pthread_mutex_lock(&mutex_ready);
			if (list_size(cola_ready) == 0) {
				pthread_mutex_unlock(&mutex_ready);
				continue;
			}
			t_pcb * un_pcb = list_remove (cola_ready, 0);//Por ser RR saca a primer elemento de la cola de ready
		pthread_mutex_unlock(&mutex_ready);

		t_recurso * cpu_libre = buscar_cpu_libre ();

		if (cpu_libre != NULL)
		{
			// Cambiar estado a EXEC
			un_pcb->estado = EXEC;
			
			pthread_mutex_lock (&mutex_exec);
				list_add (cola_exec, (void *) un_pcb);
			pthread_mutex_unlock (&mutex_exec);
			
			/********************** LOG Obligatorio **********************/
  			log_info (logger, "## (%u) Pasa del estado %s al estado %s", un_pcb->pid, "READY", "EXEC");	
  			
  			// Enviar PCB al CPU para que lo ejecute
  			t_paquete* paquete_pcb = crear_paquete(EXECUTE_PROCESS);
  			agregar_a_paquete(paquete_pcb, un_pcb, sizeof(t_pcb));
  			
  			// Enviar también las instrucciones
  			if (instrucciones_padre != NULL && list_size(instrucciones_padre) > 0) {
  				int num_instrucciones = list_size(instrucciones_padre);
  				agregar_a_paquete(paquete_pcb, &num_instrucciones, sizeof(int));
  				
  				for (int i = 0; i < num_instrucciones; i++) {
  					char* instr = (char*) list_get(instrucciones_padre, i);
  					agregar_a_paquete(paquete_pcb, instr, strlen(instr) + 1);
  				}
  			}
  			
  			// Enviar al CPU a través del socket
  			enviar_paquete(paquete_pcb, cpu_libre->socket);
  			destruir_paquete(paquete_pcb);
  			
  			log_info(logger, "## Proceso %u enviado a CPU %s con quantum de %dms", un_pcb->pid, cpu_libre->id, rr_quantum);
  			
  			// Esperar el quantum antes de volver a ejecutar
  			usleep(rr_quantum * 1000);
  			
  			// Después del quantum, mover de vuelta a READY si sigue en EXEC
  			pthread_mutex_lock (&mutex_exec);
  				// Buscar el PCB en cola_exec comparando PIDs
  				bool encontrado = false;
  				for (int i = 0; i < list_size(cola_exec); i++) {
  					t_pcb* pcb_actual = (t_pcb*) list_get(cola_exec, i);
  					if (pcb_actual->pid == un_pcb->pid) {
  						list_remove(cola_exec, i);
  						encontrado = true;
  						break;
  					}
  				}
  				pthread_mutex_unlock (&mutex_exec);
  				
  				if (encontrado) {
  					// Mover de vuelta a READY
  					un_pcb->estado = READY;
  					pthread_mutex_lock(&mutex_ready);
  						list_add(cola_ready, (void*) un_pcb);
  					pthread_mutex_unlock(&mutex_ready);
  					
  					/********************** LOG Obligatorio **********************/
  					log_info (logger, "## (%u) Pasa del estado %s al estado %s (quantum agotado)", un_pcb->pid, "EXEC", "READY");
  					
  					// Señalar que hay un CPU disponible
  					signal(&sem_ready);
  					// Señalar que hay procesos en ready
  					signal(&sem_new_ready);
  				} else {
  					log_info(logger, "## Proceso %u terminó su ejecución antes de agotar el quantum", un_pcb->pid);
  				}
		}	
	}

	return NULL;
}


void * colas_multinivel (void * arg)
{


	return NULL;
}

t_pcb * crear_pcb (uint32_t pid, uint32_t prioridad)
{
	t_pcb * new_pcb = (t_pcb *) malloc (sizeof (t_pcb));
	
	new_pcb->pid= pid;
	new_pcb->prioridad = prioridad;
	new_pcb->pc = 0;
	new_pcb->estado = NEW;
	(new_pcb->contexto).ax = 0;
	(new_pcb->contexto).bx = 0;
	(new_pcb->contexto).cx = 0;
	(new_pcb->contexto).dx = 0;
	(new_pcb->contexto).eax = 0;
	(new_pcb->contexto).ebx = 0;
	(new_pcb->contexto).ecx = 0;
	(new_pcb->contexto).edx = 0;
	(new_pcb->contexto).si = 0;
	(new_pcb->contexto).di = 0;
	
	return new_pcb;
}


void * long_term_scheduler (void * arg)
{
	while(true)
	{
		wait (&sem_new);//Bloqueado hasta que se agregue un proceso a cola_new

		//Retira el primer pcb de new y lo mantiene con una variable auxiliar un_pcb
		pthread_mutex_lock(&mutex_ready);
			t_pcb * un_pcb = list_remove (cola_new, 0/*Primer posicion de new*/);
		pthread_mutex_unlock(&mutex_ready);
		
		//Agrega el pcb de la variable auxiliar a cola_ready
		pthread_mutex_lock (&mutex_ready);
			list_add (cola_ready, (void *) un_pcb);
		pthread_mutex_unlock (&mutex_ready);
		/********************** LOG 05 Obligatorio **********************/
  		log_info (logger, "## (%u) Pasa del estado %s al estado %s", un_pcb->pid, "NEW", "READY");	
  		signal (&sem_new_ready);
	}

	return NULL;
}

void * medium_term_scheduler (void * arg)
{

	return NULL;
}

bool esta_disponible(void * elemento)
{
	t_recurso * recurso = elemento;

	return recurso->disponible;
}

t_recurso * buscar_cpu_libre(void)
{
	pthread_mutex_lock(&mutex_cpu);

	t_recurso * cpu_libre = list_find(
		recursos_cpu,
		esta_disponible
	);

	if(cpu_libre != NULL)
	{
		cpu_libre->disponible = false;
	}

	pthread_mutex_unlock(&mutex_cpu);

	return cpu_libre;
}
