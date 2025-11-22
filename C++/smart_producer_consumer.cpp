#include <iostream>
#include <queue>              // Kuyruk yapısı
#include <string>
#include <thread>             // Çoklu iş parçacığı
#include <mutex>              // Mutex ve lock_guard için
#include <condition_variable> // Thread senkronizasyonu
#include <memory>             // Akıllı İşaretçiler (shared_ptr)
#include <vector>
#include <chrono>


// Burada cout kullandım ama siz print kullanırsanız hepsi yerine
// yazılarda atomik bir şekilde terminale yazılacaktır.

// --- 1. Thread Safe Kuyruk: SafeQueue ---
// Üretici ve tüketiciler arasında veri paylaşımını mutex ve condition_variable ile senkronize eder.


class SafeQueue{
private:
    std::queue<std::string> queue_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool terminated_ = false;
public:

    void push(const std::string& item) {
        std::lock_guard<std::mutex> lock(mtx_);
        queue_.push(item);
        cv_.notify_one();
    }

    void terminate() {
        std::lock_guard<std::mutex> lock(mtx_);
        terminated_ = true;
        cv_.notify_all();
    }

    std::string pop() {
        std::unique_lock<std::mutex> lock(mtx_);

        cv_.wait(lock, [this]{return !queue_.empty() || terminated_;});

        if (terminated_ && queue_.empty()) {
            return "SYSTEM_STOPPED";
        }

        std::string item = queue_.front();
        queue_.pop();
        return item;
    }
};

class Producer{
public:
    Producer(std::shared_ptr<SafeQueue> queue) : queue_(std::move(queue)) {}
    ~Producer() {
        std::cout << "Producer destructed\n";
    }

    void run() {
        std::cout << "[PRODUCER] Started. Generating data blocks.\n";

        for(int i = 0; i < 20; i++) {
            std::string message = "DATA_Block_" + std::to_string(i);

            queue_->push(message);
            std::cout << "[PRODUCER] Pushed to Queue: " << message << "\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        queue_->terminate();
        std::cout << "[PRODUCER] Finished and sent TERMINATE signal.\n";
    }

private:
    std::shared_ptr<SafeQueue> queue_;
};

class Consumer{
public:
    Consumer(int id, std::shared_ptr<SafeQueue> q) : id_(id), queue_(std::move(q)) {}

    void run() {
        std::cout << "[CONSUMER " << id_ << "] Started.\n";
        while(true) {
            std::string message = queue_->pop();

            if(message == "SYSTEM_STOPPED") {
                break;
            }

            std::cout << "[CONSUMER " << id_ << "] Consumed & Processed: " << message << "\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        std::cout << "[CONSUMER " << id_ << "] Gracefully stopped.\n";
    }

private:
    int id_;
    std::shared_ptr<SafeQueue> queue_;
};

int main() {
    std::cout << "--- STARTING THREADED QUEUE SYSTEM ---\n";

    std::shared_ptr<SafeQueue> message_queue = std::make_shared<SafeQueue>();


    Producer producer(message_queue);
    Consumer consumer1(1, message_queue);
    Consumer consumer2(2, message_queue);

    std::thread producer_thread(&Producer::run, &producer);
    std::thread consumer_thread1(&Consumer::run, &consumer1);
    std::thread consumer_thread2(&Consumer::run, &consumer2);

    producer_thread.join();
    consumer_thread1.join();
    consumer_thread2.join();

    std::cout << "\n[MAIN] All threads have finished their tasks.\n";
    std::cout << "--- SYSTEM SHUTDOWN COMPLETE (RAII Cleaned Everything) ---\n";

    return 0;
}

