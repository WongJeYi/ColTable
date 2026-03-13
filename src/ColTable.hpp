#ifndef COLTABLE_INTERFACE
#define COLTABLE_INTERFACE

#include <iostream>
#include <cstdlib>
#include <cstdint>
#include <memory>
#include <vector>
#include <malloc.h>
#include <cstring>

enum class format {

INT32, INT64, DOUBLE, FLOAT, STRING, LIST, STRUCT, DICT, UNKNOWN
};

class Buffer{
    
    private:
        void* raw_ptr = nullptr;    // The actual address for free()
        void* aligned_ptr = nullptr;
    public:
        void* buffer;
        size_t bufferSize;

        Buffer(size_t size) : bufferSize(size){
            size_t alignment = 64;
            size_t total_size = size + alignment + sizeof(void*);
            raw_ptr = std::malloc(total_size);
            if (!raw_ptr) throw std::bad_alloc();
            uintptr_t raw_addr = reinterpret_cast<uintptr_t>(raw_ptr);
            uintptr_t start_addr = raw_addr + sizeof(void*);
            uintptr_t aligned_addr = (start_addr + alignment - 1) & ~(alignment - 1);

            aligned_ptr = reinterpret_cast<void*>(aligned_addr);
            buffer = static_cast<void**>(aligned_ptr)[-1] = raw_ptr;
        }
        
        // Standard Destructor
        ~Buffer() {
            if (aligned_ptr) {
                // Retrieve the hidden original pointer
                void* original = static_cast<void**>(aligned_ptr)[-1];
                std::free(original);
            }
        }
        void* get() const { return aligned_ptr; }
    
        // Disable copying (to prevent double-free)
        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;
    template <typename T = void>
    T* as(format fmt) const {
    // We use reinterpret_cast because we are forcing a 
    // specific type interpretation onto raw memory.
    switch (fmt) {
        case format::INT32:  return reinterpret_cast<T*>(aligned_ptr);
        case format::INT64:  return reinterpret_cast<T*>(aligned_ptr);
        case format::FLOAT:  return reinterpret_cast<T*>(aligned_ptr);
        case format::DOUBLE: return reinterpret_cast<T*>(aligned_ptr);
        case format::STRING: return reinterpret_cast<T*>(aligned_ptr);
        case format::LIST:
        case format::STRUCT:
        case format::DICT:   return reinterpret_cast<T*>(aligned_ptr);
        default:             return nullptr;
    }
}

    // Helper for direct indexing after casting
    template <typename T>
    T& at(size_t index) const {
        return static_cast<T*>(aligned_ptr)[index];
    }
};


class ColTableField{
    public:
        std::string key;

        format Format;

        std::vector<std::shared_ptr<ColTableField>> children;

        int64_t nChildren;

        ColTableField(const std::string& key, format fmt): key(key), 
            Format(fmt), 
            nChildren(0) // Explicitly initialize to zero
        {};

        virtual ~ColTableField() = default;

        void addChild(const std::shared_ptr<ColTableField> child){
            if(child){
                children.push_back(child);
                nChildren = children.size();
            }
        }
        // Helper to print the tree structure recursively
        void print(int indent = 0) const {
            std::string spaces(indent, ' ');
            std::cout << spaces << "|-- " << key << " (Children: " << nChildren << ")\n";
            for (const auto& child : children) {
                child->print(indent + 4);
            }
        }
};


class ColTableData{
    public:
        std::vector<std::shared_ptr<ColTableData>> children;

        size_t length;

        int64_t nullCount;

        int64_t validityBitmapsBuffer;

        int64_t nChildren;
        
        std::shared_ptr<Buffer> offsetBuffer;
        std::shared_ptr<Buffer> valueBuffer;

        ColTableData(int64_t length) : length(length), nullCount(0) {};
        
        void addChild(const std::shared_ptr<ColTableData> child){
            if(child){
                children.push_back(child);
                nChildren = children.size();
            }
        }

        void addBuffer(std::shared_ptr<Buffer> buffer){
            valueBuffer = buffer;
        }
        
        void addOffsetBuffer(std::shared_ptr<Buffer> buffer){
            offsetBuffer = buffer;
        }
        std::shared_ptr<Buffer> getBuffer(){
            return valueBuffer;
        }
        
        virtual ~ColTableData() = default;

};

void printValueBuffer(const std::shared_ptr<ColTableData>& data, format type);

class ColTable{
    public:
        std::shared_ptr<ColTableField> field;

        std::shared_ptr<ColTableData> data;

        ColTable(std::shared_ptr<ColTableField> field, std::shared_ptr<ColTableData> data) : field(field), data(data) {};

        template <typename T>
        static ColTable FromVector(const std::vector<T>& vec);

        ColTable select(int64_t start, int64_t end);

        void add(const ColTable& other, int64_t index);

        void remove(int64_t start, int64_t end);
        
        template <typename T>
        static std::vector<T> convertToArray(std::shared_ptr<ColTable> colTable);
        // Getter for the Data
        std::shared_ptr<ColTableData> getData() const { return data; }

        // Getter for the Field (Schema)
        std::shared_ptr<ColTableField> getField() const { return field; }
        virtual ~ColTable() = default;

};





#endif //COLTABLE_INTERFACE