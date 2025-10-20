#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <errno.h>

//Aca coloco el nombre del espacio de memoria y el semaforo.
const char *SHM_NOMBRE = "/shm_nombre";
const char *SEM_NOMBRE = "/sem_nombre";

//Creo la estructura que va a guardar los valores para las funciones.
struct dato_compartido {
        int valor_generado;
} dato;

//Variable para guardar el valor de los semaforos.
int valor;

int main(void) {
        //Creo el espacio de memoria.
	int descriptor = shm_open(SHM_NOMBRE, O_CREAT | O_RDWR, 0666);
	if (descriptor == -1) {
		perror("Se ha producido un error al crear el espacio de memoria.");
		exit(-1);
	}
	//Definimos el tamaño del espacio de memoria.
	int tamaño = ftruncate(descriptor, sizeof(dato));
	if (tamaño == -1) {
		perror("No se ha podido crear el tamaño adecuadamente.");
		exit(-1);
	}
	//Mapeamos el espacio de memoria.
	struct dato_compartido *dt = mmap(NULL, sizeof(dato), PROT_READ | PROT_WRITE, MAP_SHARED, descriptor, 0);
	if (dt == MAP_FAILED) {
		perror("Se ha producido un problema al mapear el espacio de memoria.");
		exit(-1);
	}
	//Abrimos el semaforo para el proceso 3.
    sem_t *sem3 = sem_open(SEM_NOMBRE, O_CREAT, 0666, 0);
    if (sem3 == SEM_FAILED) {
        perror("Ha ocurrido un problema al crear el semaforo");            
		exit(-1);
    }
	
	sem_t *mutex = sem_open("/sem_mutex", O_CREAT, 0666, 0);
	if (mutex == SEM_FAILED) {
		printf("A ocurrido un problema a la hora de crear el semaforo mutex.\n");
		exit(-1);
	}
	
	//Aquí definire el cuerpo de P3 y antes de que P3 escriba el proceso voy a mandarle la señal a P1 de que P3 esta activo.
	while(1) {
		sem_wait(sem3);
		printf("Proceso P3 en funcionamiento.\n");
		printf(", %d\n", dt -> valor_generado);
		sem_post(mutex);
		sem_wait(sem3);
		//sem_wait(sem3);
	}

	//Cerramos todo, menos el espacio de memoria.
	munmap(dt, sizeof(dato));
	close(descriptor);
	sem_close(sem3);
	printf("El proceso 3 ha terminado.\n");
	return 0;
}
