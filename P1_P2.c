#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <errno.h>
#include <math.h>

//Aca voy a colocar el nombre del espacio de memoria y los nombres de los semaforos con nombre que voy a utilizar.
const char *SHM_NOMBRE = "/shm_nombre";
const char *SEM_NOMBRE = "/sem_nombre";
const char *SEM_NOMBRE2 = "/sem_nombre2";

struct dato_compartido {
    int valor_generado;
} dato;

int valor_sem;
int valor_sem2;

int main(int argc, char *argv[]) {
	//abro el espacio de memoria.
	int descriptor = shm_open(SHM_NOMBRE, O_RDWR, 0666);
	if (descriptor == -1) {
		perror("Se ha producido un error al crear el espacio de memoria.");
		exit(-1);
	}
	//Mapeamos el espacio de memoria.
	struct dato_compartido *dt = mmap(NULL, sizeof(dato), PROT_READ | PROT_WRITE, MAP_SHARED, descriptor, 0);
	if (dt == MAP_FAILED) {
		perror("Se ha producido un problema al mapear el espacio de memoria.");
		exit(-1);
	}
    	//Creamos el semaforo para P1.
	sem_t *sem = sem_open(SEM_NOMBRE, 0);
	if (sem == SEM_FAILED) {
		perror("Ha ocurrido un problema al crear el semaforo del proceso P1.");
		exit(-1);
	}
	//Creamos el semaforo para P2.
	sem_t *sem2 = sem_open(SEM_NOMBRE2, 0);
	if (sem2 == SEM_FAILED) {
		perror("Hubo un error al crear el semaforo para el proceso P2.");
		exit(-1);
	}
	//Defino el semaforo de carrera.
	sem_t *sem_carrera = sem_open("/sem_carrera", O_CREAT, 0666, 1);
	if (sem_carrera == SEM_FAILED) {
		printf("Ha ocurrido un error al crear el semaforo de carrera.\n");
		exit(-1);
	}

	//Defino los semaforos que me va a permitir dar el orden de escritura y lectura en la secuencia.
	sem_t *mutex = sem_open("/sem_mutex", 0);
	if (mutex == SEM_FAILED) {
		printf("A ocurrido un problema a la hora de crear el semaforo mutex.\n");
		exit(-1);
	}
	
	//Defino los semaforos que me va a permitir dar el orden de escritura y lectura en la secuencia.
	sem_t *mutex2 = sem_open("/sem_mutex2", 0);
	if (mutex2 == SEM_FAILED) {
		printf("A ocurrido un error al crear el semaforo mutex2");
		exit(-1);
	}

	//Defino la verificación para que P1 verique que tanto P3 y P4 estan en ejecución.
	if (sem_getvalue(sem, &valor_sem) == 1 && sem_getvalue(sem2, &valor_sem2) == 1) {
		printf("Los procesos P3 y P4 no se encuentra en ejecución.\n");
		munmap(dt, sizeof(dato));
		close(descriptor);
		sem_close(sem);
		sem_close(sem2);
		sem_close(mutex);
		sem_close(mutex2);
		sem_close(sem_carrera);

	} else {
		printf("Los procesos P3 y P4 estan en ejecución.\n");
		//Defino a los procesos P1 y P2.
		pid_t pid;
		if (pid == -1) {
			perror("Error al crear los procesos P1 y P2.\n");
			exit(-1);
		}
		if (pid == 0) {
			sem_wait(sem_carrera);
			printf("Hola soy el proceso P2, listo para ejecutar mi secuencia.\n");
			//Casteando los valores N y a3.
			int N = atoi(argv[1]);
			int exponente = atoi(argv[4]);
			double a3 = (double) exponente;
			int base = 2;
			double base_convertida = (double) base;
			int resultado;
			for (int i = 0; i <= N; i++) {
				double potencia = pow(base_convertida, a3);
				resultado = (int)potencia;
				a3 += 1;
				dt -> valor_generado = resultado; //Region critica.
				sem_post(sem2);
				sem_wait(mutex);
				sem_post(mutex2);
				sem_wait(mutex); //Paramos al productor
		}

		} else {
			sem_wait(sem_carrera);
			printf("Hola soy el proceso P1, listo para ejecutar mi secuencia.\n");
			//Enviando los valores N, a1 y a2.
			int N = atoi(argv[1]);
			int a1 = atoi(argv[2]);
			int a2 = atoi(argv[3]);
			int sgte_valor;
			//printf("Secuencia generada: %d, %d", a1, a2);
			for (int i = 0; i <= N; i++) {
				sgte_valor = a1 + a2; //Región critica.
				//printf(", %d", sgte_valor);
				dt -> valor_generado = sgte_valor; //Región critica.
				sem_post(sem);
				sem_wait(mutex2);
				a1 = a2;
				a2 = sgte_valor;
			}
			sem_post(sem_carrera);
			sem_wait(mutex2);
		}
	}

	//Cerramos todo.
	munmap(dt, sizeof(dato));
	close(descriptor);
	sem_close(sem);
	sem_close(sem2);
	sem_close(mutex);
	sem_close(mutex2);
	sem_close(sem_carrera);
	return 0;
}
