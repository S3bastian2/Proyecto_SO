#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>


//Aca coloco el nombre del espacio de memoria y el semaforo.
const char *SHM_NOMBRE = "/shm_nombre";
const char *SEM_ALIVE_P3 = "/sem_alive_p3";
const char *SEM_ALIVE_P4 = "/sem_alive_p4";
const char *SEM_EMPTY = "/sem_empty";
const char *SEM_MUTEX = "/sem_mutex";
const char *SEM_FIB_BLOCK = "/sem_fib_block";
const char *SEM_POW_BLOCK = "/sem_pow_block";
const char *SEM_P3 = "/sem_p3";
const char *SEM_P4 = "/sem_p4";
const char *SEM_P3_BLOCK = "/sem_p3_block";
const char *SEM_P4_BLOCK = "/sem_p4_block";
const char *PIPE_P1 = "/tmp/pipe_p1"; // Pipe para -3 a P1
const char *PIPE_P2 = "/tmp/pipe_p2"; // Pipe para -3 a P2

//Creo la estructura que va a guardar los valores para las funciones.
struct dato_compartido {
    int valor_generado;
    int last1_type;
    int last2_type;
    int last1_cons;
    int last2_cons;
    int flag_minus1;
    int flag_minus2;
} dato;

//Variable para guardar el valor de los semaforos.
int valor;

void generar_fibonacci(int N, int *a1_ptr, int *a2_ptr, struct dato_compartido *dt,
                      sem_t *empty, sem_t *mutex, sem_t *fib_block, sem_t *pow_block,
                      sem_t *p3) {
    int i = 1;
    int a1 = *a1_ptr;
    int a2 = *a2_ptr;
    while (i <= N) {
        sem_wait(empty);
        sem_wait(mutex);
        if (dt->last1_type == 1 && dt->last2_type == 1) {
            sem_post(mutex);
            sem_wait(fib_block);
            sem_wait(mutex);
        }
        bool break_other = (dt->last1_type == 2 && dt->last2_type == 2);
        dt->valor_generado = a1 + a2;
        a1 = a2;
        a2 = dt->valor_generado;
        dt->last2_type = dt->last1_type;
        dt->last1_type = 1; // Tipo Fib
        if (break_other) {
            sem_post(pow_block);
        }
        sem_post(mutex);
        sem_post(p3);
        i++;
    }
    *a1_ptr = a1;
    *a2_ptr = a2;
}

