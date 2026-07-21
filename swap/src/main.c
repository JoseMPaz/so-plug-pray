#include <utils/hello.h>

t_config * config = NULL;
t_log * logger = NULL;

int main(int argc, char* argv[]) 
{
	int socket_kernel_memory; //Este socket se conecta al servidor kernel memory
	char * log_level;
	char * ip_kernel_memory, * puerto_kernek_memory;
	char * swap_file_path;
	int swap_file_size;
	int block_size;
	FILE * archivo_swap;
	
	saludar("swap");//Se eliminara
	
	config = iniciar_config ("./swap.config");
	
	log_level = config_get_string_value (config, "LOG_LEVEL");
	logger = iniciar_log ( "swap.log", "SWAP", log_level_from_string ( log_level ) );
	log_info ( logger, "MODULO SWAP HA INICIADO");
    
	ip_kernel_memory = config_get_string_value (config, "IP_KERNEL_MEMORY");
	puerto_kernek_memory = config_get_string_value (config, "PUERTO_KERNEL_MEMORY");
	swap_file_path = config_get_string_value(config, "SWAP_FILE_PATH");
	swap_file_size = config_get_int_value(config, "SWAP_FILE_SIZE");
	block_size = config_get_int_value(config, "BLOCK_SIZE");

	socket_kernel_memory = crear_socket ( CLIENTE, ip_kernel_memory, puerto_kernek_memory);
	conectar_a_servidor ( socket_kernel_memory, ip_kernel_memory, puerto_kernek_memory);
	log_info(logger, "## Conectado a Kernel Memory");

	t_paquete * paquete = crear_paquete(NEW_SWAP);
	agregar_a_paquete(paquete, &block_size, sizeof(int));
	agregar_a_paquete(paquete, &swap_file_size, sizeof(int));
	enviar_paquete(paquete, socket_kernel_memory);
	destruir_paquete(paquete);

	archivo_swap = fopen(swap_file_path, "r+");
	if (archivo_swap == NULL) {
    archivo_swap = fopen(swap_file_path, "w+");
    if (archivo_swap == NULL) {
        log_error(logger, "No se pudo crear el archivo de SWAP en %s", swap_file_path);
        exit(EXIT_FAILURE);
    }
    if (ftruncate(fileno(archivo_swap), swap_file_size) == -1) {
        log_error(logger, "No se pudo asignar el tamaño al archivo de SWAP");
        exit(EXIT_FAILURE);
    }
}

	// Loop principal de SWAP - esperar operaciones
	while(1) {
		t_operacion operacion = recibir_operacion(socket_kernel_memory);

		switch(operacion) {
			case ESCRIBIR_BLOQUE_SWAP: {
				t_list* params_write = recibir_carga_util(socket_kernel_memory);

				int numero_bloque;
				memcpy(&numero_bloque, list_get(params_write, 0), sizeof(int));
				char * contenido = (char *) list_get(params_write, 1);

				long posicion = (long) numero_bloque * block_size;

				fseek(archivo_swap, posicion, SEEK_SET);
				size_t escritos = fwrite(contenido, block_size, 1, archivo_swap);
				if (escritos != 1) {
					log_error(logger, "Error al escribir el bloque %d en SWAP", numero_bloque);
				}
				fflush(archivo_swap);

				log_info(logger, "## Escritura del bloque: %d", numero_bloque);

				t_paquete * respuesta = crear_paquete(RES_OK);
				enviar_paquete(respuesta, socket_kernel_memory);
				destruir_paquete(respuesta);

				list_destroy_and_destroy_elements(params_write, free);
				break;
			}
			
			case LEER_BLOQUE_SWAP: {
				t_list* params_read = recibir_carga_util(socket_kernel_memory);

				int numero_bloque;
				memcpy(&numero_bloque, list_get(params_read, 0), sizeof(int));

				long posicion = (long) numero_bloque * block_size;
				char * bytes_leidos = malloc(block_size);

				fseek(archivo_swap, posicion, SEEK_SET);
				size_t leidos = fread(bytes_leidos, block_size, 1, archivo_swap);
				if (leidos != 1) {
					log_error(logger, "Error al leer el bloque %d de SWAP", numero_bloque);
				}

				log_info(logger, "## Lectura del bloque: %d", numero_bloque);

				t_paquete * respuesta = crear_paquete(R_LECTURA_SWAP);
				agregar_a_paquete(respuesta, bytes_leidos, block_size);
				enviar_paquete(respuesta, socket_kernel_memory);
				destruir_paquete(respuesta);

				free(bytes_leidos);
				list_destroy_and_destroy_elements(params_read, free);
				break;
			}
			
			default:
				log_warning(logger, "SWAP: Operación desconocida recibida: %d", operacion);
				break;
		}
	}

	return 0;
}
