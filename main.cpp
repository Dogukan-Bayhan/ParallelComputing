#include <omp.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <windows.h>
#include <thread>


using namespace std;
using namespace chrono;

// OpenMP de #pragma ... altında kalan ilk blok bu pragmada
// sağlanan özelliklere sahip olur. Bu bloktan sonra kod yine
// normal c++ derleyicisindeki özellikler ile devam eder

// g++ -fopenmp main.cpp -o main kodu bu şekilde derlemelisiniz
// -fopenmp olmadan OpenMP özellikleri derlenmez ve hata verir

int main() {

    // ==========================================================
    //  ÖRNEK 1: OpenMP Parallel Temelleri
    // ==========================================================

    // Bu kod altındaki Hello toplam thread sayısı kadar
    // çağrılır. Örneğin benim bilgisayarım 16 core yani 32 
    // thread aynı anda çalışabiliyor. Bu yüzden 32 kere Hello
    // çağrılır ama World yazısı sadece bir kere çağrılır çünkü o
    // artık farklı bir blokta yer alır.
    #pragma omp parallel
    printf("Hello\n");   
    printf("World\n"); 


    // ==========================================================
    //  ÖRNEK 2: Thread Yönetimi ve ID Kavramı
    // ==========================================================
    
    // omp_set_num_threads global bir ayardır yani bundan sonra
    // thread sayımız 4'e sabitlenir. Fakat bundan önceki kodda
    // var olan tüm threadleriniz kullanılmaktaydı.
    omp_set_num_threads(4);

    // Bu kodda ise hem Hello hem World paralel olarak birden fazla
    // thread üzerinde çalıştırılır. 
    #pragma omp parallel
    {
        printf("Hello\n");
        printf("World\n");
    }

    // omp_get_num_threads ile kaç thread çalıştığına bakılabilir
    #pragma omp parallel
    {
        printf("Thread %d of %d\n", omp_get_thread_num(), omp_get_num_threads());
    }
    // OpenMP her threade bir sayı atar ama bu operating systemdan
    // gelen threadid değildir. 
    // Thread id ye ulaşmak için işletim sisteminin kendi
    // thread id kodunu kullanmanız gerekir.
    // Benim kodum şu an Windows üzerinde çalıştığı için
    // GetCurrentThreadId fonksiyonu windows.h üzerinden çağrılır.
    #pragma omp parallel
    {
        int omp_id = omp_get_thread_num();
        DWORD os_tid = GetCurrentThreadId();
        printf("OpenMP thread %d -> OS thread ID: %lu\n", omp_id, os_tid);
    }
    

    // ==========================================================
    //  ÖRNEK 3: Static vs Dynamic Schedule Testi
    // ==========================================================

    // Parallel komutu bloktaki her işlemi bütün threadler tarafından
    // baştan sona aynı şekide çalıştırı ama bazen belirli bir 
    // sayıda işlemi farklı threadler tarafından bir kere yapılmasını 
    // istersiniz. Bu durumda parallel for ile bir döngü sadece
    // bir kez tamamlanır ama iterasyonlar threadlere parçalanarak
    // parallel şekilde yürütülür.
    int sum = 0;

    #pragma omp parallel for
    for (int i = 0; i < 10000; i++) {
        #pragma omp atomic
        sum += 1;
    }
 
    printf("Sum is %d\n", sum);

    // Varsayılan olarak döngülerde schedule(static) geçelidir
    // Bu static şu anlama gelir döngü threadlere eşit olarak 
    // bölünür. Overheadi azdır ama karmaşık işlemlerde sıkıntı çıkartabilir
    // schedule(dynamic) ise kim müsait ise ona atama yapar.
    sum = 0;

    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < 10000; i++) {
        #pragma omp atomic
        sum += 1;
    }

    printf("Sum is %d\n", sum);

    // ==========================================================
    //  ÖRNEK 4: Zaman Ölçümü (omp_get_wtime)
    // ==========================================================


    // OpenMP parallel işlemlerde daha güvenli olacak şekilde 
    // tasarlanmış bir monoton artan saat kullanır ve bu saat OS saati ile 
    // senkronize olmadığından kullanıcı tarafından bozulamaz.
    // omp_get_wtime() fonksiyonu o anki ölüçümü getirir ve ana amacı
    // iki farklı zamanı birbirinden çıkartarak geçen zamanı  hesaplamaktır

    // Burda wtime wall-clock time olarak geçer ve ölçtüğü şey
    // kullanıcının gerçekte beklediği süredir.
    // Eğer CPU kullanım oranı istenirse clock() gibi fonksiyonlar
    // kullanılmalıdır.
    double start = omp_get_wtime();

    int a = 0;
    #pragma omp parallel for
    for (int i = 0; i < 10000; i++) {
        #pragma omp atomic
        a += 1;
    }
    printf("Counters final value is: %d \n", a);
    double end = omp_get_wtime();

    printf("Elapsed time = %f seconds\n", end - start);

    // ==========================================================
    //  ÖRNEK 5: Senkronizasyon Mekanizmaları
    // ==========================================================

    // Barrier tüm threadlerin belirli bir yere ulaşmadan geçmesini
    // engeller

    auto work = [](int n) {
        if (n % 1000 == 0)
            this_thread::sleep_for(milliseconds(100));
    };

    #pragma omp parallel
    {
        int id = omp_get_thread_num();
        work(id);
        #pragma omp barrier
        if (id == 0) 
            printf("All threads finished work \n");
    }

    // Burada race condition görülmesi için bir paralel döngü
    // yazıyoruz.
    int total = 0;
    int N = 100000000;
    vector<int> data(N, 5);

    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        total += data[i];
    }

    printf("Total after non-atomic: %d\n", total);

    total = 0;
    // Critical
    // Aynı anda sadece bir thread bu bloğu çalıştırır (mutex gibi).
    start = omp_get_wtime();
    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        #pragma omp critical
        total += data[i];
    }
    end = omp_get_wtime();

    printf("Total after critical: %d total time: %f\n", total, end - start);

    total = 0;
    // Atomic
    // Tek bir değişken güncellenecekse critical yerine daha hızlıdır.
    start = omp_get_wtime();
    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        #pragma omp atomic
        total += data[i];
    }
    end = omp_get_wtime();

    printf("Total after atomic: %d total time: %f\n", total, end - start);


    // reduction ise atomic counter veya locklama yapmadan direkt 
    // olarak her thread için bir lokal değişken oluşturup sonrasında ise
    // onları birleştirir.
    // Birleştirme türlere ayrılır alttaki örenkte sonuçlar toplanır
    // ama istersek *, -, &, max, min operasyonları da kullanılabilir.
    sum = 0;
    start = omp_get_wtime();
    #pragma omp parallel for reduction(+:sum)
    for (int i = 0; i < N; i++) {
        sum += data[i];
    }
    end = omp_get_wtime();

    printf("Total after reduction: %d total time: %f\n", sum, end - start);

    // ==========================================================
    //  ÖRNEK 6: Değişken Görünürlüğü
    // ==========================================================

    // OpenMp de bir paralel işlem yaparken normalde
    // runtime da kendisi değişkenin private mi shared mi
    // olacağına karar verir ve bu istenmeyen davranışlara 
    // sebep verebilir. Bu yüzden bütün paralelleştirdiğiniz
    // kısımların değişkenlerin türlerini açıkça belirtirseniz
    // hem istenmeyen davranışları azaltır hem de hataları önceden
    // tespit edebilirsiniz.

    // default(none) kullanma sebebimizde değişkenlerin private mi
    // shared mi olduğunu belirtmediğimiz sürece default bir türle 
    // başlatmamasını istememizdir.
    sum = 0;
    int arr[2] = {1};
    #pragma omp parallel default(none) shared(arr, N) reduction(+:sum) num_threads(2)
    {
        int id = omp_get_thread_num();
        for(int i = 0; i < N; i++) {
            sum += arr[id];
        }
    } 
    
}