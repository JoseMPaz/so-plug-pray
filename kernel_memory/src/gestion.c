#include "gestion.h"

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


void * admitir (void * socket_de_atencion)
{
	char * id;
	int socket = *(int*) socket_de_atencion;
	t_recurso * recurso;
	free (socket_de_atencion);  // liberar memoria
	t_list * parametros_new_kernel_scheduler = NULL;
	t_list * parametros_new_memory_stick = NULL;
	t_list * parametros_new_cpu = NULL;
	char ruta_absoluta_instrucciones[150];

	int operacion = recibir_operacion(socket);

	switch (operacion) //Selecciona el tipo de operacion
	{
		case NEW_SWAP: //Se encolar el socket io en la lista de ios
			if (recurso_swap == NULL)
			{
				recurso_swap = (int *) malloc (sizeof(int));
				*recurso_swap = socket;
				log_info ( logger, "NUEVO SWAP CONECTADO A KERNEL_MEMORY");
			}
			else
			{
				log_info ( logger, "UN SWAP INTENTO CONECTARSE A KERNEL_MEMORY");
				close (socket);
			}
			break;
		case NEW_KERNEL_SCHEDULER: //Se encolar el socket io en la lista de ios
			*socket_kernel_scheduler = socket;
			log_info ( logger, "## Nueva conexion: kernel Scheduler");
			parametros_new_kernel_scheduler = recibir_carga_util (socket);
			char * str_pid = strdup ( (char *) list_get (parametros_new_kernel_scheduler, 0) );
				
			printf ("Cantidad de argumentos: %d\n", list_size (parametros_new_kernel_scheduler));
			printf ("Kernel scheduler envio pid: %s\n", str_pid);
			strcpy (ruta_absoluta_instrucciones, scripts_basepath);
			strcat (ruta_absoluta_instrucciones, "/");
			strcat (ruta_absoluta_instrucciones, (char *) list_get (parametros_new_kernel_scheduler, 1));
			printf ("ruta absoluta: %s\n", ruta_absoluta_instrucciones);
				
			t_codigo * codigo = (t_codigo *) malloc (sizeof (t_codigo));
			codigo->str_pid = str_pid;
			codigo->instrucciones = leer_archivo_a_lista (ruta_absoluta_instrucciones);
			list_add (codigos, codigo);
				
			printf("PID: %s\n", codigo->str_pid);
				
			t_codigo * un_codigo = (t_codigo *) list_get (codigos, 0);

			for (int i = 0; i < list_size(un_codigo->instrucciones); i++)
				printf("Instruccion: %s\n", (char *) list_get(un_codigo->instrucciones, i));
    			
				
			//Creo la imagen a ese pid
			//Aviso que fue creado y cierro el socket
				
			list_destroy_and_destroy_elements (parametros_new_kernel_scheduler, free);
			t_paquete * paquete = crear_paquete (RESPUESTA_IMAGEN_PROCESO);
			agregar_a_paquete (paquete, "OK", strlen ("OK") + 1);
			enviar_paquete (paquete, socket);
				//atender_kernel_scheduler (*socket_kernel_scheduler);
				
			break;
		case NEW_MEMORY_STICK://Se encolar el socket cpu en la lista de cpus
			parametros_new_memory_stick = recibir_carga_util (socket); //Recibe identificador de cpu
			if (parametros_new_memory_stick == NULL || list_size(parametros_new_memory_stick) == 0) 
			{
    		log_error (logger, "Error: parametros_new_memory_stick inválido");
    		return NULL;
			}
			
			recurso = (t_recurso *) malloc (sizeof(t_recurso));

			id = list_get (parametros_new_memory_stick, 0);
			recurso->id= strdup(id);
			recurso->socket = socket;
			recurso->disponible = true;
			
			list_add ( recursos_memory_stick, (void *) recurso);//Se le debe agregar mutex
			
			log_info ( logger, "NUEVO MEMORY_STICK CONECTADO A KERNEL_MEMORY");
			list_destroy(parametros_new_memory_stick);
			break;
		case NEW_CPU://Se encolar el socket cpu en la lista de cpus
			parametros_new_cpu = recibir_carga_util (socket); //Recibe identificador de cpu
			if (parametros_new_cpu == NULL || list_size(parametros_new_cpu) == 0) 
			{
    		log_error (logger, "Error: parametros_new_memory_stick inválido");
    		return NULL;
			}
			
			recurso = (t_recurso *) malloc (sizeof(t_recurso));

			id = list_get (parametros_new_cpu, 0/*ID del CPU*/);
			recurso->id= strdup(id);
			recurso->socket = socket;
			recurso->disponible = true;
			
			list_add ( recursos_cpu, (void *) recurso);//Se le debe agregar mutex
			
			log_info ( logger, "NUEVO CPU CONECTADO A KERNEL_MEMORY");
			list_destroy(parametros_new_cpu);

			atender_cpu(socket);
			break;
		case OPERACION_DESCONOCIDA:
			fprintf (stderr, "Operación no registrada");
		default:
			; /*operación desconocida*/
	}		          			

	return NULL;
}

