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
	
	getchar ();

	return 0;
}
