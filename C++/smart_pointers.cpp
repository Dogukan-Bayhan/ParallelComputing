#include <iostream>
#include <memory> // unique ve shared pointers için
#include <utility> // std::move için


// ------------ Unique Pointer ----------------


// unique_ptr yalnızca tek bir sahip(owner) tarafından yönetilebilen bir akıllı işaretçidir.
// Yönettiği kaynağın yaşam döngüsünden sadece bu işaretçi sorumludur.

// unique_ptr kopyalanamaz, ancak taşınabilir.

// Hafiflik: Ek bir kontrol bloğu veya referans sayacı tutmaz; bu yüzden shared_ptr'dan daha 
// hafiftir ve ham işaretçi(raw pointer) kadar hızlıdır.

class Resource{
public:
    Resource() {
        std::cout << "Resource Created\n";
    }
    ~Resource() {
        std::cout << "Resource Destroyed\n";
    }
    void operation() {
        std::cout << "Resource in operation\n";
    }
};

void process_resource(std::unique_ptr<Resource> res) {
    std::cout << "-> Entering process_resource function\n";
    if (res) {
        res->operation();
    }
    std::cout << "<- Exiting process_resource function\n";
}


// ------------ Shared Pointer ----------------

class SharedResource : public std::enable_shared_from_this<SharedResource>{
public:
    SharedResource(int id) : id_(id) { std::cout << "SharedResource " << id_ << " Created\n"; }
    ~SharedResource() { std::cout << "SharedResource " << id_ << " Destroyed\n"; }
    void access() { std::cout << "SharedResource " << id_ << " is being accessed. Ref Count: " << ref_count() << "\n"; }

    long ref_count() const {
        // use_count() metodunu kullanarak referans sayacını görebiliriz.
        return shared_from_this().use_count(); 
    }
private:
    int id_;
};

void use_resource(std::shared_ptr<SharedResource> res) {
    res->access();
}

int main() {
    std::cout << "--- unique_ptr Example ---\n";

    // Burada önemli bir konudan bahsedelim:
    // Burayı std::unique_ptr<Resource> source(new Resource);
    // olarak çağırabilirdik ve aynı işi yapardı fakat std::make_unique bunu
    // daha güvenli optimize şekilde yapar bu yüzden tercihiniz bu şekilde olmalıdır.
    std::unique_ptr<Resource> ptr1 = std::make_unique<Resource>();

    if(ptr1) {
        std::cout << "ptr1 is valid\n";
    }

    std::cout << "\n--- Moving Ownership ---\n";
    std::unique_ptr<Resource> ptr2 = std::move(ptr1);


    // Burada ptr1 sahipliğini ptr2'ye devretmiştir
    if(!ptr1) {
        std::cout << "ptr1 is now nullptr after move\n";
    }

    ptr2->operation();
    
    std::cout << "\n--- Transfering to Function ---\n";

    // Burada move ile ptr2 sahipliğini fonksiyon içerisindeki değişkene
    // geçiriyor ve sonunda scope sonu olduğu için yok oluyor.
    process_resource(std::move(ptr2));

    if(!ptr2) {
        std::cout << "ptr2 is now nullptr after function call\n";
    }

    std::cout << "--- End of main (No leaks!) ---\n";


    std::cout << "----------------------------------\n\n";
    std::cout << "-------- Shared Pointers ---------\n";

    std::cout << "--- shared_ptr Example ---\n";

    std::shared_ptr<SharedResource> ptr_shared;

    {
        std::shared_ptr<SharedResource> ptr_main = std::make_shared<SharedResource>(100);
        ptr_shared = ptr_main;

        std::cout << "Current Ref Count: " << ptr_main.use_count() << "\n";

        std::shared_ptr<SharedResource> ptr_temp = ptr_main;
        std::cout << "Current Ref Count: " << ptr_main.use_count() << "\n";

        use_resource(ptr_temp);
        std::cout << "After function call Ref Count: " << ptr_main.use_count() << "\n"; 

    }

    std::cout << "\n--- After Inner Scope ---\n";
    std::cout << "ptr1 Ref Count: " << ptr_shared.use_count() << "\n"; 
    ptr_shared->access();

    std::cout << "--- End of main ---\n";

    return 0;
}