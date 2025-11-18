#include <bits/stdc++.h>
#include <x86intrin.h>
using namespace std;
using namespace std::chrono;

int main() {
    const int N = 10000000;
    vector<int> a(N, 1);

    auto start = high_resolution_clock::now();
    unsigned long long start_c = __rdtsc();

    long long sum = 0;
    for(int i = 0; i < N; i++) {
        sum += a[i];
    }
    unsigned long long end_c = __rdtsc();
    auto end = high_resolution_clock::now();
    double ms = duration<double, milli>(end - start).count();

    unsigned long long loop_start = _rdtsc();

    long long s = 0;
    for(int i = 0; i < N; i++) {
        s += i;
    }

    unsigned long long loop_end = _rdtsc();


    unsigned long long real_cycles = end_c - start_c;
    unsigned long long loop_overhead = loop_end - loop_start;
    unsigned long long pure_cycles = real_cycles - loop_overhead;

    cout << "Sum = " << sum << "\n";
    cout << "Total time: " << ms << " ms\n";
    cout << "Total cycles: " << real_cycles << "\n";
    cout << "Loop overhead: " << loop_overhead << "\n";
    cout << "Pure compute+memory cycles: " << pure_cycles << "\n";
}