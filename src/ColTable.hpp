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
#include <optional>

// Overload the << operator for ANY std::optional
template <typename T>
std::ostream& operator<<(std::ostream& os, const std::optional<T>& opt) {
    if (opt.has_value()) {
        os << opt.value();
    } else {
        os << "[std::nullopt]";
    }
    return os;
}
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
        //Move constructor
        Buffer(Buffer && other){
            std::swap(other. raw_ptr,raw_ptr);
            std::swap(other.aligned_ptr,aligned_ptr);
            std::swap(other.buffer,buffer);
            std::swap(other.bufferSize,bufferSize);
        }
        //Move assignment operator
        Buffer & operator=(Buffer && other){
            if(this==&other){return *this;}
            else{
                if(raw_ptr){
                    std::free(raw_ptr);
                }
                std::swap(other. raw_ptr,raw_ptr);
                std::swap(other.aligned_ptr,aligned_ptr);
                std::swap(other.buffer,buffer);
                std::swap(other.bufferSize,bufferSize);
            }
            return *this;
        }
        // Standard Destructor
        ~Buffer() {
            if (raw_ptr) {
                std::free(raw_ptr);
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
        //Copy constructor
        ColTableField(ColTableField const & other){
           
            Format=other.Format;
            key=other.key;
            nChildren=other.nChildren;
            if(!other.children.empty()){
                children.reserve(other.children.size());
                for (const auto & ptr: other.children){
                    if(ptr){
                        children.push_back(std::make_shared<ColTableField>(*ptr));
                    }else{
                        children.push_back(nullptr);
                    }
                }
            }
        }
        //Move Constructor
        ColTableField(ColTableField && other){
            std::swap(other.key,key);
            std::swap(other.Format,Format);
            std::swap(other.children,children);
            std::swap(other.nChildren,nChildren);
        }
        //Move assignment operator
        ColTableField & operator = (ColTableField && other){
            if( this==&other){ return *this;}
            std::swap(other.key,key);
            std::swap(other.Format,Format);
            std::swap(other.children,children);
            std::swap(other.nChildren,nChildren);
            return *this;
        }
        //Copy assignment operator
        ColTableField & operator = (ColTableField const & other){
            if( this==&other){ return *this;}
            Format=other.Format;
            key=other.key;
            nChildren=other.nChildren;
            children.clear();
            if(!other.children.empty()){
                    
                children.reserve(other.children.size());
                for (const auto & ptr: other.children){
                    if(ptr){
                        children.push_back(std::make_shared<ColTableField>(*ptr));
                    }else{
                        children.push_back(nullptr);
                    }
                }
            }
            return *this;
        }
        void addChild(const std::shared_ptr<ColTableField> child){
            if(child){
                children.push_back(child);
                nChildren = children.size();
            }
        }
        void removeChild(int64_t index){
            children.erase(children.begin()+index);
            nChildren = children.size();
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
        //Copy constructor
        ColTableData(ColTableData const & other){
            children = other.children;
            length = other.length;
            nullCount=other.nullCount;
            validityBitmapsBuffer=other.validityBitmapsBuffer;
            nChildren=other.nChildren;
            if(!other.children.empty()){
                children.reserve(other.children.size());
                for (const auto & ptr: other.children){
                    if(ptr){
                        children.push_back(std::make_shared<ColTableData>(*ptr));
                    }else{
                        children.push_back(nullptr);
                    }
                }
            }
            if(other.offsetBuffer){
                offsetBuffer=(other.offsetBuffer);
            }
            if(other.valueBuffer){
                valueBuffer=(other.valueBuffer);
            }
        }
        //Move Constructor
        ColTableData(ColTableData && other){
            std::swap(other.children,children);
            std::swap(other.length,length);
            std::swap(other.nullCount,nullCount);
            std::swap(other.validityBitmapsBuffer,validityBitmapsBuffer);
            std::swap(other.valueBuffer,valueBuffer);
            std::swap(other.offsetBuffer,offsetBuffer);
        }
        //Move assignment operator
        ColTableData & operator=(ColTableData && other){
            if(this==&other){return *this;}
            std::swap(other.children,children);
            std::swap(other.length,length);
            std::swap(other.nullCount,nullCount);
            std::swap(other.validityBitmapsBuffer,validityBitmapsBuffer);
            std::swap(other.valueBuffer,valueBuffer);
            std::swap(other.offsetBuffer,offsetBuffer);
            return *this;
        }
        //Copy assignment operator
        ColTableData & operator=(ColTableData const & other){
            if(this==&other){return *this;}
            children=other.children;
            length=other.length;
            validityBitmapsBuffer=other.validityBitmapsBuffer;
            if(!other.children.empty()){
                children.reserve(other.children.size());
                for (const auto & ptr: other.children){
                    if(ptr){
                        children.push_back(std::make_shared<ColTableData>(*ptr));
                    }else{
                        children.push_back(nullptr);
                    }
                }
            }
            if(other.valueBuffer){
                valueBuffer=(other.valueBuffer);
            }
            if(other.offsetBuffer){
                offsetBuffer=(other.offsetBuffer);
            }
            return *this;
        }
        void addChild(const std::shared_ptr<ColTableData> child){
            if(child){
                children.push_back(child);
                nChildren = children.size();
            }
        }
        void removeChild(int64_t index){
            
            children.erase(children.begin()+index);
            nChildren = children.size();
            
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
        //Copy constructor
        ColTable(ColTable const & other){
            if(other.field){
                field=std::make_shared<ColTableField>(*other.field);
            }
            if(other.data){
                data=std::make_shared<ColTableData>(*other.data);
            }
        } 
        //Move constructor
        ColTable(ColTable && other){
            std::swap(other.field,field);
            std::swap(other.data,data);
        }
        //Move assignment constructor
        ColTable & operator=(ColTable && other){
            if(this==&other){ return *this;}
            else{
                std::swap(other.field,field);
                std::swap(other.data,data);
            }
            return *this;        
        }
        //Copy assignment constructor
        ColTable & operator=(ColTable const & other){
            if(this==&other){return *this;}
            else{
                if(other.field){
                    field=std::make_shared<ColTableField>(*other.field);
                }
                if(other.data){
                    data=std::make_shared<ColTableData>(*other.data);
                }
            }
            return *this;
        }
        template <typename T>
        static ColTable FromVector(const std::vector<T>& vec);

        static ColTable FromStruct(const std::vector<ColTable>& members,std::vector<std::string> keys);

        template <typename T>
        static ColTable FromVector(const std::vector<std::vector<T>>& nestedVec);

        template <typename T>
        static ColTable FromVector(const std::list<T>& vec);

        template <typename T>
        static ColTable FromVector(const std::vector<std::list<T>>& vec);
        ColTable select(int64_t start, int64_t end);

        void add(const ColTable& other, int64_t index){
            if(!other.data){
                this->data->addChild(other.data);
            }
            if(!other.field){
                this->field->addChild(other.field);
            }
        }

        void remove(int64_t index){
            if(!this->data->children.empty()){
                this->data->removeChild(index);
            }
            if(!this->field->children.empty()){
                this->field->removeChild(index);
            }
            
        }
        // General template declaration for primitive types (1D)
        template <typename T>
        static void convertToArray(std::shared_ptr<ColTable> colTable, std::vector<T>& outResult);
        
        // General template declaration for struct
        static void convertToStruct(std::shared_ptr<ColTable> colTable, std::vector<ColTable>& outResult);

        // Specialized declaration for 2D Array of Arrays
        template <typename T>
        static void convertToArray(std::shared_ptr<ColTable> table, std::vector<std::vector<T>>& outResult);
        
        template <typename T>
        static void convertToArray(std::shared_ptr<ColTable> table, std::list<T>& outResult);
        
        // General template declaration for vector of list
        template <typename T>
        static void convertToArray(std::shared_ptr<ColTable> colTable, std::vector<std::list<T>>& outResult);
        
        // Getter for the Data
        std::shared_ptr<ColTableData> getData() const { return data; }

        // Getter for the Field (Schema)
        std::shared_ptr<ColTableField> getField() const { return field; }
        virtual ~ColTable() = default;
    
};



//wrapper to map string key for struct variables
template<typename T, typename FieldType>
struct ColTableStructField{
    std::string name;
    FieldType T::*ptr;
};
    class ColTableStructMapper{
        private:
            // std::vector<T>& vec is the vector of struct
            template<typename T, typename FieldType>
            static ColTable extract_column(const std::vector<T>& vec,const std::string& name, FieldType T::* ptr){
                std::vector<FieldType> column;
                column.reserve(vec.size());
                for(const auto& v : vec){
                    column.push_back(v.*(ptr));
                }
                // Construct the coltable from column
                ColTable col=ColTable::FromVector(column);
                col.field->key = name;
                return col;
            }

            template<typename T, typename FieldType>
            static void assign_column(std::vector<T>& vec,const std::vector<ColTable>& coltable, const std::string& name, FieldType T::* ptr){
                for( const auto & col : coltable){
                    std::vector<FieldType> extracted;
                    if(col.field->key==name){
                        ColTable::convertToArray(std::make_shared<ColTable>(col),extracted);
                        for(int i=0;i<extracted.size();i++){
                            vec[i].*(ptr)=std::move(extracted[i]);
                        }
                        break;
                    }
                }
            }
            template <typename T>
            static void assign_all(std::vector<T>& /*vec*/, const std::vector<ColTable>& /*structCols*/) {
                // End of recursion
            }
            template <typename T, typename StringType, typename FieldType, typename... Rest>
            static void assign_all(std::vector<T>& vec, const std::vector<ColTable>& cols, const StringType& name, FieldType T::*ptr, Rest... rest) {
                assign_column(vec, cols, name, ptr);  // Do the first one
                assign_all(vec, cols, rest...);   // Recursively call for the rest
            }
            // Base Case
            template <typename T>
            static void extract_all(const std::vector<T>& /*vec*/, std::vector<ColTable>& /*members*/, std::vector<std::string>& /*keys*/) {}

            // Recursive Case (Pulls TWO arguments off the list: 'name' and 'ptr')
            template <typename T, typename StringType, typename FieldType, typename... Rest>
            static void extract_all(const std::vector<T>& vec, std::vector<ColTable>& members, std::vector<std::string>& keys, const StringType& name, FieldType T::* ptr, Rest... rest) {
                members.push_back(extract_column(vec, name, ptr));
                keys.push_back(name);
                extract_all(vec, members,keys ,rest...); // Recurse with whatever is left
            }
        public:
            template <typename T, typename... Args>
            static ColTable Serialize(const std::vector<T>& vec, Args... args){
                std::vector<ColTable> fieldsTable;
                std::vector<std::string> keys;
                //Construct column from struct
                extract_all(vec,fieldsTable,keys, args...);
                //Pass keys map to variable to ColTable
                return ColTable::FromStruct(fieldsTable, keys);
            }

            template <typename T, typename... Args>
            static void Deserialize(std::shared_ptr<ColTable> coltable, std::vector<T>& outVec,Args... args){
                std::vector<ColTable> coltableStruct;
                ColTable::convertToStruct(coltable,coltableStruct);
                size_t len= coltableStruct.size();
                outVec.resize(len);
                assign_all(outVec, coltableStruct,args...);
            }
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


inline ColTable ColTable::FromStruct(const std::vector<ColTable>& members, const std::vector<std::string> keys) {
    size_t length = members.size();
    
    auto parentData = std::make_shared<ColTableData>(length);
    auto parentField = std::make_shared<ColTableField>("STRUCT", format::STRUCT);
    for(int i=0;i<length;i++){
        members[i].field->key=keys[i];
        parentField->addChild(members[i].getField());
        parentData->addChild(members[i].getData());
    }  
    
    parentData->length = length; // Fixed minor syntax error
    
    return ColTable(parentField, parentData);
}

inline void ColTable::convertToStruct(std::shared_ptr<ColTable> table, std::vector<ColTable>& outResult) {
    outResult.clear();
    
    // 1. Safety check
    if (table->getField()->Format != format::STRUCT) {
        throw std::runtime_error("Table format is not a STRUCT!");
    }
    
    auto data = table->getData();
    if (data->children.empty()) return;
    auto field = table->getField();
    
    size_t len=table->data->length;
    for (int i=0;i<len;i++){
        outResult.push_back(ColTable(field->children[i],data->children[i]));
    }
    
}
// --- Explicitly Sized Signed Integers ---

template<>
inline ColTable ColTable::FromVector<int8_t>(const std::vector<int8_t>& vec) {
    auto field = std::make_shared<ColTableField>("INT8", format::INT8);
    size_t byteSize = vec.size() * sizeof(int8_t);
    auto buffer = std::make_shared<Buffer>(byteSize);
    std::memcpy(buffer->get(), vec.data(), byteSize);
    auto data  = std::make_shared<ColTableData>(vec.size());
    data->addBuffer(buffer);
    data->nullCount = -1; 
    return ColTable(field, data);
}

template <>
inline void ColTable::convertToArray(std::shared_ptr<ColTable> colTable, std::vector<int8_t>& outResult) {
    outResult.clear();
    auto dataPtr = colTable->getData()->valueBuffer;
    if (!dataPtr) return;

    int8_t* buffer = static_cast<int8_t*>(dataPtr->get());
    size_t count = colTable->getData()->length;
    outResult.resize(count);
    for (size_t i = 0; i < count; i++) outResult[i] = buffer[i];
}

template<>
inline ColTable ColTable::FromVector<int16_t>(const std::vector<int16_t>& vec) {
    auto field = std::make_shared<ColTableField>("INT16", format::INT16);
    size_t byteSize = vec.size() * sizeof(int16_t);
    auto buffer = std::make_shared<Buffer>(byteSize);
    std::memcpy(buffer->get(), vec.data(), byteSize);
    auto data  = std::make_shared<ColTableData>(vec.size());
    data->addBuffer(buffer);
    data->nullCount = -1; 
    return ColTable(field, data);
}

template <>
inline void ColTable::convertToArray(std::shared_ptr<ColTable> colTable, std::vector<int16_t>& outResult) {
    outResult.clear();
    auto dataPtr = colTable->getData()->valueBuffer;
    if (!dataPtr) return;

    int16_t* buffer = static_cast<int16_t*>(dataPtr->get());
    size_t count = colTable->getData()->length;
    outResult.resize(count);
    for (size_t i = 0; i < count; i++) outResult[i] = buffer[i];
}

template<>
inline ColTable ColTable::FromVector<int32_t>(const std::vector<int32_t>& vec) {
    auto field = std::make_shared<ColTableField>("INT32", format::INT32);
    size_t byteSize = vec.size() * sizeof(int32_t);
    auto buffer = std::make_shared<Buffer>(byteSize);
    std::memcpy(buffer->get(), vec.data(), byteSize);
    auto data  = std::make_shared<ColTableData>(vec.size());
    data->addBuffer(buffer);
    data->nullCount = -1; 
    return ColTable(field, data);
}

template <>
inline void ColTable::convertToArray(std::shared_ptr<ColTable> colTable, std::vector<int32_t>& outResult) {
    outResult.clear();
    auto dataPtr = colTable->getData()->valueBuffer;
    if (!dataPtr) return;

    int32_t* buffer = static_cast<int32_t*>(dataPtr->get());
    size_t count = colTable->getData()->length;
    outResult.resize(count);
    for (size_t i = 0; i < count; i++) outResult[i] = buffer[i];
}

template<>
inline ColTable ColTable::FromVector<int64_t>(const std::vector<int64_t>& vec) {
    auto field = std::make_shared<ColTableField>("INT64", format::INT64);
    size_t byteSize = vec.size() * sizeof(int64_t);
    auto buffer = std::make_shared<Buffer>(byteSize);
    std::memcpy(buffer->get(), vec.data(), byteSize);
    auto data  = std::make_shared<ColTableData>(vec.size());
    data->addBuffer(buffer);
    data->nullCount = -1; 
    return ColTable(field, data);
}

template <>
inline void ColTable::convertToArray(std::shared_ptr<ColTable> colTable, std::vector<int64_t>& outResult) {
    outResult.clear();
    auto dataPtr = colTable->getData()->valueBuffer;
    if (!dataPtr) return;

    int64_t* buffer = static_cast<int64_t*>(dataPtr->get());
    size_t count = colTable->getData()->length;
    outResult.resize(count);
    for (size_t i = 0; i < count; i++) outResult[i] = buffer[i];
}

// --- C++ Native Signed Integers ---

template<>
inline ColTable ColTable::FromVector<long long>(const std::vector<long long>& vec) {
    auto field = std::make_shared<ColTableField>("LONG_LONG", format::LONG_LONG);
    size_t byteSize = vec.size() * sizeof(long long);
    auto buffer = std::make_shared<Buffer>(byteSize);
    std::memcpy(buffer->get(), vec.data(), byteSize);
    auto data  = std::make_shared<ColTableData>(vec.size());
    data->addBuffer(buffer);
    data->nullCount = -1; 
    return ColTable(field, data);
}

template <>
inline void ColTable::convertToArray(std::shared_ptr<ColTable> colTable, std::vector<long long>& outResult) {
    outResult.clear();
    auto dataPtr = colTable->getData()->valueBuffer;
    if (!dataPtr) return;

    long long* buffer = static_cast<long long*>(dataPtr->get());
    size_t count = colTable->getData()->length;
    outResult.resize(count);
    for (size_t i = 0; i < count; i++) outResult[i] = buffer[i];
}

// --- Explicitly Sized Unsigned Integers ---

template<>
inline ColTable ColTable::FromVector<uint8_t>(const std::vector<uint8_t>& vec) {
    auto field = std::make_shared<ColTableField>("UINT8", format::UINT8);
    size_t byteSize = vec.size() * sizeof(uint8_t);
    auto buffer = std::make_shared<Buffer>(byteSize);
    std::memcpy(buffer->get(), vec.data(), byteSize);
    auto data  = std::make_shared<ColTableData>(vec.size());
    data->addBuffer(buffer);
    data->nullCount = -1; 
    return ColTable(field, data);
}

template <>
inline void ColTable::convertToArray(std::shared_ptr<ColTable> colTable, std::vector<uint8_t>& outResult) {
    outResult.clear();
    auto dataPtr = colTable->getData()->valueBuffer;
    if (!dataPtr) return;

    uint8_t* buffer = static_cast<uint8_t*>(dataPtr->get());
    size_t count = colTable->getData()->length;
    outResult.resize(count);
    for (size_t i = 0; i < count; i++) outResult[i] = buffer[i];
}

template<>
inline ColTable ColTable::FromVector<uint16_t>(const std::vector<uint16_t>& vec) {
    auto field = std::make_shared<ColTableField>("UINT16", format::UINT16);
    size_t byteSize = vec.size() * sizeof(uint16_t);
    auto buffer = std::make_shared<Buffer>(byteSize);
    std::memcpy(buffer->get(), vec.data(), byteSize);
    auto data  = std::make_shared<ColTableData>(vec.size());
    data->addBuffer(buffer);
    data->nullCount = -1; 
    return ColTable(field, data);
}

template <>
inline void ColTable::convertToArray(std::shared_ptr<ColTable> colTable, std::vector<uint16_t>& outResult) {
    outResult.clear();
    auto dataPtr = colTable->getData()->valueBuffer;
    if (!dataPtr) return;

    uint16_t* buffer = static_cast<uint16_t*>(dataPtr->get());
    size_t count = colTable->getData()->length;
    outResult.resize(count);
    for (size_t i = 0; i < count; i++) outResult[i] = buffer[i];
}

template<>
inline ColTable ColTable::FromVector<uint32_t>(const std::vector<uint32_t>& vec) {
    auto field = std::make_shared<ColTableField>("UINT32", format::UINT32);
    size_t byteSize = vec.size() * sizeof(uint32_t);
    auto buffer = std::make_shared<Buffer>(byteSize);
    std::memcpy(buffer->get(), vec.data(), byteSize);
    auto data  = std::make_shared<ColTableData>(vec.size());
    data->addBuffer(buffer);
    data->nullCount = -1; 
    return ColTable(field, data);
}

template <>
inline void ColTable::convertToArray(std::shared_ptr<ColTable> colTable, std::vector<uint32_t>& outResult) {
    outResult.clear();
    auto dataPtr = colTable->getData()->valueBuffer;
    if (!dataPtr) return;

    uint32_t* buffer = static_cast<uint32_t*>(dataPtr->get());
    size_t count = colTable->getData()->length;
    outResult.resize(count);
    for (size_t i = 0; i < count; i++) outResult[i] = buffer[i];
}

template<>
inline ColTable ColTable::FromVector<uint64_t>(const std::vector<uint64_t>& vec) {
    auto field = std::make_shared<ColTableField>("UINT64", format::UINT64);
    size_t byteSize = vec.size() * sizeof(uint64_t);
    auto buffer = std::make_shared<Buffer>(byteSize);
    std::memcpy(buffer->get(), vec.data(), byteSize);
    auto data  = std::make_shared<ColTableData>(vec.size());
    data->addBuffer(buffer);
    data->nullCount = -1; 
    return ColTable(field, data);
}

template <>
inline void ColTable::convertToArray(std::shared_ptr<ColTable> colTable, std::vector<uint64_t>& outResult) {
    outResult.clear();
    auto dataPtr = colTable->getData()->valueBuffer;
    if (!dataPtr) return;

    uint64_t* buffer = static_cast<uint64_t*>(dataPtr->get());
    size_t count = colTable->getData()->length;
    outResult.resize(count);
    for (size_t i = 0; i < count; i++) outResult[i] = buffer[i];
}

// --- C++ Native Unsigned Integers ---

template<>
inline ColTable ColTable::FromVector<unsigned long long>(const std::vector<unsigned long long>& vec) {
    auto field = std::make_shared<ColTableField>("UNSIGNED_LONG_LONG", format::UNSIGNED_LONG_LONG);
    size_t byteSize = vec.size() * sizeof(unsigned long long);
    auto buffer = std::make_shared<Buffer>(byteSize);
    std::memcpy(buffer->get(), vec.data(), byteSize);
    auto data  = std::make_shared<ColTableData>(vec.size());
    data->addBuffer(buffer);
    data->nullCount = -1; 
    return ColTable(field, data);
}

template <>
inline void ColTable::convertToArray(std::shared_ptr<ColTable> colTable, std::vector<unsigned long long>& outResult) {
    outResult.clear();
    auto dataPtr = colTable->getData()->valueBuffer;
    if (!dataPtr) return;

    unsigned long long* buffer = static_cast<unsigned long long*>(dataPtr->get());
    size_t count = colTable->getData()->length;
    outResult.resize(count);
    for (size_t i = 0; i < count; i++) outResult[i] = buffer[i];
}

// --- Floating Point ---

template<>
inline ColTable ColTable::FromVector<float>(const std::vector<float>& vec) {
    auto field = std::make_shared<ColTableField>("FLOAT", format::FLOAT);
    size_t byteSize = vec.size() * sizeof(float);
    auto buffer = std::make_shared<Buffer>(byteSize);
    std::memcpy(buffer->get(), vec.data(), byteSize);
    auto data  = std::make_shared<ColTableData>(vec.size());
    data->addBuffer(buffer);
    data->nullCount = -1; 
    return ColTable(field, data);
}

template <>
inline void ColTable::convertToArray(std::shared_ptr<ColTable> colTable, std::vector<float>& outResult) {
    outResult.clear();
    auto dataPtr = colTable->getData()->valueBuffer;
    if (!dataPtr) return;

    float* buffer = static_cast<float*>(dataPtr->get());
    size_t count = colTable->getData()->length;
    outResult.resize(count);
    for (size_t i = 0; i < count; i++) outResult[i] = buffer[i];
}

template<>
inline ColTable ColTable::FromVector<double>(const std::vector<double>& vec) {
    auto field = std::make_shared<ColTableField>("DOUBLE", format::DOUBLE);
    size_t byteSize = vec.size() * sizeof(double);
    auto buffer = std::make_shared<Buffer>(byteSize);
    std::memcpy(buffer->get(), vec.data(), byteSize);
    auto data  = std::make_shared<ColTableData>(vec.size());
    data->addBuffer(buffer);
    data->nullCount = -1; 
    return ColTable(field, data);
}

template <>
inline void ColTable::convertToArray(std::shared_ptr<ColTable> colTable, std::vector<double>& outResult) {
    outResult.clear();
    auto dataPtr = colTable->getData()->valueBuffer;
    if (!dataPtr) return;

    double* buffer = static_cast<double*>(dataPtr->get());
    size_t count = colTable->getData()->length;
    outResult.resize(count);
    for (size_t i = 0; i < count; i++) outResult[i] = buffer[i];
}

// --- Strings ---

static size_t getLength(const std::vector<std::optional<std::string>>& vec) {
    size_t totalStringLength = 0;
    for (const auto& v : vec) {
        if(v.has_value()){
            totalStringLength += v->length();
        }
    }
    return totalStringLength;
}

template<>
inline ColTable ColTable::FromVector<std::optional<std::string>>(const std::vector<std::optional<std::string>>& vec) {
    size_t totalStringLength = getLength(vec);
    size_t offset_bytes = totalStringLength + sizeof(int64_t);
    size_t validitybitmap=0;

    auto offsetBuffersize = (vec.size() + 1) * sizeof(int64_t);
    auto offsetBuffer = std::make_shared<Buffer>(offsetBuffersize);
    int64_t* offsetBuffer_ptr = static_cast<int64_t*>(offsetBuffer->get());
    
    auto buffer = std::make_shared<Buffer>(offset_bytes);
    char* buffer_ptr = static_cast<char*>(buffer->get());
    int64_t offset = 0;

    for (size_t i = 0; i < vec.size(); ++i) {
        offsetBuffer_ptr[i] = offset;
        if(vec[i].has_value()){
            std::memcpy((buffer_ptr + offset), vec[i]->data(), vec[i]->size());
            offset += vec[i]->size();
            validitybitmap =(validitybitmap<<1) |1;
        }else{
            validitybitmap= (validitybitmap<<1) |0;
        }
    } 
    offsetBuffer_ptr[vec.size()] = offset;

    auto field = std::make_shared<ColTableField>("STRING", format::STRING);
    auto data  = std::make_shared<ColTableData>(vec.size());
    
    data->addBuffer(buffer);
    data->addOffsetBuffer(offsetBuffer);
    data->validityBitmapsBuffer=validitybitmap;
    data->nullCount = -1; 
    return ColTable(field, data);
}

template <>
inline void ColTable::convertToArray(std::shared_ptr<ColTable> colTable, std::vector<std::optional<std::string>>& outResult) {
    outResult.clear();
    auto dataPtr = colTable->getData()->valueBuffer;
    auto offsetPtr = colTable->getData()->offsetBuffer;
    size_t validitybitmap = colTable->getData()->validityBitmapsBuffer;
    
    if (!dataPtr || !offsetPtr) return;
    
    char* buffer = static_cast<char*>(dataPtr->get());
    int64_t* offsetBuffer = static_cast<int64_t*>(offsetPtr->get());
    size_t count = colTable->getData()->length;
    
    outResult.resize(count);
    for (size_t i = 0; i < count; i++) {
        // Optimization: C++ string constructor instantly builds from a start pointer and length
        size_t start = offsetBuffer[i];
        size_t end = offsetBuffer[i+1];
        bool is_valid = (validitybitmap >> (count - 1 - i)) & 1;
        if(!is_valid){
            outResult[i] = std::nullopt;
        }else{
            outResult[i] = std::string(buffer + start, end - start);
        }
    }
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
inline ColTable ColTable::FromVector(const std::list<T>& lst) {
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
template <typename T>
inline ColTable ColTable::FromVector(const std::vector<std::list<T>>& nestedList) {
    size_t length = nestedList.size();
    
    // 1. Calculate total elements across all lists
    size_t totalInnerElements = 0;
    for (const auto& lst : nestedList) {
        totalInnerElements += lst.size(); 
    }

    // 2. Set up the offset buffer
    auto offsetBufferSize = (length + 1) * sizeof(int64_t);
    auto offsetBuffer = std::make_shared<Buffer>(offsetBufferSize);
    int64_t* offsets = static_cast<int64_t*>(offsetBuffer->get());
    
    // 3. Flatten the data into a 1D vector
    std::vector<T> flattened;
    flattened.reserve(totalInnerElements);
    
    int64_t currentOffset = 0;
    for (size_t i = 0; i < length; ++i) {
        offsets[i] = currentOffset;
        currentOffset += nestedList[i].size();
        // Extract items from the list into the flat vector
        for (const auto& item : nestedList[i]) {
            flattened.push_back(item);
        }
    }
    offsets[length] = currentOffset; 

    // 4. Pass the flattened 1D data to the standard FromVector
    ColTable childTable = ColTable::FromVector<T>(flattened);
    
    // 5. Build the parent Table (treated as an Array of Arrays)
    auto parentField = std::make_shared<ColTableField>("AoA", format::AoA);
    parentField->addChild(childTable.getField()); 
    
    auto parentData = std::make_shared<ColTableData>(length);
    parentData->addOffsetBuffer(offsetBuffer);
    parentData->addChild(childTable.getData());   
    parentData->nullCount = -1;
    parentData->length = length; 

    return ColTable(parentField, parentData);
}
template <typename T>
inline void ColTable::convertToArray(std::shared_ptr<ColTable> table, std::vector<std::list<T>>& outResult) {
    outResult.clear();
    auto data = table->getData();
    auto offsetPtr = data->offsetBuffer;
    
    // 1. Safety checks
    if (table->getField()->Format != format::AoA) {
        throw std::runtime_error("Table is not an Array of Arrays!");
    }
    if (!offsetPtr) return;
    
    const int64_t* offsetBuffer = static_cast<const int64_t*>(offsetPtr->get());
    size_t count = data->length; 
    
    auto childTable = std::make_shared<ColTable>(table->getField()->children[0], data->children[0]);
    
    // 2. Recursively extract the 1D flat vector
    std::vector<T> flatVec;
    ColTable::convertToArray(childTable, flatVec);

    // 3. Reconstruct the lists using the offsets
    outResult.reserve(count);
    for (size_t i = 0; i < count; i++) {
        size_t start = offsetBuffer[i];
        size_t end = offsetBuffer[i+1];
        
        // Construct the list from the flat vector iterators
        outResult.emplace_back(flatVec.begin() + start, flatVec.begin() + end);
    }
}
#endif //COLTABLE_INTERFACE