void generar_potencias(int N, int a3, struct dato_compartido *dt,
                      sem_t *empty, sem_t *mutex, sem_t *pow_block, sem_t *fib_block,
                      sem_t *p4) {
    int i = 1;
    while (i <= N) {
        sem_wait(empty);
        sem_wait(mutex);
        if (dt->last1_type == 2 && dt->last2_type == 2) {
            sem_post(mutex);
            sem_wait(pow_block);
            sem_wait(mutex);
        }
        bool break_other = (dt->last1_type == 1 && dt->last2_type == 1);
        dt->valor_generado = (int)pow(2, a3 + i - 1);
        dt->last2_type = dt->last1_type;
        dt->last1_type = 2; // Tipo Pow
        if (break_other) {
            sem_post(fib_block);
        }
        sem_post(mutex);
        sem_post(p4);
        i++;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Uso: %s N a1 a2 a3\n", argv[0]);
        exit(-1);
    }

    int N = atoi(argv[1]); // Cantidad de números a generar
    int a1 = atoi(argv[2]); // Primer término de Fibonacci
    int a2 = atoi(argv[3]); // Segundo término de Fibonacci
    int a3 = atoi(argv[4]); // Exponente inicial para potencias de 2

    // Validación de parámetros
    if (N <= 0 || a1 < 0 || a2 < 0 || a3 < 0) {
        printf("Parametros invalidos\n");
        exit(-1);
    }

    //Abrimos el espacio de memoria compartida.
    int descriptor = shm_open(SHM_NOMBRE, O_RDWR, 0666);
    if (descriptor == -1) {
        perror("Error abriendo el espacio de memoria compartida");
        exit(-1);
    }

    //Mapeamos el espacio de memoria.
    struct dato_compartido *dt = mmap(NULL, sizeof(dato), PROT_READ | PROT_WRITE, MAP_SHARED, descriptor, 0);
    if (dt == MAP_FAILED) {
        perror("Error mapeando el espacio de memoria");
        exit(-1);
    }

    //Verificamos si P3 y P4 están vivos usando sem_getvalue.
    sem_t *alive_p3 = sem_open(SEM_ALIVE_P3, 0);
    sem_t *alive_p4 = sem_open(SEM_ALIVE_P4, 0);
    if (alive_p3 == SEM_FAILED || alive_p4 == SEM_FAILED) {
        printf("P3 o P4 no están en ejecución\n");
        exit(-1);
    }
    int val_p3, val_p4;
    if (sem_getvalue(alive_p3, &val_p3) == -1 || sem_getvalue(alive_p4, &val_p4) == -1 ||
        val_p3 != 1 || val_p4 != 1) {
        printf("P3 o P4 no están en ejecución\n");
        exit(-1);
    }

    //Abrimos los otros semáforos.
    sem_t *empty = sem_open(SEM_EMPTY, 0);
    sem_t *mutex = sem_open(SEM_MUTEX, 0);
    sem_t *fib_block = sem_open(SEM_FIB_BLOCK, 0);
    sem_t *pow_block = sem_open(SEM_POW_BLOCK, 0);
    sem_t *p3 = sem_open(SEM_P3, 0);
    sem_t *p4 = sem_open(SEM_P4, 0);
    sem_t *p3_block = sem_open(SEM_P3_BLOCK, 0);
    sem_t *p4_block = sem_open(SEM_P4_BLOCK, 0);

    if (empty == SEM_FAILED || mutex == SEM_FAILED || fib_block == SEM_FAILED || pow_block == SEM_FAILED ||
        p3 == SEM_FAILED || p4 == SEM_FAILED || p3_block == SEM_FAILED || p4_block == SEM_FAILED) {
        perror("Error abriendo semáforos");
        exit(-1);
    }

    //Creo los pipes si no existen
    if (mkfifo(PIPE_P1, 0666) == -1 && errno != EEXIST) {
        perror("Error creando pipe_p1");
        exit(-1);
    }
    if (mkfifo(PIPE_P2, 0666) == -1 && errno != EEXIST) {
        perror("Error creando pipe_p2");
        exit(-1);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("Error en el fork");
        exit(-1);
    }

    if (pid == 0) { // Proceso hijo (P2)
        generar_potencias(N, a3, dt, empty, mutex, pow_block, fib_block, p4);
        // Enviar testigo -2
        sem_wait(empty);
        sem_wait(mutex);
        dt->valor_generado = -2; // Testigo P2
        sem_post(mutex);
        sem_post(p4);
        //Esperamos el -3 por pipe
        int pipe_fd = open(PIPE_P2, O_RDONLY);
        if (pipe_fd == -1) {
            perror("Error abriendo pipe_p2 para leer");
            exit(-1);
        }
        int token;
        if (read(pipe_fd, &token, sizeof(int)) != sizeof(int)) {
            perror("Error leyendo -3");
            exit(-1);
        }
        close(pipe_fd);
        printf("-3 P2 termina\n");
        fflush(stdout); // Asegurar que se imprima
        // Cerrar recursos en hijo
        munmap(dt, sizeof(dato));
        close(descriptor);
        sem_close(alive_p3);
        sem_close(alive_p4);
        sem_close(empty);
        sem_close(mutex);
        sem_close(fib_block);
        sem_close(pow_block);
        sem_close(p3);
        sem_close(p4);
        sem_close(p3_block);
        sem_close(p4_block);
        exit(0);
    } else { // Proceso padre (P1)
        generar_fibonacci(N, &a1, &a2, dt, empty, mutex, fib_block, pow_block, p3);
        // Enviar testigo -1
        sem_wait(empty);
        sem_wait(mutex);
        dt->valor_generado = -1; // Testigo P1
        sem_post(mutex);
        sem_post(p3);
        //Esperamos el -3 por pipe
        int pipe_fd = open(PIPE_P1, O_RDONLY);
        if (pipe_fd == -1) {
            perror("Error abriendo pipe_p1 para leer");
            exit(-1);
        }
        int token;
        if (read(pipe_fd, &token, sizeof(int)) != sizeof(int)) {
            perror("Error leyendo -3");
            exit(-1);
        }
        close(pipe_fd);
        printf("-3 P1 termina\n");
        fflush(stdout); // Asegurar que se imprima
        // Cerrar recursos en padre
        munmap(dt, sizeof(dato));
        close(descriptor);
        sem_close(alive_p3);
        sem_close(alive_p4);
        sem_close(empty);
        sem_close(mutex);
        sem_close(fib_block);
        sem_close(pow_block);
        sem_close(p3);
        sem_close(p4);
        sem_close(p3_block);
        sem_close(p4_block);
    }

    return 0;
}
