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
	int socket = *(int*) socket_de_atencion;
	free(socket_de_atencion);  // liberar memoria
	
	log_info(logger, "MEMORY_STICK: Nuevo cliente conectado, socket: %d", socket);
	
	// Loop infinito para procesar operaciones del cliente
	while(1) {
		int operacion = recibir_operacion(socket);
		
		switch (operacion) {
			case MOV_IN: { // Lectura desde memory_stick
				t_list* parametros = recibir_carga_util(socket);
				if (parametros == NULL || list_size(parametros) < 3) {
					log_error(logger, "MEMORY_STICK: Parámetros inválidos para MOV_IN");
					break;
				}
				
				char* direccion_str = (char*) list_get(parametros, 0);
				char* tamaño_str = (char*) list_get(parametros, 1);
				int direccion = atoi(direccion_str);
				int tamaño = atoi(tamaño_str);
				
				log_info(logger, "MEMORY_STICK: MOV_IN - Leyendo desde dirección %d, tamaño %d", direccion, tamaño);
				
				// Lectura de datos
				char* datos_leidos = (char*) malloc(tamaño);
				memcpy(datos_leidos, memory_stick + direccion, tamaño);
				
				// Enviar confirmación
				t_paquete* respuesta = crear_paquete(RES_OK);
				agregar_a_paquete(respuesta, datos_leidos, tamaño);
				enviar_paquete(respuesta, socket);
				
				free(datos_leidos);
				list_destroy(parametros);
				break;
			}
			
			case MOV_OUT: { // Escritura a memory_stick
				t_list* parametros = recibir_carga_util(socket);
				if (parametros == NULL || list_size(parametros) < 2) {
					log_error(logger, "MEMORY_STICK: Parámetros inválidos para MOV_OUT");
					break;
				}
				
				char* direccion_str = (char*) list_get(parametros, 0);
				char* datos = (char*) list_get(parametros, 1);
				int direccion = atoi(direccion_str);
				int tamaño = strlen(datos);
				
				log_info(logger, "MEMORY_STICK: MOV_OUT - Escribiendo en dirección %d, tamaño %d", direccion, tamaño);
				
				// Escritura de datos
				memcpy(memory_stick + direccion, datos, tamaño);
				
				// Enviar confirmación
				t_paquete* respuesta = crear_paquete(RES_OK);
				enviar_paquete(respuesta, socket);
				
				list_destroy(parametros);
				break;
			}
			
			default:
				log_warning(logger, "MEMORY_STICK: Operación desconocida recibida: %d", operacion);
				break;
		}
	}

	return NULL;
}
