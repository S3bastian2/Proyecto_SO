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
const char *SEM_NOMBRE2 = "/sem_nombre2";

//Creo la estructura que va a guardar los valores para las fun>
struct dato_compartido {
        int valor_generado;
} dato;

//Variable para guardar el valor de los semaforos.
int valor;

int main(void) {
        //Creo el espacio de memoria.
        int descriptor = shm_open(SHM_NOMBRE, O_RDWR, 0666);
        if (descriptor == -1) {
                perror("Se ha producido un error al crear el espacio de memoria.\n");
                exit(-1);
        }
        //Mapeamos el espacio de memoria.
        struct dato_compartido *dt = mmap(NULL, sizeof(dato), PROT_READ | PROT_WRITE, MAP_SHARED, descriptor, 0);
        if (dt == MAP_FAILED) {
                perror("Se ha producido un problema al mapear el espacio de memoria.\n");
                exit(-1);
        }
        //Abrimos el semaforo para el proceso 4.
        sem_t *sem4 = sem_open(SEM_NOMBRE2, O_CREAT, 0666, 0);
        if (sem4 == SEM_FAILED) {
                perror("Ha ocurrido un problema al crear el semaforo.\n");
                exit(-1);
       	}

        sem_t *mutex2 = sem_open("/sem_mutex2", O_CREAT, 0666, 0);
	if (mutex2 == SEM_FAILED) {
		printf("Ha ocurrido un error al crear el semaforo mutex2");
		exit(-1);
	}

        //Defino el semaforo de carrera.
	sem_t *sem_carrera = sem_open("/sem_carrera", O_CREAT,  0666, 1);
	if (sem_carrera == SEM_FAILED) {
		printf("Ha ocurrido un error al crear el semaforo de carrera.");
		exit(-1);
	}

	//Generamos el cuerpo del proceso P4.
        while (1) {
	        sem_wait(sem4); 
	        printf("El proceso P4 esta en funcionamiento.\n");
                printf(", %d\n", dt -> valor_generado);
                sem_post(sem_carrera);
	        sem_wait(sem4);
                //sem_wait(sem4);
        }

	//Cerramos todo menos el espacio de memoria.
	munmap(dt, sizeof(dato));
        close(descriptor);
        sem_close(sem4);
        sem_close(mutex2);
        printf("El proceso 4 ha terminado.\n");
        return 0;
}
