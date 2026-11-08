#include <iostream>
#include <optional>
#include <string>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>
#include <cstddef>
#include <cstdint>


using std::cin; using std::cout; using std::string; using std::remove;using std::vector;

// Linked list of key -> value string pairs.
// Each key appears at most once: setting an existing key updates its value.
void printBanner() {
    std::cout << R"ART(
 ____        _                    ____  ____
/ ___|  __ _| |_ _   _ _ __ _ __ |  _ \| __ )
\___ \ / _` | __| | | | '__| '_ \| | | |  _ \
 ___) | (_| | |_| |_| | |  | | | | |_| | |_) |
|____/ \__,_|\__|\__,_|_|  |_| |_|____/|____/
)ART" << "\n";
}



string delSpaces(string s) {
    s.erase(remove(s.begin(), s.end(), ' '), s.end());
    return s;
}
class KVList {
private:
    struct Node {
        std::string key;
        std::string value;
        Node* next;
    };
 
    Node* head;
    int count;
 
    // Copying would make two lists share the same nodes (double delete),
    // so we forbid it for now.
    KVList(const KVList&) = delete;
    KVList& operator=(const KVList&) = delete;
 
public:
    KVList() {
        head = nullptr;
        count = 0;
    }
 
    // Free every node when the list goes away.
    ~KVList() {
        while (head != nullptr) {
            Node* old = head;
            head = head->next;
            delete old;
        }
    }
 
    // Add a new key, or update the value if the key already exists.
    // Returns true if a new key was added, false if an existing one was updated.
    bool set(const std::string& key, const std::string& value) {
        for (Node* cur = head; cur != nullptr; cur = cur->next) {
            if (cur->key == key) {
                cur->value = value;
                return false;
            }
        }
        head = new Node{key, value, head};   // new node points to the old head
        count++;
        return true;
    }
 
    // Returns the value, or std::nullopt if the key is not in the list.
    std::optional<std::string> get(const std::string& key) const {
        for (const Node* cur = head; cur != nullptr; cur = cur->next) {
            if (cur->key == key) {
                return cur->value;
            }
        }
        return std::nullopt;
    }
 
    bool contains(const std::string& key) const {
        for (const Node* cur = head; cur != nullptr; cur = cur->next) {
            if (cur->key == key) {
                return true;
            }
        }
        return false;
    }
 
    // Remove a key. Returns false if it wasn't there.
    bool remove(const std::string& key) {
        Node* prev = nullptr;
        Node* cur = head;
        while (cur != nullptr) {
            if (cur->key == key) {
                if (prev == nullptr) {       // removing the first node
                    head = cur->next;
                } else {
                    prev->next = cur->next;
                }
                delete cur;
                count--;
                return true;
            }
            prev = cur;
            cur = cur->next;
        }
        return false;
    }
 
    // NEW: call f(key, value) for every pair in the list.
    // The hash table uses this to move entries when it grows.
    template <typename Func>
    void forEach(Func f) const {
        for (const Node* cur = head; cur != nullptr; cur = cur->next) {
            f(cur->key, cur->value);
        }
    }

    int size() const {
        return count;
    }
 
    void print() const {
        for (const Node* cur = head; cur != nullptr; cur = cur->next) {
            std::cout << cur->key << "=" << cur->value << " -> ";
        }
        std::cout << "null\n";
    }
};
 
// NEW: hash table with separate chaining: an array of buckets, each bucket a KVList.
// A key goes into bucket  hash(key) % bucketCount.
// When the table holds more entries than buckets, it doubles the bucket count.
class HashTable {
private:
    KVList* buckets;          // array of bucketCount lists
    std::size_t bucketCount;
    std::size_t count;        // number of key/value pairs stored

    // Same reason as in KVList: a shallow copy would double-delete the buckets.
    HashTable(const HashTable&) = delete;
    HashTable& operator=(const HashTable&) = delete;

    std::size_t indexFor(const std::string& key) const {
        return hash(key) % bucketCount;
    }

    // Allocate newCount buckets and move every entry into them.
    // The index must be recomputed with the NEW bucket count.
    void resize(std::size_t newCount) {
        KVList* newBuckets = new KVList[newCount];
        for (std::size_t i = 0; i < bucketCount; i++) {
            buckets[i].forEach([&](const std::string& k, const std::string& v) {
                newBuckets[hash(k) % newCount].set(k, v);
            });
        }
        delete[] buckets;
        buckets = newBuckets;
        bucketCount = newCount;
    }

public:
    // FNV-1a, 64 bit: start from a constant, then for every byte XOR it in
    // and multiply by a prime.
    static std::uint64_t hash(const std::string& key) {
        std::uint64_t h = 14695981039346656037ULL;   // FNV offset basis
        for (char c : key) {
            h ^= static_cast<unsigned char>(c);
            h *= 1099511628211ULL;                   // FNV prime
        }
        return h;
    }

    explicit HashTable(std::size_t initialBuckets = 8) {
        bucketCount = (initialBuckets == 0) ? 1 : initialBuckets;  // avoid % 0
        buckets = new KVList[bucketCount];
        count = 0;
    }

    ~HashTable() {
        delete[] buckets;   // runs every KVList destructor, which frees the nodes
    }

    // Add a new key, or update an existing one.
    // Returns true if a new key was added, false if an existing one was updated.
    bool set(const std::string& key, const std::string& value) {
        bool isNew = buckets[indexFor(key)].set(key, value);
        if (isNew) {
            count++;
            if (count > bucketCount) {      // load factor above 1.0: grow
                resize(bucketCount * 2);
            }
        }
        return isNew;
    }

    std::optional<std::string> get(const std::string& key) const {
        return buckets[indexFor(key)].get(key);
    }

    bool contains(const std::string& key) const {
        return buckets[indexFor(key)].contains(key);
    }

    bool remove(const std::string& key) {
        bool removed = buckets[indexFor(key)].remove(key);
        if (removed) {
            count--;
        }
        return removed;
    }

    std::size_t size() const {
        return count;
    }

    // Prints every non-empty bucket with its index, so you can see how
    // the keys are distributed and where collisions happen.
    void print() const {
        for (std::size_t i = 0; i < bucketCount; i++) {
            if (buckets[i].size() > 0) {
                std::cout << "[" << i << "] ";
                buckets[i].print();
            }
        }
    }
};

std::string execute(HashTable& store, const std::vector<std::string>& args) {
    if (args[0] == "PING") {
        return "PONG";
    }
    if (args[0] == "GET") {
     if (args.size() != 2) return "ERR wrong number of arguments for 'get' command";

      std::optional<std::string> result = store.get(args[1]);
      if (result.has_value()) {
        return *result;        
      }
      return "(nil)";
}
    if(args[0] == "SET"){
       if(args.size() != 3) return "ERR wrong number of arguments for 'set' command";
       if(store.set(args[1],args[2])) return "Key added"; 
       return "Value replaced";

    }
   if (args[0] == "DEL") {
    if (args.size() < 2) return "ERR wrong number of arguments for 'del' command";
 
    int deleted = 0;
    std::string missing;
    for (size_t i = 1; i < args.size(); i++) {
        if (store.remove(args[i])) {
            deleted++;
        } else {
            missing += " " + args[i];
        }
    }

    std::string reply = std::to_string(deleted);
    if (!missing.empty()) reply += " (not found:" + missing + ")";
    return reply;
}
   if(args[0] == "PRINT"){
    if(args.size() > 1) return "ERR wrong number of arguments for 'print' command";
    store.print();
    return "";
   }
   if(args[0] == "SIZE"){
    if(args.size() > 1) return "ERR wrong number of arguments for 'size' command";
    return "Size: " + std::to_string(store.size());
   }
   if(args[0] == "CONTAINS"){
    if(args.size() != 2) return "ERR wrong number of arguments for 'contains' command";
    if(!store.contains(args[1])) return "FALSE";
    return "TRUE";
   }
   return "ERR unknown command '" + args[0] + "'";
}



int main(){
    HashTable store;   
    printBanner();
    do{
        cout << ">>";
        std::string line;
        if (!std::getline(std::cin, line)) break;        
        std::istringstream iss(line);
        std::vector<std::string> args;
        std::string word; 
        while (iss >> word) {
          args.push_back(word);
        }
        if (args.empty()) continue;
        std::transform(args[0].begin(), args[0].end(),args[0].begin(),[](unsigned char c) {return std::toupper(c);});       
        if(args[0] == "EXIT") break;
        cout << execute(store, args) << '\n';

                
        }
        while(true);
        return 0;
       }