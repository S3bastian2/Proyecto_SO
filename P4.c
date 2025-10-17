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
        sem_t *sem4 = sem_open(SEM_NOMBRE2, 0);
        if (sem4 == SEM_FAILED) {
                perror("Ha ocurrido un problema al crear el semaforo.\n");
                exit(-1);
       	}
	//Generamos el cuerpo del proceso P4.
	sem_wait(sem4); //15. Reducimos el valor del semaforo de P2 y P4 de forma global.
	printf("Hola soy el proceso 4 y ahora leere los valores de la secuencia de potencias de 2.\n");
	printf("%d\n", dt -> valor_generado);
	sem_wait(sem4); //16. Detenemos el proceso P4.

	//Cerramos todo menos el espacio de memoria.
	munmap(dt, sizeof(dato));
        close(descriptor);
        sem_close(sem4);
        printf("El proceso 4 ha terminado.\n");
        return 0;
}