t_list* parsear_instrucciones(char* path_archivo){
	t_list* lista_instrucciones = list_create();

	FILE* archivo = fopen(path_archivo,"r");

	if(archivo == NULL){
		printf("Error: No se pudo abrir el archivo %s\n", path_archivo);
	}

	char buffer_linea[100];

	while (fgets(buffer_linea, sizeof(buffer_linea),archivo)!= NULL){
		char* instruccion = string_duplicate(buffer_linea);
		string_trim(&instruccion);
		if(string_length(buffer_linea) > 0){
			list_add(lista_instrucciones, instruccion);
		} else{
			free(instruccion);
		}
	}
 fclose(archivo);
 return lista_instrucciones;

}

void atender_kernel_scheduler (int socket)
{
	int operacion;
	t_list * parametros = NULL;
	t_paquete * paquete_memoria = NULL;

	while(true) //loop infinito para que se quede escuchando mensajes del cpu
		{
			operacion = recibir_operacion(socket); //recien se iguala operacion aca porque cpu puede mandar muchas operaciones, y si estaria afuera, procesaria siempre la misma
			switch(operacion)
			{
				case EXECUTE_PROCESS: { // Recibir instrucciones del proceso
					parametros = recibir_carga_util(socket);
					if (parametros != NULL && list_size(parametros) > 0) {
						// Primera posición es el nombre del archivo (para log)
						char* nombre_archivo = (char*) list_get(parametros, 0);
						log_info(logger, "KERNEL_MEMORY: Instrucciones recibidas del archivo %s", nombre_archivo);
						
						pthread_mutex_lock(&mutex_instrucciones);
						// Almacenar todas las instrucciones (índices 1 en adelante)
						for (int i = 1; i < list_size(parametros); i++) {
							char* instr = string_duplicate((char*) list_get(parametros, i));
							list_add(instrucciones_proceso, instr);
						}
						pthread_mutex_unlock(&mutex_instrucciones);
						
						log_info(logger, "KERNEL_MEMORY: Se almacenaron %d instrucciones", list_size(instrucciones_proceso));
					}
					list_destroy_and_destroy_elements(parametros, free);
					break;
				}
				case ESPACIO_LIBRE:
					parametros = recibir_carga_util(socket); // este y el list_destroy de abajo sonpara que en el proximo recibir_operacion no lea basura
					log_info(logger, "Pedido de espacio libre del scheduler");
					paquete_memoria = crear_paquete(R_ESPACIO);

					int espacio_libre = 1024; //MOCK
					agregar_a_paquete(paquete_memoria, &espacio_libre, sizeof(int));
					enviar_paquete(paquete_memoria, socket);
					destruir_paquete(paquete_memoria);
					list_destroy(parametros);
					break;
				default:
					close (socket);
					return;
			}
		}
}

void atender_cpu (int socket) //Devolver instrucciones y simular memoria
{
	int operacion;
	t_list * parametros = NULL;
	t_paquete * paquete_ok = NULL;

	while(true) //loop infinito para que se quede escuchando mensajes del cpu
		{
			operacion = recibir_operacion(socket); //recien se iguala operacion aca porque cpu puede mandar muchas operaciones, y si estaria afuera, procesaria siempre la misma
			switch(operacion)
			{
				case MOV_IN: { // Lectura: devolver instrucción o dato
					parametros = recibir_carga_util(socket);
					if (parametros != NULL && list_size(parametros) >= 2) {
						char* pid_str = (char*) list_get(parametros, 0);
						char* pc_str = (char*) list_get(parametros, 1);
						int pc = atoi(pc_str);
						
						paquete_ok = crear_paquete(RES_OK);
						
						// Si PC corresponde a una instrucción, devolverla
						pthread_mutex_lock(&mutex_instrucciones);
						if (pc >= 0 && pc < list_size(instrucciones_proceso)) {
							char* instruccion = (char*) list_get(instrucciones_proceso, pc);
							log_info(logger, "KERNEL_MEMORY: MOV_IN PID=%s PC=%d Instruccion=%s", pid_str, pc, instruccion);
							agregar_a_paquete(paquete_ok, instruccion, strlen(instruccion) + 1);
						} else {
							// Instrucción fuera de rango o dato en memoria
							char* respuesta = "EMPTY";
							agregar_a_paquete(paquete_ok, respuesta, strlen(respuesta) + 1);
						}
						pthread_mutex_unlock(&mutex_instrucciones);
						
						enviar_paquete(paquete_ok, socket);
						destruir_paquete(paquete_ok);
					}
					list_destroy_and_destroy_elements(parametros, free);
					break;
				}
				case MOV_OUT: { // Escritura: simular almacenamiento
					parametros = recibir_carga_util(socket);
					if (parametros != NULL && list_size(parametros) >= 2) {
						char* pid_str = (char*) list_get(parametros, 0);
						log_info(logger, "KERNEL_MEMORY: MOV_OUT PID=%s", pid_str);
					}
					paquete_ok = crear_paquete(RES_OK);
					agregar_a_paquete(paquete_ok, "OK", strlen("OK") + 1);
					enviar_paquete(paquete_ok, socket);
					destruir_paquete(paquete_ok);
					list_destroy_and_destroy_elements(parametros, free);
					break;
				}
				default:
					close (socket);
					return;
			}
		}
}


