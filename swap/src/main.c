#include <utils/hello.h>

t_config * config = NULL;
t_log * logger = NULL;

int main(int argc, char* argv[]) 
{
	int socket_kernel_memory; //Este socket se conecta al servidor kernel memory
	char * log_level;
	char * ip_kernel_memory, * puerto_kernek_memory;
	
	saludar("swap");//Se eliminara
	
	config = iniciar_config ("./swap.config");
	
	log_level = config_get_string_value (config, "LOG_LEVEL");
	logger = iniciar_log ( "swap.log", "SWAP", log_level_from_string ( log_level ) );
	log_info ( logger, "MODULO SWAP HA INICIADO");
    
	ip_kernel_memory = config_get_string_value (config, "IP_KERNEL_MEMORY");
	puerto_kernek_memory = config_get_string_value (config, "PUERTO_KERNEL_MEMORY");

	socket_kernel_memory = crear_socket ( CLIENTE, ip_kernel_memory, puerto_kernek_memory);
	conectar_a_servidor ( socket_kernel_memory, ip_kernel_memory, puerto_kernek_memory);

	t_paquete * paquete = crear_paquete (NEW_SWAP);
	agregar_a_paquete (paquete, "HHDD", strlen("HHDD"));
	enviar_paquete (paquete, socket_kernel_memory);
	
	// Loop principal de SWAP - esperar operaciones
	while(1) {
		t_operacion operacion = recibir_operacion(socket_kernel_memory);

		switch(operacion) {
			case MOV_OUT: // Lectura desde SWAP (escritura de datos a SWAP)
				// Recibir bloque a escribir
				t_list* params_write = recibir_carga_util(socket_kernel_memory);
				log_info(logger, "SWAP: Operación de escritura recibida");
				list_destroy(params_write);
				break;
			
			case MOV_IN: // Escritura a SWAP (lectura de datos desde SWAP)
				// Recibir bloque a leer
				t_list* params_read = recibir_carga_util(socket_kernel_memory);
				log_info(logger, "SWAP: Operación de lectura recibida");
				list_destroy(params_read);
				break;
			
			default:
				log_warning(logger, "SWAP: Operación desconocida recibida: %d", operacion);
				break;
		}
	}

	return 0;
}
