#include <omp.h>
#include <iostream>
#include <chrono>
#include <thread>

using namespace std;
using namespace chrono;

// Burada dynamic ve static arasında klasik bir döngüde
// fark olur mu bunu denemek istedim. Gözlemlerime göre
// farklar önemsenmeyecek kadar az oldu ve bazen statik
// bazen de dynamic olan daha kısa sürdü.

int main() {
    // döngü uzunluğu
    const int N = 1000000;   
    const int threads = 8;
    omp_set_num_threads(threads);

    auto work = [](int i) {
        // Burada biraz döngü uzasın diye 100 milisaniyelik
        // ekstra zaman ekleniyor
        if (i % 1000 == 0)
            this_thread::sleep_for(milliseconds(100));
    };

    // --- STATIC ---
    auto t1 = high_resolution_clock::now();
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < N; i++) {
        work(i);
    }
    auto t2 = high_resolution_clock::now();
    auto staticTime = duration_cast<milliseconds>(t2 - t1).count();

    cout << "Static schedule time: " << staticTime << " ms\n";

    // --- DYNAMIC ---
    t1 = high_resolution_clock::now();
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < N; i++) {
        work(i);
    }
    t2 = high_resolution_clock::now();
    auto dynamicTime = duration_cast<milliseconds>(t2 - t1).count();

    cout << "Dynamic schedule time: " << dynamicTime << " ms\n";
}
