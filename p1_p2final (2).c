// p3.c - lector P3 (crea/shm y sems; notifica listo via SEM_READY)
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>

const char *SHM_NOMBRE = "/shm_nombre";
const char *SEM_READY   = "/sem_ready_all";

const char *SEM_ALIVE_P3 = "/sem_alive_p3";
const char *SEM_EMPTY = "/sem_empty";
const char *SEM_MUTEX = "/sem_mutex";
const char *SEM_FIB_BLOCK = "/sem_fib_block";
const char *SEM_POW_BLOCK = "/sem_pow_block";
const char *SEM_P3 = "/sem_p3";
const char *SEM_P4 = "/sem_p4";
const char *SEM_P3_BLOCK = "/sem_p3_block";
const char *SEM_P4_BLOCK = "/sem_p4_block";
const char *SEM_P3_DONE = "/sem_p3_done";
const char *SEM_P4_DONE = "/sem_p4_done";

const char *PIPE_P1 = "/tmp/pipe_p1";
const char *PIPE_P2 = "/tmp/pipe_p2";

struct dato_compartido {
    int valor_generado;
    int last1_type;
    int last2_type;
    int last1_cons;
    int last2_cons;
    int flag_minus1;
    int flag_minus2;
};

static sem_t *create_or_open_sem(const char *name, int init_val) {
    sem_t *s = sem_open(name, O_CREAT, 0666, init_val);
    if (s == SEM_FAILED) { perror("sem_open"); exit(EXIT_FAILURE); }
    return s;
}

int main(void) {
    printf("Esperando por P1\n");

    int created_shm = 0;
    int fd = shm_open(SHM_NOMBRE, O_CREAT | O_RDWR, 0666);
    if (fd == -1) { perror("shm_open p3"); exit(EXIT_FAILURE); }

    struct stat st;
    if (fstat(fd, &st) == -1 || st.st_size == 0) {
        if (ftruncate(fd, sizeof(struct dato_compartido)) == -1) { perror("ftruncate p3"); close(fd); exit(EXIT_FAILURE); }
        created_shm = 1;
    }

    struct dato_compartido *dt = mmap(NULL, sizeof(*dt), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (dt == MAP_FAILED) { perror("mmap p3"); close(fd); exit(EXIT_FAILURE); }
    if (created_shm) memset(dt, 0, sizeof(*dt));

    // Crear semaforos y marcar vivo
    sem_t *alive_p3 = create_or_open_sem(SEM_ALIVE_P3, 1);

    sem_t *empty = create_or_open_sem(SEM_EMPTY, 1);
    sem_t *mutex = create_or_open_sem(SEM_MUTEX, 1);
    sem_t *fib_block = create_or_open_sem(SEM_FIB_BLOCK, 0);
    sem_t *pow_block = create_or_open_sem(SEM_POW_BLOCK, 0);
    sem_t *p3_block = create_or_open_sem(SEM_P3_BLOCK, 0);
    sem_t *p4_block = create_or_open_sem(SEM_P4_BLOCK, 0);
    sem_t *sem3 = create_or_open_sem(SEM_P3, 0);
    sem_t *sem4 = create_or_open_sem(SEM_P4, 0);
    sem_t *p3_done = create_or_open_sem(SEM_P3_DONE, 0);
    sem_t *p4_done = create_or_open_sem(SEM_P4_DONE, 0);

    // ensure pipes exist
    mkfifo(PIPE_P1, 0666); mkfifo(PIPE_P2, 0666);

    // BARRERA: indicar que P3 está listo
    sem_t *ready = create_or_open_sem(SEM_READY, 0);
    if (sem_post(ready) == -1) { perror("sem_post ready (p3)"); }

    // ---- LECTURA: P3 lee hasta recibir -1 ----
    int received_minus1 = 0;
    while (!received_minus1) {
        sem_wait(sem3);
        sem_wait(mutex);
        if (dt->last1_cons == 3 && dt->last2_cons == 3) {
            sem_post(mutex);
            sem_wait(p3_block);
            sem_wait(mutex);
        }
        int val = dt->valor_generado;
        if (val > 0) {
            int old1 = dt->last1_cons, old2 = dt->last2_cons;
            dt->last2_cons = old1; dt->last1_cons = 3;
            if (old1 == old2 && old1 == 4) sem_post(p4_block);
            printf("%d\n", val);
        } else if (val == -1) {
            dt->flag_minus1 = 1;
            received_minus1 = 1;
            sem_post(p3_done);
        } else if (val == -2) {
            dt->flag_minus2 = 1;
        }
        sem_post(mutex);
        sem_post(empty);
    }

    // esperar a que P4 confirme que recibió -2
    sem_wait(p4_done);

    // enviar -3 a P1 por pipe
    int wfd = open(PIPE_P1, O_WRONLY);
    if (wfd != -1) { int t = -3; write(wfd, &t, sizeof(t)); close(wfd); }
    printf("P3: -3 enviado, P3 termina\n");

    // cleanup: cerrar (no unlink)
    munmap(dt, sizeof(*dt));
    close(fd);
    sem_close(alive_p3);
    sem_close(empty); sem_close(mutex);
    sem_close(fib_block); sem_close(pow_block);
    sem_close(p3_block); sem_close(p4_block);
    sem_close(sem3); sem_close(sem4);
    sem_close(p3_done); sem_close(p4_done);
    sem_close(ready);
    return 0;
}

