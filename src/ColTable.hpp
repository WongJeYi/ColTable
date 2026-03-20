#ifndef COLTABLE_INTERFACE
#define COLTABLE_INTERFACE

#include <iostream>
#include <cstdlib>
#include <cstdint>
#include <memory>
#include <vector>
#include <list>
#include <malloc.h>
#include <cstring>
enum class format {
    // --- Explicitly Sized Signed Integers ---
    INT8, 
    INT16,  
    INT32,
    INT64,

    // --- C++ Native Signed Integers ---
    SHORT,
    LONG,
    LONG_LONG,

    // --- Explicitly Sized Unsigned Integers ---
    UINT8,
    UINT16,  
    UINT32, 
    UINT64,

    // --- C++ Native Unsigned Integers ---
    UNSIGNED_SHORT,
    UNSIGNED_LONG,
    UNSIGNED_LONG_LONG,

    // --- Floating Point ---
    FLOAT,  
    DOUBLE, 

    // --- Variable-Length & Complex Types ---
    STRING,
    LIST,
    AoA,
    STRUCT,
    DICT,

    // --- Fallback ---
    UNKNOWN
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

        template <typename T>
        static ColTable FromVector(const std::vector<std::vector<T>>& nestedVec);

        template <typename T>
        static ColTable FromList(const std::list<T>& vec);

        ColTable select(int64_t start, int64_t end);

        void add(const ColTable& other, int64_t index);

        void remove(int64_t start, int64_t end);
        // General template declaration for primitive types (1D)
        template <typename T>
        static void convertToArray(std::shared_ptr<ColTable> colTable, std::vector<T>& outResult);

        // Specialized declaration for 2D Array of Arrays
        template <typename T>
        static void convertToArray(std::shared_ptr<ColTable> table, std::vector<std::vector<T>>& outResult);
        
        template <typename T>
        static void convertToArray(std::shared_ptr<ColTable> table, std::list<T>& outResult);

        // Getter for the Data
        std::shared_ptr<ColTableData> getData() const { return data; }

        // Getter for the Field (Schema)
        std::shared_ptr<ColTableField> getField() const { return field; }
        virtual ~ColTable() = default;
    
};



// --- 2D Vectors (Array of Arrays) ---

template <typename T>
inline void ColTable::convertToArray(std::shared_ptr<ColTable> table, std::vector<std::vector<T>>& outResult) {
    outResult.clear();
    auto data = table->getData();
    auto offsetPtr = data->offsetBuffer;
    
    // 1. Safety check
    if (table->getField()->Format != format::AoA) {
        throw std::runtime_error("Table is not an Array of Arrays!");
    }
    if (!offsetPtr) return;
    
    const int64_t* offsetBuffer = static_cast<const int64_t*>(offsetPtr->get());
    size_t count = data->length; 
    
    auto childTable = std::make_shared<ColTable>(table->getField()->children[0], data->children[0]);
    
    // Recursively call convertToArray using the out-parameter for the 1D flat vector
    std::vector<T> flatVec;
    ColTable::convertToArray(childTable, flatVec);

    outResult.reserve(count);
    for (size_t i = 0; i < count; i++) {
        size_t start = offsetBuffer[i];
        size_t end = offsetBuffer[i+1];
        
        // Emplace back directly constructs the sublist avoiding unnecessary copies
        outResult.emplace_back(flatVec.begin() + start, flatVec.begin() + end);
    }
}


template <typename T>
inline ColTable ColTable::FromVector(const std::vector<std::vector<T>>& nestedVec) {
    size_t length = nestedVec.size();
    
    size_t totalInnerElements = 0;
    for (const auto& vec : nestedVec) {
        totalInnerElements += vec.size(); 
    }

    auto offsetBufferSize = (length + 1) * sizeof(int64_t);
    auto offsetBuffer = std::make_shared<Buffer>(offsetBufferSize);
    int64_t* offsets = static_cast<int64_t*>(offsetBuffer->get());
    
    std::vector<T> flattened;
    flattened.reserve(totalInnerElements);
    
    int64_t currentOffset = 0;
    for (size_t i = 0; i < length; ++i) {
        offsets[i] = currentOffset;
        currentOffset += nestedVec[i].size();
        flattened.insert(flattened.end(), nestedVec[i].begin(), nestedVec[i].end());
    }
    offsets[length] = currentOffset; 

    ColTable childTable = ColTable::FromVector<T>(flattened);
    
    auto parentField = std::make_shared<ColTableField>("AoA", format::AoA);
    parentField->addChild(childTable.getField()); 
    
    auto parentData = std::make_shared<ColTableData>(length);
    parentData->addOffsetBuffer(offsetBuffer);
    parentData->addChild(childTable.getData());   
    parentData->nullCount = -1;
    parentData->length = length; // Fixed minor syntax error

    return ColTable(parentField, parentData);
}

template <typename T>
inline void ColTable::convertToArray(std::shared_ptr<ColTable> table, std::list<T>& outResult) {
    outResult.clear();
    
    // 1. Safety check
    if (table->getField()->Format != format::LIST) {
        throw std::runtime_error("Table format is not a LIST!");
    }
    
    auto data = table->getData();
    if (data->children.empty()) return;

    // 2. Reconstruct the 1D child table that actually holds the flat data
    auto childTable = std::make_shared<ColTable>(table->getField()->children[0], data->children[0]);
    
    // 3. Extract the flat data into a temporary vector using your existing 1D function
    std::vector<T> tempVec;
    ColTable::convertToArray(childTable, tempVec);
    
    // 4. Assign the extracted data directly into the output list
    outResult.assign(tempVec.begin(), tempVec.end());
}

// --- 1D Lists ---

template <typename T>
inline ColTable ColTable::FromList(const std::list<T>& lst) {
    size_t length = lst.size();

    std::vector<T> flattened;
    flattened.reserve(length);
    
    for (const auto& item : lst) {
        flattened.push_back(item);
    }

    ColTable childTable = ColTable::FromVector<T>(flattened);
    
    auto parentField = std::make_shared<ColTableField>("LIST", format::LIST);
    parentField->addChild(childTable.getField()); 
    
    auto parentData = std::make_shared<ColTableData>(length);
    parentData->addChild(childTable.getData());   
    parentData->nullCount = -1;
    parentData->length = length; // Fixed minor syntax error
    
    return ColTable(parentField, parentData);
}


#endif //COLTABLE_INTERFACE