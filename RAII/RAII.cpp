#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <chrono>

// RAII(Resource Acquisition Is Initialization)
// C++ deterministik yıkıcı(destructor) özelliğini, kaynak yönetimini
// otomatikleştirmek için kullanan bir tasarım felsefesidir.

// Deterministik Yıkıcılar: C++'ta bir nesnenin kapsamı(scope) dışına
// çıkıldığında, nesnenin yıkıcısının ne zaman çalışacağı kesindir ve garanti edilir.
// Bu garanti, istisna fırlatılsa bile geçerlidir, yani yığın geri sarılırken(stack unwinding) 
// nesnelerin yıkıcıları çağrılır.

// RAII, bir kaynağı(bellek, dosya, kilit, ...) elde etme işlemini, bir nesnenin başlatılmasına(initialization)
// sıkıca bağlar. Kaynak, nesnenin ömrü boyunca etkin kalır ve nesnenin ömrü sona erdiğinde(yıkıcı çağrıldığında)
// kaynak her zaman ve güvenli bir şekilde serbest bırakılır.


// Burada sadece destruction mantığının anlaşılması için
// basit bir class yazdık
class Data{
public:
    Data() {
        std::cout << "Data initialized\n";
    }
    ~Data() {
        std::cout << "Data destructed!\n";
    }
    void do_work() {}
};


// Burada neden unique_ptr olarak oluşturmanın memory leak önlediğini
// anlamamız için tasarladığım bir kod var
// İlk örnekteki gibi eğer düz bir pointer olarak başlatırsanız
// scope sona erdiğinde Data nesnesi hala heapte varlığını sürdürmeye devam
// ediyor olacaktır.

// Fakat unique_ptr kullandığımız zaman Data heapten scope sona erdiği zaman
// temizlenmiş olacaktır.
void memory_management() {
    std::cout << "--- 1. pointer begining ---\n";
    {
        Data* source = new Data();
        source->do_work();
    }
    std::cout << "--- 1. pointer ending ---\n";

    std::cout << "\n";

    std::cout << "--- 1. unique_ptr begining ---\n";
    {
        std::unique_ptr<Data> source(new Data());
        source->do_work();
    }
    std::cout << "--- 1. unique_ptr ending ---\n";

    std::cout << "\n";
    std::cout << "\n";
    std::cout << "\n";
}

// Burada raii örneği olarak lock_guard kullanıldı.
// lock_guard scope dışına çıkılırken otomatik olarak unlock() çağırır.

std::mutex mtx;

void lock_manager() {
    std::cout << "--- 2. lock_guard beginning ---\n";

    std::lock_guard<std::mutex> lock(mtx);

    std::cout << "Here is the critical region\n";

    std::cout << "Ending of the critical region\n";

    std::cout << "\n";
}

void lock_raii() {
    std::thread t1(lock_manager);
    std::thread t2(lock_manager);

    t1.join();
    t2.join();

    std::cout << "--- 2. lock guard ending ---\n";

    std::cout << "\n";
    std::cout << "\n";
    std::cout << "\n";
}


// Burada da yine raii prensibini kullanan ama biraz daha karmaşık
// lock yapılarını deneyimliyoruz.
// scoped_lock lock_guard gibi fakat ekstra olarak aynı anda birden fazla
// lock almayı temiz bir şekilde yapar.
// unique_lock ise aralarında en güvenlisi fakat aynı zamanda en yavaşıdır.
// unique_lock içerisinde manuel unlcok() ve lock() metodları da vardır.
std:: mutex complex_lock;

void complex_locks() {
    std::cout << "--- 3. unique_lock/scoped_lock beginning ---\n";
    {
        std::scoped_lock lock(complex_lock);
        std::cout << "locked with scoped_lock";
    }

    std::cout << "\n";

    {
        std::unique_lock<std::mutex> u_lock(complex_lock);
        std::cout << "locked with unique_lock.\n";
        u_lock.unlock(); 
        std::cout << "Manuel unlock in unique_lock.\n";
        u_lock.lock(); 
    }
}

void complex_locks_raii() {
    std::thread t1(complex_locks);
    std::thread t2(complex_locks);

    t1.join();
    t2.join();

    std::cout << "3. --- complex locks ending ---";
    std::cout << "\n";
}


int main() {
    memory_management();

    lock_raii();

    complex_locks_raii();
}