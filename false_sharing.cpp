#include <omp.h>
#include <iostream>
using namespace std;

// False Sharing, birden fazla thread'in farklı değişkenlere 
// erişmesine rağmen bu değişikenlerin aynı cache line içinde
// yer alması sonucu ortaya çıkan performans kaybıdır.

// CPU, cache tutarlılığını (coherency) korumak için MESI protokolünü
// kullanır. Eğer bir thread, cache line'daki herhangi bir byte'ı değiştirirse
// diğer çekirdeklerdeki o satır geçersiz (invalid) hale gelir.
// Bu durumda CPU'lar sürekli cache invalidation - reload döngüsüne girer.

// Sonuç:
// Çok fazla bus trafiği,
// CPU beklemeleri,
// Paralellikten beklenen hız kazancı kaybolur.

// https://en.wikipedia.org/wiki/False_sharing


// Burada alignas kullanımı bu structı tam olarak bir 
// cache line koymasını söyler ve eğer kullanmazsak
// bu değişken iki cache sarkabilir ki bu zaten bizim istemediğimiz
// bir şey.
// 64 byte hizalama (cache line boyutu)
struct alignas(64) PaddedInt {  
    int value;
    // toplam 64 byte = 1 cache line
    char pad[60];               
};


// Burdaki main kodu derleyip çalıştırarak arasındaki
// zaman farkını çok net bir şekilde görebilirsiniz.
int main() {
    int a[2] = {0};

    double start = omp_get_wtime();
    #pragma omp parallel num_threads(2)
    {
        int id = omp_get_thread_num();
        for (int i = 0; i < 100000000; i++) {
            a[id]++;
        }
    }
    double end = omp_get_wtime();

    printf("False sharing total time: %f\n", end - start);

    // Burada int yerine kendi structımız olan PaddedInt
    // değişkeni üzerinden döngü yazdık, ve bu sayede yukardaki
    // örenkte a[1] değiştiğinde a[0] da cache invalidation sorunu
    // b üzerinde yaşanmadı.

    PaddedInt b[2];
    b[0].value = 0;
    b[1].value = 0;

    start = omp_get_wtime();

    #pragma omp parallel num_threads(2)
    {
        int id = omp_get_thread_num();
        for (int i = 0; i < 100000000; i++) {
            b[id].value++;
        }
    }
    end = omp_get_wtime();

    printf("Padding total time: %f\n", end - start);


}