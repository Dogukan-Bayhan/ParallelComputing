#include <stdio.h>
#include <omp.h>

#define NUM_THREADS 4
#define PAD 8                 
static long num_steps = 100000000;
double step;

// 1) False sharing 
void pi_false_sharing() {
    double sum[NUM_THREADS];
    double pi = 0.0;
    int nthreads = 1;
    step = 1.0 / (double)num_steps;

    omp_set_num_threads(NUM_THREADS);
    double t0 = omp_get_wtime();

    #pragma omp parallel
    {
        int id     = omp_get_thread_num();
        int nthrds = omp_get_num_threads();
        if (id == 0) nthreads = nthrds;

        double x, sum_local = 0.0;   
        long i;
        for (i = id; i < num_steps; i += nthrds) {
            x = (i + 0.5) * step;
            sum_local += 4.0 / (1.0 + x * x);
        }
        sum[id] = sum_local;
    }

    for (int i = 0; i < nthreads; ++i) 
        pi += sum[i] * step;

    double t1 = omp_get_wtime();
    printf("[1] False Sharing   : pi = %.10f | time = %.4f s\n", pi, t1 - t0);
}

void pi_padded() {
    double sum[NUM_THREADS][PAD] = {{0}};  
    double pi = 0.0;
    int nthreads = 1;
    step = 1.0 / (double)num_steps;

    omp_set_num_threads(NUM_THREADS);
    double t0 = omp_get_wtime();

    #pragma omp parallel
    {
        int id     = omp_get_thread_num();
        int nthrds = omp_get_num_threads();
        if (id == 0) nthreads = nthrds;

        double x, sum_local = 0.0;   
        long i;
        for (i = id; i < num_steps; i += nthrds) {
            x = (i + 0.5) * step;
            sum_local += 4.0 / (1.0 + x * x);
        }
        sum[id][0] = sum_local;      
    }

    for (int i = 0; i < nthreads; ++i)
        pi += sum[i][0] * step;

    double t1 = omp_get_wtime();
    printf("[2] Padded          : pi = %.10f | time = %.4f s\n", pi, t1 - t0);
}

void pi_critical() {
    double pi = 0.0;
    step = 1.0 / (double)num_steps;

    omp_set_num_threads(NUM_THREADS);
    double t0 = omp_get_wtime();

    #pragma omp parallel
    {
        int id     = omp_get_thread_num();
        int nthrds = omp_get_num_threads();
        (void)id; (void)nthrds;

        double x, sum_local = 0.0;
        for (long i = id; i < num_steps; i += nthrds) {
            x = (i + 0.5) * step;
            sum_local += 4.0 / (1.0 + x * x);
        }

        #pragma omp critical
        pi += sum_local * step;
    }

    double t1 = omp_get_wtime();
    printf("[3] Critical        : pi = %.10f | time = %.4f s\n", pi, t1 - t0);
}

void pi_atomic() {
    double pi = 0.0;
    step = 1.0 / (double)num_steps;

    omp_set_num_threads(NUM_THREADS);
    double t0 = omp_get_wtime();

    #pragma omp parallel
    {
        int id     = omp_get_thread_num();
        int nthrds = omp_get_num_threads();

        double x, sum_local = 0.0;
        for (long i = id; i < num_steps; i += nthrds) {
            x = (i + 0.5) * step;
            sum_local += 4.0 / (1.0 + x * x);
        }

        #pragma omp atomic
        pi += sum_local * step;
    }

    double t1 = omp_get_wtime();
    printf("[4] Atomic          : pi = %.10f | time = %.4f s\n", pi, t1 - t0);
}

int main() {
    printf("Comparing 4 Pi implementations (NUM_THREADS=%d)\n", NUM_THREADS);
    printf("------------------------------------------------\n");
    pi_false_sharing();
    pi_padded();
    pi_critical();
    pi_atomic();
    return 0;
}


