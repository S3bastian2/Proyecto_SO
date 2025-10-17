#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <errno.h>
#include <math.h>

//Aca coloco el nombre del espacio de memoria y el semaforo.
const char *SHM_NOMBRE = "/shm_nombre";
const char *SEM_NOMBRE = "/sem_nombre";
const char *SEM_NOMBRE2 = "/sem_nombre2";
const char *SEM_MUTEX = "/sem_mutex";

//Creo la estructura que va a guardar los valores para las funciones.
struct dato_compartido {
	int valor_generado;
} dato;

//Variable para guardar el valor de los semaforos.
int valor;

//Defino la función de fibonacci.
int fibonacci(int n, int a1, int a2) {
	int sgte_valor;
	printf("Secuencia generada: %d, %d", a1, a2);
	if (n == 1) {
		return a1;
	}
	if (n == 2) {
		return a2;
	}
	for (int i = 3; i <= n; i++) {
		sgte_valor = a1 + a2;
		printf(", %d", sgte_valor);
		a1 = a2;
		a2 = sgte_valor;
	}
	printf("\n");
	return sgte_valor;
}

//defino la función de potencias.
int potencias(int n, double a3) {
	int resultado;
	for (double i = 0; i <= n; i++) {
		double potencia = pow(i, a3);
		resultado = (int)potencia;
		printf(", %d", resultado);
	}
	printf("\n");
	return resultado;
}

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
	//Creamos el semaforo para P1.
	sem_t *sem = sem_open(SEM_NOMBRE, O_CREAT, 0666, 0);
	if (sem == SEM_FAILED) {
		perror("Ha ocurrido un problema al crear el semaforo del proceso P1.");
		exit(-1);
	}
	//Creamos el semaforo para P2.
	sem_t *sem2 = sem_open(SEM_NOMBRE2, O_CREAT, 0666, 0);
	if (sem2 == SEM_FAILED) {
		perror("Hubo un error al crear el semaforo para el proceso P2.");
		exit(-1);
	}
	//Creo el semaforo Mutex que va a proteger la variable compartida.
	sem_t *mutex = sem_open(SEM_MUTEX, O_CREAT, 0666, 1);
	if (mutex == SEM_FAILED) {
		perror("Ha ocurrido un error al crear el semaforo mutex.\n");
		exit(-1);
	}
	//Defino dos semaforos sin nombre para P1 y P2.
	sem_t sem_p1, sem_p2;
	if (sem_init(&sem_p1, 1, 0) == -1) {
		perror("No se ha creado correctamente sem_p1 sin nombre.\n");
		exit(-1);
	}
	if (sem_init(&sem_p2, 1, 0) == -1) {
		perror("No se ha creado correctamente sem_p2 sin nombre.\n");
		exit(-1);
	}

	//Creamos a los procesos P1 y P2
	if (fork() == 0) {
		sem_wait(&sem_p2);
		printf("Hola soy el proceso P2 y ahora mostrare los valores de las potencias.\n");
		int n;
		double a3;
		printf("Ingrese el valor total de numeros que quieres imprimir: \n");
		scanf("%d", &n);
		printf("Ingresa el exponente: \n");
		scanf("%lf", &a3);
		sem_wait(mutex);
		dt -> valor_generado = potencias(n, a3);
		sem_post(mutex);
		sem_post(sem2);
		sem_post(&sem_p1);
		return 0;
	} else {
		sem_wait(&sem_p1);
		printf("Hola soy el proceso P1 y ahora mostrare unos valores siguiendo la secuencia de fibonacci.\n");
		int n;
		int a1;
		int a2;
		printf("Ingrese el tamaño de la secuencia: \n");
		scanf("%d", &n);
		printf("Ingrese el primer valor de la secuencia: \n");
		scanf("%d", &a1);
		printf("Ingrese el segundo valor de la secuencia: \n");
		scanf("%d", &a2);
		sem_wait(mutex);
		dt -> valor_generado = fibonacci(n, a1, a2);
		sem_post(mutex);
		sem_post(sem);
		sem_post(&sem_p2);
		return 0;
	}
	//Cerramos todo.
	munmap(dt, sizeof(dato));
	close(descriptor);
	sem_close(sem);
	sem_close(sem2);
	printf("Los procesos P1 y P2 han terminado.\n");
	return 0;
}