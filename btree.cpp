#pragma once
#include <cstdint>
#include <cstring>
#include <immintrin.h>

/*
 * BTree
 * ultra düşük gecikmeli bir B-Tree implementasyonudur.
 *
 * Başlıca optimizasyonlar:
 *   - Arena Allocator → malloc/free overhead’ı tamamen kaldırılır.
 *   - Cache-line alignment (64 byte) → false sharing ve cache miss azaltılır.
 *   - El ile döngü açma (manual unrolling) → branch prediction iyileşir.
 *   - inline fonksiyonlar → çağrı overhead’ı yoktur.
 *
 * Kullanım amacı:
 *   - Order book yapıları
 *   - Real-time lookup (O(log n)) gecikmeleri sabitleme
 *
 * ORDER parametresi → B-Tree düğümünün kapasitesini kontrol eder.
 */

template <typename Key, typename Value, int ORDER = 32>
class HFTBTree {
    static constexpr int MAX_KEYS  = ORDER * 2;   // Her node’da tutulabilecek maksimum key
    static constexpr int MAX_CHILD = MAX_KEYS + 1; // Çocuk sayısı = key + 1

    /*
     * 
     * Node Yapısı
     * 
     * Her düğüm cache-line hizalıdır (alignas(64)).
     * Böylece:
     *   - Cache locality artar
     *   - False sharing engellenir
     *   - SIMD erişimleri hızlanır
     */
    struct alignas(64) Node {
        bool leaf;              // Yaprak düğüm mü 
        uint16_t keyCount;      // Node içindeki anahtar sayısı 

        Key    keys[MAX_KEYS];  // Sabit boyutlu key dizisi
        Value  vals[MAX_KEYS];  // Key’e karşılık gelen value
        Node*  child[MAX_CHILD];// Çocuk pointer’ları 

        inline bool full() const { return keyCount == MAX_KEYS; }

        /*
         * Constructor: leaf bilgisi set edilir, çocuk pointer’ları sıfırlanır.
         * memset → hızlı sıfırlama
         */
        Node(bool lf) : leaf(lf), keyCount(0) {
            memset(child, 0, sizeof(child));
        }
    };

    Node* root;   // Ağacın kökü

    /*
     * Arena Allocator
     * Sistemlerinde "malloc" kullanmak çok pahalıdır:
     *   - Lock içerir → latency artar
     *   - Fragmentation yaratır
     *   - Cache locality zayıftır
     *
     * Bu yüzden tek büyük memory block (arena) ayrılır.
     * Her node placement-new ile bu arenadan alınır.
     */
    static constexpr size_t ARENA_SIZE = 1ull << 26; // 64 MB arena
    uint8_t* arena;
    size_t arenaOffset;

    /*
     * Arena’dan hizalı bir Node tahsisi yapılır.
     */
    inline Node* allocNode(bool leaf) {
        Node* ptr = reinterpret_cast<Node*>(arena + arenaOffset);
        arenaOffset += sizeof(Node);
        return new(ptr) Node(leaf); // Placement-new → malloc yok
    }

public:
    /*
     * Constructor:
     *  - Arena allocate edilir
     *  - Kök node oluşturulur 
     */
    HFTBTree() {
        arena = reinterpret_cast<uint8_t*>(aligned_alloc(64, ARENA_SIZE));
        arenaOffset = 0;

        root = allocNode(true);
    }

    /*
     * findPos() → B-Tree node içinde arama
     * Bu fonksiyon bir node içinde "k" anahtarının doğru pozisyonunu bulur.
     * Manual loop unrolling → 4 adım birden kontrol edilir.
     *
     * Amaç:
     *   - Branch misprediction azaltma
     *   - Pipeline yapısını koruma
     *   - CPU’ya daha öngörülebilir kod sağlama
     */
    inline int findPos(const Node* node, const Key& k) const {
        int i = 0;
        int n = node->keyCount;

        // 4’lü bloklarla hızlı arama
        for (; i + 4 <= n; i += 4) {
            if (node->keys[i] >= k)     return i;
            if (node->keys[i + 1] >= k) return i + 1;
            if (node->keys[i + 2] >= k) return i + 2;
            if (node->keys[i + 3] >= k) return i + 3;
        }
        // Geriye kalan birkaç eleman için normal arama
        for (; i < n; i++) {
            if (node->keys[i] >= k) return i;
        }
        return n;
    }

