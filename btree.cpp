#pragma once
#include <cstdint>
#include <cstring>
#include <immintrin.h>

template <typename Key, typename Value, int ORDER = 32>
class HFTBTree {
    static constexpr int MAX_KEYS  = ORDER * 2;
    static constexpr int MAX_CHILD = MAX_KEYS + 1;

    struct alignas(64) Node {
        bool leaf;
        uint16_t keyCount;

        Key    keys[MAX_KEYS];
        Value  vals[MAX_KEYS];
        Node*  child[MAX_CHILD];

        inline bool full() const { return keyCount == MAX_KEYS; }

        Node(bool lf) : leaf(lf), keyCount(0) {
            memset(child, 0, sizeof(child));
        }
    };

    Node* root;

    static constexpr size_t ARENA_SIZE = 1ull << 26; 
    uint8_t* arena;
    size_t arenaOffset;

    inline Node* allocNode(bool leaf) {
        Node* ptr = reinterpret_cast<Node*>(arena + arenaOffset);
        arenaOffset += sizeof(Node);
        return new(ptr) Node(leaf);
    }

public:
    HFTBTree() {
        arena = reinterpret_cast<uint8_t*>(aligned_alloc(64, ARENA_SIZE));
        arenaOffset = 0;

        root = allocNode(true);
    }

    inline int findPos(const Node* node, const Key& k) const {
        int i = 0;
        int n = node->keyCount;

        for (; i + 4 <= n; i += 4) {
            if (node->keys[i] >= k)     return i;
            if (node->keys[i + 1] >= k) return i + 1;
            if (node->keys[i + 2] >= k) return i + 2;
            if (node->keys[i + 3] >= k) return i + 3;
        }
        for (; i < n; i++) {
            if (node->keys[i] >= k) return i;
        }
        return n;
    }

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

    inline void splitChild(Node* parent, int idx) {
        Node* fullNode = parent->child[idx];
        Node* newNode  = allocNode(fullNode->leaf);

        const int mid = MAX_KEYS / 2;

        newNode->keyCount = MAX_KEYS - mid - 1;
        memcpy(newNode->keys, fullNode->keys + mid + 1,
               newNode->keyCount * sizeof(Key));
        memcpy(newNode->vals, fullNode->vals + mid + 1,
               newNode->keyCount * sizeof(Value));

        if (!fullNode->leaf) {
            memcpy(newNode->child,
                   fullNode->child + mid + 1,
                   (newNode->keyCount + 1) * sizeof(Node*));
        }

        fullNode->keyCount = mid;

        for (int i = parent->keyCount; i > idx; --i) {
            parent->child[i + 1] = parent->child[i];
            parent->keys[i]      = parent->keys[i - 1];
            parent->vals[i]      = parent->vals[i - 1];
        }

        parent->child[idx + 1] = newNode;
        parent->keys[idx] = fullNode->keys[mid];
        parent->vals[idx] = fullNode->vals[mid];
        parent->keyCount++;
    }

public:
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

    inline void insertNonFull(Node* node, const Key& k, const Value& v) {
        int i = node->keyCount - 1;

        if (node->leaf) {
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
            if (node->child[pos]->full()) {
                splitChild(node, pos);
                if (node->keys[pos] < k) pos++;
            }
            insertNonFull(node->child[pos], k, v);
        }
    }
};