void crear_tabla_segmentos_proceso(uint32_t pid) {
    pthread_mutex_lock(&mutex_tablas_segmentos);

    t_tabla_segmentos* nueva_tabla = malloc(sizeof(t_tabla_segmentos));
    nueva_tabla->pid = pid;
    nueva_tabla->segmentos = list_create();

	list_add(tablas_segmentos_procesos, nueva_tabla);
    
    log_info(logger, "## Tabla de segmentos creada para el PID: %u", pid);

    pthread_mutex_unlock(&mutex_tablas_segmentos);
}

static uint32_t _pid_buscado;
static bool _comparar_pid(void* elemento) {
    t_tabla_segmentos* tabla = (t_tabla_segmentos*) elemento;
    return tabla->pid == _pid_buscado;
}

t_tabla_segmentos* buscar_tabla_segmentos(uint32_t pid) {
    pthread_mutex_lock(&mutex_tablas_segmentos);
    
    _pid_buscado = pid;
    t_tabla_segmentos* tabla = list_find(tablas_segmentos_procesos, _comparar_pid);
    
    pthread_mutex_unlock(&mutex_tablas_segmentos);
    return tabla;
}

static uint32_t _seg_id_buscado;
static bool _comparar_segmento_id(void* elemento) {
    t_segmento* seg = (t_segmento*) elemento;
    return seg->id_segmento == _seg_id_buscado;
}

t_segmento* buscar_segmento_en_proceso(t_tabla_segmentos* tabla, uint32_t id_segmento) {
    if (tabla == NULL) return NULL;
    
    _seg_id_buscado = id_segmento;
    return list_find(tabla->segmentos, _comparar_segmento_id);
}

void registrar_nuevo_segmento(uint32_t pid, uint32_t id_segmento, uint32_t base_fisica, uint32_t limite) {
    t_tabla_segmentos* tabla = buscar_tabla_segmentos(pid);
    if (tabla == NULL) {
        log_error(logger, "Error: No se encontró la tabla de segmentos para PID %u", pid);
        return;
    }

t_segmento* nuevo_segmento = malloc(sizeof(t_segmento));
    nuevo_segmento->id_segmento = id_segmento;
    nuevo_segmento->direccion_base = base_fisica;
    nuevo_segmento->limite = limite;

	pthread_mutex_lock(&mutex_tablas_segmentos);
    list_add(tabla->segmentos, nuevo_segmento);
    pthread_mutex_unlock(&mutex_tablas_segmentos);

log_info(logger, "## Creación de Segmento: PID: %u - Crear Segmento: %u - Tamaño: %u", 
             pid, id_segmento, limite);
}

void eliminar_segmento_de_proceso(uint32_t pid, uint32_t id_segmento) {
    t_tabla_segmentos* tabla = buscar_tabla_segmentos(pid);
    if (tabla == NULL) return;

    pthread_mutex_lock(&mutex_tablas_segmentos);
    
    _seg_id_buscado = id_segmento;
    t_segmento* seg_a_borrar = list_remove_by_condition(tabla->segmentos, _comparar_segmento_id);
    
    pthread_mutex_unlock(&mutex_tablas_segmentos);

    if (seg_a_borrar != NULL) {
        log_info(logger, "## PID: %u - Eliminación de Segmento ID: %u - Base Física: %u", 
                 pid, id_segmento, seg_a_borrar->direccion_base);
				
				 free(seg_a_borrar); 
    }
}