    /*
     * search() → Arama
     * Kökten başlayarak:
     *   - Pozisyon bulunur
     *   - Eğer key eşleşmişse değer döner
     *   - Yaprak değilse doğru çocuğa geçilir
     */
    inline Value* search(const Key& k) {
        Node* cur = root;
        while (true) {
            int pos = findPos(cur, k);
            if (pos < cur->keyCount && cur->keys[pos] == k)
                return &cur->vals[pos];
            if (cur->leaf) return nullptr;
            cur = cur->child[pos];
        }
    }

private:

    /*
     * splitChild() → Dolu olan bir çocuğu ikiye böler
     * 
     * Bir node’daki bir çocuk MAX_KEYS’e ulaştığında:
     *   - Orta eleman (mid) yukarı taşınır
     *   - Sağ taraf yeni node’a ayrılır
     *   - Parent içine yerleştirilir
     *
     * B-Tree’nin yapısal garantisi:
     *   Node hiçbir zaman MAX_KEYS’i geçmez.
     */
    inline void splitChild(Node* parent, int idx) {
        Node* fullNode = parent->child[idx];
        Node* newNode  = allocNode(fullNode->leaf);

        const int mid = MAX_KEYS / 2;

        // Sağ tarafa düşecek key sayısı
        newNode->keyCount = MAX_KEYS - mid - 1;

        // Sağ bloğu yeni node’a kopyala
        memcpy(newNode->keys, fullNode->keys + mid + 1,
               newNode->keyCount * sizeof(Key));
        memcpy(newNode->vals, fullNode->vals + mid + 1,
               newNode->keyCount * sizeof(Value));

        // Eğer yaprak değilse çocuk pointerlarını da kaydır
        if (!fullNode->leaf) {
            memcpy(newNode->child,
                   fullNode->child + mid + 1,
                   (newNode->keyCount + 1) * sizeof(Node*));
        }

        // Sol node’da kalan key sayısını azalt
        fullNode->keyCount = mid;

        // Parent içindeki elemanları sağa kaydır 
        for (int i = parent->keyCount; i > idx; --i) {
            parent->child[i + 1] = parent->child[i];
            parent->keys[i]      = parent->keys[i - 1];
            parent->vals[i]      = parent->vals[i - 1];
        }

        // Yeni node’u parent'a bağla
        parent->child[idx + 1] = newNode;
        parent->keys[idx] = fullNode->keys[mid];
        parent->vals[idx] = fullNode->vals[mid];
        parent->keyCount++;
    }

public:
    /*
     * insert() → Ağaca key/value ekler
     *
     * Eğer kök doluysa:
     *   - Yeni kök oluşturulur
     *   - Kök split edilir
     *   - Sonra insertNonFull çağrılır
     */
    inline void insert(const Key& k, const Value& v) {
        Node* r = root;
        if (r->full()) {
            Node* s = allocNode(false);
            root = s;
            s->child[0] = r;
            splitChild(s, 0);
            insertNonFull(s, k, v);
        } else {
            insertNonFull(r, k, v);
        }
    }

private:

    /*
     * insertNonFull():
     * Node dolu değilse arama yapılır ve uygun yere ekleme yapılır.
     *
     * Yaprak ise:
     *   - Key’ler kaydırılır ve doğru pozisyona eklenir.
     * İç düğüm ise:
     *   - Doğru çocuğa iner
     *   - Eğer çocuk doluysa önce split yapılır
     */
    inline void insertNonFull(Node* node, const Key& k, const Value& v) {
        int i = node->keyCount - 1;

        if (node->leaf) {
            // Doğru pozisyonu açmak için kaydırma
            while (i >= 0 && node->keys[i] > k) {
                node->keys[i + 1] = node->keys[i];
                node->vals[i + 1] = node->vals[i];
                i--;
            }
            node->keys[i + 1] = k;
            node->vals[i + 1] = v;
            node->keyCount++;
        } else {
            int pos = findPos(node, k);

            // Çocuk dolu ise split yap
            if (node->child[pos]->full()) {
                splitChild(node, pos);

                // Eğer split sonrası orta eleman k’dan küçükse sağa git
                if (node->keys[pos] < k) pos++;
            }

            insertNonFull(node->child[pos], k, v);
        }
    }
};
