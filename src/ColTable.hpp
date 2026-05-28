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
#include <sstream>
#include <typeindex>
#include <typeinfo>
#include <numeric>

// Overload the << operator for ANY std::optional
template <typename T>
std::ostream &operator<<(std::ostream &os, const std::optional<T> &opt)
{
    if (opt.has_value())
    {
        os << opt.value();
    }
    else
    {
        os << "[std::nullopt]";
    }
    return os;
}
enum class format
{
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

    BOOL,

    // --- Variable-Length & Complex Types ---
    STRING,
    LIST,
    AoA,
    STRUCT,
    DICT,

    // --- Fallback ---
    UNKNOWN
};

class Buffer
{
private:
    // raw_ptr is the allocated buffer location later call free to remove
    void *raw_ptr = nullptr;
    // the data stored starts here
    void *aligned_ptr = nullptr;

public:
    // same as raw_ptr
    void *buffer;
    size_t bufferSize;
    size_t capacity;
    const int& operator[](size_t index) {
        return static_cast<int*>(aligned_ptr)[index];
    }

    Buffer(size_t size) : bufferSize(size)
    {
        capacity = (size == 0) ? 1 : size * 2;
        // Alignment for padding
        size_t alignment = 64;
        // Size of buffer + end padding + size of pointer to store the address of the aligned_ptr
        size_t total_cap = alignment + capacity + sizeof(void *);
        // Allocate buffer of size total_size
        raw_ptr = std::malloc(total_cap);
        if (!raw_ptr)
            return;
        // get the raw address of starting address
        std::uintptr_t starting_address = reinterpret_cast<std::uintptr_t>(raw_ptr) + sizeof(void *);
        // assign the first aligned_ptr, aligned_addr=(starting_address+alignment-1) & (alignment-1)
        std::uintptr_t aligned_addr = reinterpret_cast<std::uintptr_t>(starting_address + alignment - 1) & ~(alignment - 1);
        // aligned_ptr

        aligned_ptr = reinterpret_cast<void *>(aligned_addr);
        buffer = static_cast<void **>(aligned_ptr)[-1] = raw_ptr;
    }
    // Move constructor
    Buffer(Buffer &&other)
    {
        std::swap(other.raw_ptr, raw_ptr);
        std::swap(other.aligned_ptr, aligned_ptr);
        std::swap(other.buffer, buffer);
        std::swap(other.bufferSize, bufferSize);
        std::swap(other.capacity, capacity);
    }
    // Move assignment operator
    Buffer &operator=(Buffer &&other)
    {
        if (this != &other)
        {
            std::swap(raw_ptr, other.raw_ptr);
            std::swap(aligned_ptr, other.aligned_ptr);
            std::swap(buffer, other.buffer);
            std::swap(bufferSize, other.bufferSize);
            std::swap(other.capacity, capacity);
        }
        return *this;
    }
    // Standard Destructor
    ~Buffer()
    {
        if (raw_ptr)
        {
            std::free(raw_ptr);
        }
    }
    void insert_at(size_t start_index, void *data, size_t input_size)
    {   
        
        if (input_size == 0)
            return;
        if (start_index > bufferSize)
        {
            throw std::runtime_error("Buffer::insert_at start_index > bufferSize");
        }
        int64_t new_bufferSize = input_size + bufferSize;
        if (new_bufferSize > capacity)
        {   
            Buffer next(new_bufferSize);
            uint8_t *target = static_cast<uint8_t *>(next.get());
            uint8_t *old_src = static_cast<uint8_t *>(aligned_ptr);
            if (old_src)
            {
                
                std::memcpy(target, old_src, start_index);
                
                std::memcpy(target + start_index + input_size, old_src + start_index, bufferSize - start_index);
            }
            if (data)
            {
                
                std::memcpy(target + start_index, data, input_size);
            }
            else
            {
                std::memset(target + start_index, 0, input_size);
            }
            
            next.bufferSize = new_bufferSize;
            *this = std::move(next);
        }
        else
        {
            uint8_t *old_src = static_cast<uint8_t *>(aligned_ptr);

            std::memmove(old_src + start_index + input_size, old_src + start_index, bufferSize - start_index);
            if (data)
            {
                std::memmove(old_src + start_index, data, input_size);
            }
            else
            {
                std::memset(old_src + start_index, 0, input_size);
            }
            bufferSize = new_bufferSize;
        }
    }
    void remove_at(size_t index_start, size_t range)
    {   
        #ifdef DEBUG
        std::cout << "index_startr" << index_start << std::endl;
        std::cout << "ranger" << range << std::endl;
        std::cout << "bufferr" << bufferSize << std::endl;
        #endif
        if (index_start + range > bufferSize)
        {
            std::ostringstream oss;
            oss << "index_start+range>bufferSize" << " index_start: " << index_start << " range: " << range << " bufferSize: " << bufferSize;
            throw std::runtime_error(oss.str());
            return;
        }

        char *data = static_cast<char *>(aligned_ptr);
        std::memmove(&data[index_start], &data[index_start + range], (bufferSize - range - index_start));
        bufferSize -= range;
    }
    void *get() const { return aligned_ptr; }

    // Disable copying (to prevent double-free)
    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer &) = delete;

    template <typename T = void>
    T *as(format fmt) const
    {
        // We use reinterpret_cast because we are forcing a
        // specific type interpretation onto raw memory.
        switch (fmt)
        {
        case format::INT32:
            return reinterpret_cast<T *>(aligned_ptr);
        case format::INT64:
            return reinterpret_cast<T *>(aligned_ptr);
        case format::FLOAT:
            return reinterpret_cast<T *>(aligned_ptr);
        case format::DOUBLE:
            return reinterpret_cast<T *>(aligned_ptr);
        case format::STRING:
            return reinterpret_cast<T *>(aligned_ptr);
        case format::LIST:
        case format::STRUCT:
        case format::DICT:
            return reinterpret_cast<T *>(aligned_ptr);
        default:
            return nullptr;
        }
    }

    // Helper for direct indexing after casting
    template <typename T>
    T &at(size_t index) const
    {
        return static_cast<T *>(aligned_ptr)[index];
    }
};

class ColTableField
{
public:
    std::string key;

    format Format;

    std::vector<std::shared_ptr<ColTableField>> children;

    int64_t nChildren;

    std::type_index expected_type;

    ColTableField(std::string k, format f, const std::type_info &t)
        : key(k), Format(f), expected_type(t)
    {   
        #ifdef DEBUG
        std::cout << expected_type.name() << std::endl;
        #endif
    }

    virtual ~ColTableField() = default;
    // Copy constructor
    ColTableField(ColTableField const &other) : expected_type(other.expected_type)
    {

        Format = other.Format;
        key = other.key;
        nChildren = other.nChildren;
        if (!other.children.empty())
        {
            children.reserve(other.children.size());
            for (const auto &ptr : other.children)
            {
                if (ptr)
                {
                    children.push_back(std::make_shared<ColTableField>(*ptr));
                }
                else
                {
                    children.push_back(nullptr);
                }
            }
        }
    }
    // Move Constructor
    ColTableField(ColTableField &&other) : expected_type(other.expected_type)
    {
        std::swap(other.key, key);
        std::swap(other.Format, Format);
        std::swap(other.children, children);
        std::swap(other.nChildren, nChildren);
        expected_type = other.expected_type;
    }
    // Move assignment operator
    ColTableField &operator=(ColTableField &&other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }
        std::swap(other.key, key);
        std::swap(other.Format, Format);
        std::swap(other.children, children);
        std::swap(other.nChildren, nChildren);
        expected_type = other.expected_type;
        return *this;
    }
    // Copy assignment operator
    ColTableField &operator=(ColTableField const &other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }
        Format = other.Format;
        key = other.key;
        nChildren = other.nChildren;
        children.clear();
        expected_type = other.expected_type;
        if (!other.children.empty())
        {

            children.reserve(other.children.size());
            for (const auto &ptr : other.children)
            {
                if (ptr)
                {
                    children.push_back(std::make_shared<ColTableField>(*ptr));
                }
                else
                {
                    children.push_back(nullptr);
                }
            }
        }
        return *this;
    }
    void addChild(const std::shared_ptr<ColTableField> child)
    {
        if (child)
        {
            children.push_back(child);
            nChildren = children.size();
        }
    }
    void removeChild(int64_t index)
    {
        children.erase(children.begin() + index);
        nChildren = children.size();
    }
    // Helper to print the tree structure recursively
    void print(int indent = 0) const
    {
        std::string spaces(indent, ' ');
        std::cout << spaces << "|-- " << key << " (Children: " << nChildren << ")\n";
        for (const auto &child : children)
        {
            child->print(indent + 4);
        }
    }
};
template <typename T>
struct FlatBufferView {
    T* data = nullptr;
    size_t length = 0;
    std::shared_ptr<Buffer> owner;

    FlatBufferView() = default;

    FlatBufferView(
        T* data_,
        size_t length_,
        std::shared_ptr<Buffer> owner_
    )
        : data(data_),
          length(length_),
          owner(std::move(owner_))
    {}
};
template <typename T>
struct View {
    T* values = nullptr;
    size_t rows = 0;
    size_t total_length = 0;

    std::vector<int64_t> row_starts;
    std::vector<int64_t> row_lengths;

    std::shared_ptr<Buffer> value_owner;
    std::shared_ptr<Buffer> offset_owner;

    View() = default;

    View(
        T* values_,
        size_t total_length_,
        int64_t* offsets,
        size_t rows_,
        std::shared_ptr<Buffer> value_owner_,
        std::shared_ptr<Buffer> offset_owner_
    )
        : values(values_),
          rows(rows_),
          total_length(total_length_),
          value_owner(std::move(value_owner_)),
          offset_owner(std::move(offset_owner_))
    {   
        std::cout << "View constructor\n";
        row_starts.resize(rows);
        row_lengths.resize(rows);

        for (size_t i = 0; i < rows; ++i) {
            row_starts[i] = offsets[i];
            row_lengths[i] = offsets[i + 1] - offsets[i];
        }
    }

    T get(int64_t i, int64_t j) const {
        int64_t nrows = static_cast<int64_t>(rows);

        if (i < 0) {
            i += nrows;
        }

        if (i < 0 || i >= nrows) {
            throw std::out_of_range("row index out of range");
        }

        int64_t row_len = row_lengths[i];

        if (j < 0) {
            j += row_len;
        }

        if (j < 0 || j >= row_len) {
            throw std::out_of_range("column index out of range");
        }

        return values[row_starts[i] + j];
    }
};
struct StringView
{
    char *data_ptr;         // Start of the giant char buffer
    int64_t *offsets_ptr;   // Start of the offset buffer
    size_t length;          // Number of strings in this view
    uint8_t *validity_bits; // The bitmap for nulls

    // Returns a Python-friendly string or None
    std::optional<std::string> get_item(size_t i) const
    {
        if (i >= length)
            return std::nullopt;

        // Check validity bitmap
        bool is_valid = validity_bits[i];
        if (!is_valid)
            return std::nullopt;

        size_t start = offsets_ptr[i];
        size_t end = offsets_ptr[i + 1];
        return std::string(data_ptr + start, end - start);
    }
};
class ColTableData
{
public:
    std::vector<std::shared_ptr<ColTableData>> children;

    size_t length;

    int64_t nullCount;

    std::shared_ptr<Buffer> validityBitmapsBuffer;

    int64_t nChildren;

    std::shared_ptr<Buffer> offsetBuffer;
    std::shared_ptr<Buffer> valueBuffer;

    ColTableData(int64_t length) : length(length), nullCount(0) {};
    // Copy constructor
    ColTableData(ColTableData const &other)
    {
        children = other.children;
        length = other.length;
        nullCount = other.nullCount;
        validityBitmapsBuffer = other.validityBitmapsBuffer;
        nChildren = other.nChildren;
        if (!other.children.empty())
        {
            children.reserve(other.children.size());
            for (const auto &ptr : other.children)
            {
                if (ptr)
                {
                    children.push_back(std::make_shared<ColTableData>(*ptr));
                }
                else
                {
                    children.push_back(nullptr);
                }
            }
        }
        if (other.offsetBuffer)
        {
            offsetBuffer = (other.offsetBuffer);
        }
        if (other.valueBuffer)
        {
            valueBuffer = (other.valueBuffer);
        }
    }
    // Move Constructor
    ColTableData(ColTableData &&other)
    {
        std::swap(other.children, children);
        std::swap(other.length, length);
        std::swap(other.nullCount, nullCount);
        std::swap(other.validityBitmapsBuffer, validityBitmapsBuffer);
        std::swap(other.valueBuffer, valueBuffer);
        std::swap(other.offsetBuffer, offsetBuffer);
    }
    // Move assignment operator
    ColTableData &operator=(ColTableData &&other)
    {
        if (this == &other)
        {
            return *this;
        }
        std::swap(other.children, children);
        std::swap(other.length, length);
        std::swap(other.nullCount, nullCount);
        std::swap(other.validityBitmapsBuffer, validityBitmapsBuffer);
        std::swap(other.valueBuffer, valueBuffer);
        std::swap(other.offsetBuffer, offsetBuffer);
        return *this;
    }
    // Copy assignment operator
    ColTableData &operator=(ColTableData const &other)
    {
        if (this == &other)
        {
            return *this;
        }
        children = other.children;
        length = other.length;
        validityBitmapsBuffer = other.validityBitmapsBuffer;
        if (!other.children.empty())
        {
            children.reserve(other.children.size());
            for (const auto &ptr : other.children)
            {
                if (ptr)
                {
                    children.push_back(std::make_shared<ColTableData>(*ptr));
                }
                else
                {
                    children.push_back(nullptr);
                }
            }
        }
        if (other.valueBuffer)
        {
            valueBuffer = (other.valueBuffer);
        }
        if (other.offsetBuffer)
        {
            offsetBuffer = (other.offsetBuffer);
        }
        return *this;
    }
    /*
     * indices is the array numbering of start_index
     * v_data  represents the buffer of flattenned vector
     * v_data_size represents the bufferSize of flattened vector
     * o_data represents the offsetBuffer of each element in vector
     * o_data_size represents the bufferSize of offsetBuffer
     * depth is to count the number of recursion
     */
    void insert_at(std::vector<int64_t> &indices, void *v_data, size_t v_data_size, int64_t *o_data, size_t o_data_size, int depth = 0)
    {
        if (depth >= indices.size())
            return;
        int64_t start_index = indices[depth];
        if (start_index > length)
        {
            start_index = length;
        }
        if (nChildren <= 0)
        {
            if (start_index <= length)
            {
                if (offsetBuffer && valueBuffer)
                {
                    // String

                    int64_t *ptr = static_cast<int64_t *>(offsetBuffer->get());

                    int64_t offset_start = ptr[start_index];
                    if (start_index < length)
                    {
                        for (size_t i = start_index + 1; i < length + 1; ++i)
                        {
                            ptr[i] += v_data_size;
                        }
                    }

                    if (o_data_size > 0 && o_data)
                    {
                        for (size_t i = 0; i < o_data_size; i++)
                        {
                            o_data[i] += offset_start;
                        }
                        offsetBuffer->insert_at((start_index + 1) * sizeof(int64_t), o_data, o_data_size * sizeof(int64_t));
                    }

                    if (v_data_size > 0)
                    {
                        valueBuffer->insert_at(offset_start, v_data, v_data_size);
                    }
                }
                else if (!offsetBuffer && children.empty())
                {   
                    if (!valueBuffer) {
                        valueBuffer = std::make_shared<Buffer>(v_data_size);
                    }
                    // primitive types
                    size_t element_byte_size = (o_data_size > 0) ? (v_data_size / o_data_size) : 0;
                    valueBuffer->insert_at(start_index * element_byte_size, v_data, v_data_size);
                }
                length += o_data_size;
            }
        }
        else
        {
            // recursive
            if (offsetBuffer && !children.empty())
            {
                // AoA
                // AoA flattened the child into first child, and the parent will contain offsetBuffer and one child
                // get the offset of offsetbuffer
           
                int64_t *ptr = static_cast<int64_t *>(offsetBuffer->get());

                int64_t offset_start = ptr[start_index];

                std::vector<int64_t> child_indices = indices;

                if (depth + 1 < child_indices.size())
                {
                    child_indices[depth + 1] = offset_start;
                }
                else
                {
                    child_indices.push_back(offset_start);
                }

            #ifdef DEBUG
                for (int i = 0; i <= start_index; i++)
                {
                    std::cout << "START_INDEX" << ptr[i] << std::endl;
                }
            #endif

                int64_t elements_inserted = 0;

                if (o_data && o_data_size > 0)
                {
                    // Your o_data is [row1_count, row1+row2_count, ...]
                    // Example: [50, 80]
                    // So total inserted child elements is the last value.
                    elements_inserted = o_data[o_data_size - 1];
                }

                if (start_index < length)
                {
                    // Do not shift ptr[start_index].
                    // It is the insertion anchor.
                    for (size_t i = start_index + 1; i < length + 1; ++i)
                    {
                        ptr[i] += elements_inserted;
                    }
                }

                if (o_data && o_data_size > 0)
                {
                    std::vector<int64_t> shifted_o_data(o_data, o_data + o_data_size);

                    for (size_t i = 0; i < o_data_size; i++)
                    {
                        shifted_o_data[i] += offset_start;
                    }

            #ifdef DEBUG
                    for (int i = 0; i < shifted_o_data.size(); i++)
                    {
                        std::cout << "Shifted_o_data" << shifted_o_data[i] << std::endl;
                    }
            #endif

                    offsetBuffer->insert_at(
                        (start_index + 1) * sizeof(int64_t),
                        shifted_o_data.data(),
                        shifted_o_data.size() * sizeof(int64_t)
                    );
                }

            #ifdef DEBUG
                std::cout << "original length" << length << std::endl;
            #endif

                length += o_data_size;

            #ifdef DEBUG
                std::cout << "new length" << length << std::endl;
            #endif

                if (children[0]->offsetBuffer && children[0]->valueBuffer)
                {
                    // Do not handle AoA<string> here.
                    // Use the special 2D string overload.
                    throw std::runtime_error(
                        "AoA<string> insert should use the dedicated 2D string insert path"
                    );
                }
                else
                {
                    // Child is primitive flat value column.
                    // Pass elements_inserted as o_data_size so primitive insert can calculate element size.
                    children[0]->insert_at(
                        child_indices,
                        v_data,
                        v_data_size,
                        nullptr,
                        elements_inserted,
                        depth + 1
                    );
                }
            }
            else if (!children.empty())
            {
                // Struct
                // no OffsetBuffer
                for (auto &child : children)
                {
                    child->insert_at(indices, v_data, v_data_size, o_data, o_data_size, depth);
                }
            }
        }
    }
    /*
     * indices is the array numbering of start_index
     * range represents the range of removal
     * depth is to count the number of recursion
     */
    void remove_at(std::vector<int64_t> &indices, size_t range, int depth = 0)
    {
        if (depth >= indices.size())
        {
            std::ostringstream oss;
            oss << "depth >= indices.size()" << " depth: " << depth << " indices size: " << indices.size();
            throw std::runtime_error(oss.str());
            return;
        }
        int64_t start_index = indices[depth];
        if (nChildren <= 0)
        {
            if (start_index + range <= length+1)
            {
                if (offsetBuffer && valueBuffer)
                {
                    // String
                    int64_t *ptr = static_cast<int64_t *>(offsetBuffer->get());
                    int64_t offset_start = ptr[start_index];
                    int64_t offset_end = ptr[start_index + range];
                    int64_t offset_range = offset_end - offset_start;
                    if ((start_index + range) > length)
                    {
                        std::cout << "CRITICAL: Attempting to remove range beyond length!" << std::endl;
                        return;
                    }
                    if (start_index < length)
                    {
                        for (size_t i = start_index + range + 1; i <= length; ++i)
                        {
                            ptr[i] -= offset_range;
                        }
                    }
                    #ifdef DEBUG
                    std::cout << "offset_start" << offset_start << std::endl;
                    std::cout << "offset_end" << offset_end << std::endl;
                    std::cout << "offset_range" << offset_range << std::endl;
                    #endif
                    offsetBuffer->remove_at((start_index + 1) * sizeof(int64_t), range * sizeof(int64_t));
                    #ifdef DEBUG
                    std::cout << "offset_range" << offset_range << std::endl;
                    #endif
                    valueBuffer->remove_at(offset_start, offset_range);
                }
                else if (!offsetBuffer && children.empty())
                {
                    // primitive types
                    if (!valueBuffer || length == 0) {
                        return; // Nothing to remove
                    }
                    size_t element_size = (length > 0) ? (valueBuffer->bufferSize / length) : 0;
                    valueBuffer->remove_at(start_index * element_size, range * element_size);
                }
                else
                {
                    return;
                }
            }
            else
            {
                std::ostringstream oss;
                oss << "start_index+range<=length" << " start_index: " << start_index << " range: " << range << " length:" << length;
                if(offsetBuffer){
                    char* ptr = static_cast<char*>(valueBuffer->get());
                    int64_t* offsetBuffer_ptr = static_cast<int64_t*>(offsetBuffer->get());
                    int64_t length = offsetBuffer_ptr[length];
                    int64_t count = 0;
                    for (size_t i = 0; i < length; ++i) {
                        #ifdef DEBUG
                        std::cout << ptr[i] << (i == offsetBuffer_ptr[count]-1 ? ", " : "");
                        std::cout << offsetBuffer_ptr[count] << (i == offsetBuffer_ptr[count]-1 ? ", " : "");
                        #endif
                        if (offsetBuffer_ptr[count] < i) {
                            count += 1;
                        }
                    }
                    length = length+1;
                    count = 0;
                    #ifdef DEBUG
                    std::cout <<std::endl;
                    std::cout <<"OffsetBuffer"<<length<<std::endl;
                    #endif
                    for (size_t i = 0; i < length; ++i) {
                        #ifdef DEBUG
                        std::cout << offsetBuffer_ptr[i] << ", ";
                        #endif
                        if (offsetBuffer_ptr[count] < i) {
                            count += 1;
                        }
                    }
                    
                }else{
                double* ptr = static_cast<double*>(valueBuffer->get());
                #ifdef DEBUG
                for (size_t i = 0; i < length; ++i) {
                    std::cout << ptr[i] << (i == length - 1 ? "" : ", ");
                }
                #endif
                }
                
                throw std::runtime_error(oss.str());
                return;
            }
            length -= range;
        }
        else
        {
            // recursive
            if (offsetBuffer && !children.empty())
            {
                // AoA
                //offsetbuffer ptr
                int64_t *ptr = static_cast<int64_t *>(offsetBuffer->get());
                #ifdef DEBUG
                //calculate offset start,end,range from offsetbuffer
                for (int i=0;i<=start_index+range;i++){
                    std::cout<<"START_INDEX"<<ptr[i]<<std::endl;
                }
                #endif
                int64_t offset_start = ptr[start_index];//should this is wrong, what will be the offset_start
                int64_t offset_end = ptr[start_index + range];
                int64_t offset_range = offset_end - offset_start;
                #ifdef DEBUG
                std::cout << "offset_start" << offset_start << std::endl;
                std::cout << "offset_end" << offset_end << std::endl;
                std::cout << "offset_range" << offset_range << std::endl;
                #endif
                std::vector<int64_t> child_indices = indices;
                
                if (depth + 1 < child_indices.size())
                {
                    child_indices[depth + 1] = offset_start;
                }
                else
                {
                    child_indices.push_back(offset_start);
                }
                #ifdef DEBUG
                for (int i=0;i<child_indices.size();i++){
                    std::cout<<"CHILD_INDICES"<<child_indices[i]<<std::endl;
                }
                #endif

                //remove value from child
                children[0]->remove_at(child_indices, offset_range, depth + 1);

                // remove from the offsetBuffer
                offsetBuffer->remove_at((start_index + 1) * sizeof(int64_t), range * sizeof(int64_t));

                // re-fetch pointer because remove_at might have resized/moved memory!
                ptr = static_cast<int64_t *>(offsetBuffer->get());

                // 4. SHIFT the remaining trailing offsets DOWN
                if (start_index < length - range)
                {
                    for (size_t i = start_index + 1; i <= length - range; ++i)
                    {
                        ptr[i] -= offset_range;
                    }
                }
                #ifdef DEBUG
                std::cout<<"original length"<<length<<std::endl;
                #endif
                length -= range;
                #ifdef DEBUG
                std::cout<<"new length"<<length<<std::endl;
                #endif
            }
            else if (!children.empty())
            {
                // Struct
                // no OffsetBuffer
                for (auto &child : children)
                {
                    child->remove_at(indices, range, depth);
                }
            }
            else
            {
                return;
            }
        }
    }
    void addChild(const std::shared_ptr<ColTableData> child)
    {
        if (child)
        {
            children.push_back(child);
            nChildren = children.size();
        }
    }
    void removeChild(int64_t index)
    {

        children.erase(children.begin() + index);
        nChildren = children.size();
    }
    
    void addBuffer(std::shared_ptr<Buffer> buffer)
    {
        valueBuffer = buffer;
    }

    void addOffsetBuffer(std::shared_ptr<Buffer> buffer)
    {
        offsetBuffer = buffer;
    }
    void addValidityBuffer(std::shared_ptr<Buffer> buffer)
    {
        validityBitmapsBuffer = buffer;
    }
    std::shared_ptr<Buffer> getBuffer()
    {
        return valueBuffer;
    }
    std::shared_ptr<Buffer> getvalidityBitmapsBuffer()
    {
        return validityBitmapsBuffer;
    }
    virtual ~ColTableData() = default;
};

void printValueBuffer(const std::shared_ptr<ColTableData> &data, format type);

class ColTable
{
    std::type_index expected_type;

public:
    std::shared_ptr<ColTableField> field;

    std::shared_ptr<ColTableData> data;

    ColTable(std::shared_ptr<ColTableField> field, std::shared_ptr<ColTableData> data, const std::type_index t)
        : field(field), data(data), expected_type(t) {};

    ColTable(ColTable const &other) : expected_type(other.expected_type)
    {
        if (other.field)
        {
            field = std::make_shared<ColTableField>(*other.field);
        }
        if (other.data)
        {
            data = std::make_shared<ColTableData>(*other.data);
        }
    }

    ColTable(ColTable &&other) noexcept : expected_type(other.expected_type)
    {
        field = std::move(other.field);
        data = std::move(other.data);
    }

    ColTable &operator=(ColTable &&other) noexcept
    {
        if (this != &other)
        {
            field = std::move(other.field);
            data = std::move(other.data);
            expected_type = other.expected_type;
        }
        return *this;
    }

    ColTable &operator=(ColTable const &other)
    {
        if (this != &other)
        {
            if (other.field)
            {
                field = std::make_shared<ColTableField>(*other.field);
            }
            if (other.data)
            {
                data = std::make_shared<ColTableData>(*other.data);
            }
            expected_type = other.expected_type;
        }
        return *this;
    }
    std::type_index getTypeIndex() const
    {
        return expected_type;
    }

    void sync_length_with_children()
    {
        size_t max_child_len = 0;
        for (auto &child : this->data->children)
        {
            if (child && child->length > max_child_len)
            {
                max_child_len = child->length;
            }
        }
        this->data->length = max_child_len;
    }
    
    StringView get_string_view() const
    {
        if (!data->valueBuffer || !data->offsetBuffer)
            return {nullptr, nullptr, 0, 0};

        return {
            static_cast<char *>(data->valueBuffer->get()),
            static_cast<int64_t *>(data->offsetBuffer->get()),
            data->length,
            static_cast<uint8_t *>(data->validityBitmapsBuffer->get())};
    }
    
    template <typename T>
    View<T> make_view(int64_t index) const
    {
        if (!data || index < 0 || index >= static_cast<int64_t>(data->length)) {
            return {};
        }

        if (data->nChildren <= 0) {

            // String-like / variable-length data
            if (data->offsetBuffer && data->valueBuffer)
            {
                auto* offsets = static_cast<int64_t*>(data->offsetBuffer->get());

                int64_t start = offsets[index];
                int64_t end   = offsets[index + 1];

                auto* base = static_cast<T*>(data->valueBuffer->get());
                auto* ptr  = base + start;

                return View<T>(
                    ptr,
                    static_cast<size_t>(end - start),
                    data->valueBuffer       // keep value buffer alive
                );
            }

            // Primitive scalar
            if (data->valueBuffer)
            {
                auto* base = static_cast<T*>(data->valueBuffer->get());

                return View<T>(
                    base + index,
                    1,
                    data->valueBuffer       // keep value buffer alive
                );
            }
        }
        else {

            // Array of Arrays
            if (data->offsetBuffer && !data->children.empty())
            {
                auto& dataChild = data->children[0];

                if (!dataChild || !dataChild->valueBuffer) {
                    return {};
                }

                auto* offsets = static_cast<int64_t*>(data->offsetBuffer->get());

                int64_t start = offsets[index];
                int64_t end   = offsets[index + 1];

                auto* base = static_cast<T*>(dataChild->valueBuffer->get());
                auto* ptr  = base + start;

                return View<T>(
                    ptr,
                    static_cast<size_t>(end - start),
                    dataChild->valueBuffer  // keep child value buffer alive
                );
            }
            else {
                // Struct-like fixed-width data
                if (!data->valueBuffer) {
                    return {};
                }

                auto* base = static_cast<T*>(data->valueBuffer->get());

                return View<T>(
                    base + index,
                    1,
                    data->valueBuffer
                );
            }
        }

        return {};
    }
    void remove_at(std::vector<int64_t> &indices, size_t range)
    {
        this->data->remove_at(indices, range);
    }

    // For  String
    void insert_at(std::vector<int64_t> &indices, std::vector<std::optional<std::string>> &vec)
    {
        size_t total_bytes = 0;
        for (const auto &str : vec)
        {
            if (str.has_value())
            {
                total_bytes += str->size();
            }
        }
        std::vector<char> flat_buffer;
        flat_buffer.reserve(total_bytes);

        std::vector<int64_t> offsets;
        offsets.reserve(vec.size());
        std::vector<uint8_t> validity_bits;
        validity_bits.reserve(vec.size());
        int64_t current_offset = 0;
        for (const auto &str : vec)
        {
            if (str.has_value())
            {
                flat_buffer.insert(flat_buffer.end(), str->begin(), str->end());
                current_offset += str->size();
                offsets.push_back(current_offset);
                validity_bits.push_back(1);
            }
            else
            {
                offsets.push_back(current_offset);
                validity_bits.push_back(0);
            }
        }
        std::shared_ptr<Buffer> validitybitmap = this->data->validityBitmapsBuffer;
        if (validitybitmap)
        {
            validitybitmap->insert_at(
                indices[0],
                validity_bits.data(),
                validity_bits.size());
        }
        // Pass the flattened characters and the generated offsets down to the data buffers
        this->data->insert_at(
            indices,
            flat_buffer.data(),
            flat_buffer.size(),
            offsets.data(),
            offsets.size());
    }
    void insert_at(
        std::vector<int64_t>& indices,
        std::vector<std::vector<std::optional<std::string>>>& vec
    ) {
        if (this->field->Format != format::AoA) {
            throw std::runtime_error("insert_at 2D string requires AoA table");
        }

        if (this->data->children.empty() || !this->data->children[0]) {
            throw std::runtime_error("AoA string table missing child data");
        }

        if (this->field->children.empty() || !this->field->children[0]) {
            throw std::runtime_error("AoA string table missing child field");
        }

        if (this->field->children[0]->Format != format::STRING) {
            throw std::runtime_error("insert_at 2D string requires STRING child");
        }

        int64_t start_index = indices.empty() ? 0 : indices[0];

        if (start_index < 0) {
            start_index = 0;
        }

        if (start_index > static_cast<int64_t>(this->data->length)) {
            start_index = static_cast<int64_t>(this->data->length);
        }

        // ============================================================
        // 1. Build AoA parent offsets and flattened string vector
        // ============================================================

        std::vector<std::optional<std::string>> flat_strings;
        std::vector<int64_t> parent_offsets;

        parent_offsets.reserve(vec.size());

        int64_t string_count = 0;

        for (const auto& row : vec) {
            for (const auto& s : row) {
                flat_strings.push_back(s);
                string_count++;
            }

            // AoA parent offset = cumulative number of strings
            parent_offsets.push_back(string_count);
        }

        size_t rows_inserted = vec.size();
        int64_t strings_inserted = string_count;

        if (rows_inserted == 0) {
            return;
        }

        // ============================================================
        // 2. Update AoA parent offsetBuffer
        // ============================================================

        int64_t* parent_ptr =
            static_cast<int64_t*>(this->data->offsetBuffer->get());

        size_t old_parent_length = this->data->length;

        // This is where the inserted strings begin in child string column
        int64_t child_start = parent_ptr[start_index];

        std::vector<int64_t> shifted_parent_offsets;
        shifted_parent_offsets.reserve(parent_offsets.size());

        for (auto off : parent_offsets) {
            shifted_parent_offsets.push_back(off + child_start);
        }

        this->data->offsetBuffer->insert_at(
            (start_index + 1) * sizeof(int64_t),
            shifted_parent_offsets.data(),
            shifted_parent_offsets.size() * sizeof(int64_t)
        );

        // Re-fetch because insert_at may move memory
        parent_ptr =
            static_cast<int64_t*>(this->data->offsetBuffer->get());

        // Shift old trailing offsets by number of inserted strings
        for (size_t i = start_index + 1 + rows_inserted;
            i <= old_parent_length + rows_inserted;
            ++i) {
            parent_ptr[i] += strings_inserted;
        }

        this->data->length += rows_inserted;

        // ============================================================
        // 3. Let existing 1D string insert handle child string offsets
        // ============================================================

        ColTable child_table(
            this->field->children[0],
            this->data->children[0],
            this->field->children[0]->expected_type
        );

        std::vector<int64_t> child_indices = { child_start };

        child_table.insert_at(child_indices, flat_strings);
    }
    // For primitives
    template <typename T>
    void insert_at(std::vector<int64_t> &indices, std::vector<T> &vec)
    {
        if (expected_type != std::type_index(typeid(T)))
        {
            if (expected_type != typeid(T))
            {
                throw std::runtime_error(
                    "Type mismatch: Cannot insert '" + std::string(typeid(T).name()) +
                    "' into a column of expected type '" + std::string(expected_type.name()) + "'!");
            }
        }
        if constexpr (std::is_same_v<T, bool>)
        {
            // This block handles the bool case by calling the specialization
            bool *ptr = reinterpret_cast<bool *>(std::malloc(vec.size()));
            size_t i = 0;
            for (bool val : vec)
            {
                ptr[i++] = val ? 1 : 0;
            }

            this->data->insert_at(
                indices,
                ptr,        // This works! uint8_t has .data()
                vec.size(), // byte_size is 1:1 with size()
                nullptr,
                vec.size());
        }
        else
        {
            size_t byte_size = vec.size() * sizeof(T);
            this->data->insert_at(
                indices,
                vec.data(),
                byte_size,
                nullptr,
                vec.size());
        }
    }
    // For AoA
    template <typename T>
    void insert_at(std::vector<int64_t> &indices, std::vector<std::vector<T>> &vec)
    {
        std::vector<T> flat_elements;
        std::vector<int64_t> offsets;
        offsets.reserve(vec.size());

        int64_t current_offset = 0;
        if constexpr (std::is_same_v<T, bool>)
        {

            size_t total_bools = 0;
            for (const auto &inner_vec : vec)
            {
                total_bools += inner_vec.size();
            }

            bool *ptr = reinterpret_cast<bool *>(std::malloc(total_bools));

            size_t i = 0;
            for (const auto &inner_vec : vec)
            {

                for (bool val : inner_vec)
                {
                    ptr[i++] = (val ? 1 : 0);
                }
                current_offset += inner_vec.size();
                offsets.push_back(current_offset);
            }
            this->data->insert_at(
                indices, ptr,
                total_bools,
                offsets.data(),
                offsets.size());
            std::free(ptr);
        }
        else
        {
            for (const auto &inner_vec : vec)
            {
                flat_elements.insert(flat_elements.end(), inner_vec.begin(), inner_vec.end());
                current_offset += inner_vec.size();
                offsets.push_back(current_offset);
            }
            //offsets.push_back(current_offset);
            // Now that the 2D array is flattened to 1D, we calculate byte sizes
            size_t byte_size = flat_elements.size() * sizeof(T);
            
            // Call the recursive ColTableData function.
            // The AoA block inside ColTableData will handle routing these bytes to the child.
            this->data->insert_at(
                indices,
                flat_elements.data(),
                byte_size,
                offsets.data(),
                offsets.size());
        }
    }

    // For struct
    template <typename T, typename FieldType>
    void insert_struct_column(std::vector<int64_t> &indices,
                              std::vector<T> &vec,
                              const std::string &key,
                              FieldType T::*member_ptr)
    {
        
        if (expected_type != std::type_index(typeid(T)))
        {
            if (expected_type != typeid(T) && expected_type != typeid(void))
            {
                throw std::runtime_error(
                    "Type mismatch: Cannot insert '" + std::string(typeid(T).name()) +
                    "' into a column of expected type '" + std::string(expected_type.name()) + "'!");
            }
        }
        int target_child_index = -1;
        // find the key
        for (size_t i = 0; i < this->field->children.size(); ++i)
        {
            if (this->field->children[i] != nullptr && this->field->children[i]->key == key)
            {
                target_child_index = i;
                break;
            }
        }
        // key not found
        if (target_child_index == -1)
        {
            throw std::runtime_error("Column key not found in schema: " + key);
        }
        std::vector<FieldType> extracted_column;
        extracted_column.reserve(vec.size());
        for (const auto &row : vec)
        {
            // gets the specific variable
            extracted_column.push_back(row.*member_ptr);
        }
        ColTable child_table(
            this->field->children[target_child_index],
            this->data->children[target_child_index],
            typeid(FieldType));
        child_table.insert_at(indices, extracted_column);
        size_t updated_child_length = this->data->children[target_child_index]->length;
        if (updated_child_length > this->data->length)
        {
            this->data->length = updated_child_length;
        }
    }
    template <typename T>
    void insert_at(std::vector<int64_t> &indices, std::vector<std::list<T>> &vec)
    {
        
        if (expected_type != std::type_index(typeid(T)))
        {
            if (expected_type != typeid(T) && expected_type != typeid(std::list<T>))
            {
                throw std::runtime_error(
                    "Type mismatch: Cannot insert '" + std::string(typeid(T).name()) +
                    "' into a column of expected type '" + std::string(expected_type.name()) + "'!");
            }
        }
        std::vector<T> flat_elements;
        std::vector<int64_t> offsets;
        offsets.reserve(vec.size());
        int64_t current_offset = 0;

        if constexpr (std::is_same_v<T, bool>)
        {
            size_t total_bools = 0;
            for (const auto &inner_list : vec)
            {
                total_bools += inner_list.size();
            }
            bool *ptr = reinterpret_cast<bool *>(std::malloc(total_bools));
            size_t i = 0;
            for (const auto &inner_list : vec)
            {
                for (bool val : inner_list)
                {
                    ptr[i++] = (val ? 1 : 0);
                }
                current_offset += inner_list.size();
                offsets.push_back(current_offset);
            }

            this->data->insert_at(
                indices, ptr,
                total_bools,
                offsets.data(), offsets.size());
            std::free(ptr);
        }
        else
        {
            for (const auto &inner_list : vec)
            {
                flat_elements.insert(flat_elements.end(), inner_list.begin(), inner_list.end());
                current_offset += inner_list.size();
                offsets.push_back(current_offset);
            }

            size_t byte_size = flat_elements.size() * sizeof(T);
            this->data->insert_at(
                indices, flat_elements.data(), byte_size,
                offsets.data(), offsets.size());
        }
    }
    template <typename T>
    static format getFormat();

    template <typename T>
    static ColTable FromVector(const std::vector<T> &vec);

    static ColTable FromStruct(const std::vector<ColTable> &members, std::vector<std::string> keys);

    template <typename T>
    static ColTable FromVector(const std::vector<std::vector<T>> &nestedVec);

    template <typename T>
    static ColTable FromVector(const std::list<T> &vec);

    template <typename T>
    static ColTable FromVector(const std::vector<std::list<T>> &vec);

    void addChild(const ColTable &other)
    {
        if (other.data && other.field)
        {
            this->data->addChild(other.data);
            this->field->addChild(other.field);
        }
    }
    std::shared_ptr<ColTable> getChild(const std::string &key)
    {
        for (size_t i = 0; i < field->children.size(); ++i)
        {
            if (field->children[i] && field->children[i]->key == key)
            {
                // Return a new ColTable wrapper pointing to the child buffers
                return std::make_shared<ColTable>(field->children[i], data->children[i], field->children[i]->expected_type);
            }
        }
        throw std::runtime_error("Column key not found in schema: " + key);
    }
    void removeChild(int64_t index)
    {
        if (index >= 0 && index < (int64_t)this->data->children.size())
        {
            this->data->removeChild(index);
        }
        if (index >= 0 && index < (int64_t)this->field->children.size())
        {
            this->field->removeChild(index);
        }
    }
    // General template declaration for primitive types (1D)
    template <typename T>
    static void convertToArray(std::shared_ptr<ColTable> colTable, std::vector<T> &outResult);

    // General template declaration for struct
    static void convertToStruct(std::shared_ptr<ColTable> colTable, std::vector<ColTable> &outResult);

    // Specialized declaration for 2D Array of Arrays
    template <typename T>
    static void convertToArray(std::shared_ptr<ColTable> table, std::vector<std::vector<T>> &outResult);

    template <typename T>
    static void convertToArray(std::shared_ptr<ColTable> table, std::list<T> &outResult);

    // General template declaration for vector of list
    template <typename T>
    static void convertToArray(std::shared_ptr<ColTable> colTable, std::vector<std::list<T>> &outResult);

    // Getter for the Data
    std::shared_ptr<ColTableData> getData() const { return data; }

    // Getter for the Field (Schema)
    std::shared_ptr<ColTableField> getField() const { return field; }
    virtual ~ColTable() = default;

    int64_t get_index(const std::shared_ptr<ColTableData>& node,std::vector<int64_t>& index, size_t layer=0) const{
        if (!node) {
            throw std::runtime_error("Null data node");
        }
        if (layer >= index.size()) {
            throw std::runtime_error("Index layer out of range");
        }
        if(node->nChildren<=0){
            if(node->offsetBuffer&&node->valueBuffer){
                //String
                index[layer]=static_cast<int64_t*>(node->offsetBuffer.get()->buffer)[index[layer]];
                if(index.size()>1){
                    return index[layer-1]+index[layer];
                }else{
                    return index[layer];
                }
            }
            else if(node->valueBuffer){
                //Primitive
                if(index.size()>1){
                    return index[layer-1]+index[layer];
                }
                else{
                    return std::accumulate(index.begin(),index.end(),0);   
                }

            }else{
                throw std::runtime_error("No value");
            }
        }else{
            if(node->offsetBuffer){
                //AoA
                if (layer + 1 >= index.size()) {
                    throw std::runtime_error("Missing inner index for AoA");
                }

                if(index[layer]>=node->length){
                    throw std::runtime_error("out of bound AoA");
                }
                index[layer]=static_cast<int64_t*>(node->offsetBuffer.get()->buffer)[index[layer]];
                if(layer==0){
                    return get_index(node->children[0],index,layer+1);
                }else{
                    index[layer]=index[layer-1]+index[layer];
                    return get_index(node->children[0],index,layer+1);
                }
                
            }
            else if(!node->children.empty()){
                //Struct
                //Struct has children, and needs to have key, assume the top layer will handle the key and pass the children here no value should be here
                throw std::runtime_error("Struct should decompose into children before pass to get_index");

            }else{
                throw std::runtime_error("No value");
            }
        }
        
    }
};

// wrapper to map string key for struct variables
template <typename T, typename FieldType>
struct ColTableStructField
{
    std::string name;
    FieldType T::*ptr;
};
class ColTableStructMapper
{
private:
    // std::vector<T>& vec is the vector of struct
    template <typename T, typename FieldType>
    static ColTable extract_column(const std::vector<T> &vec, const std::string &name, FieldType T::*ptr)
    {
        std::vector<FieldType> column;
        column.reserve(vec.size());
        for (const auto &v : vec)
        {
            column.push_back(v.*(ptr));
        }
        // Construct the coltable from column
        ColTable col = ColTable::FromVector(column);
        col.field->key = name;
        return col;
    }

    template <typename T, typename FieldType>
    static void assign_column(std::vector<T> &vec, const std::vector<ColTable> &coltable, const std::string &name, FieldType T::*ptr)
    {
        for (const auto &col : coltable)
        {
            std::vector<FieldType> extracted;
            if (col.field->key == name)
            {
                ColTable::convertToArray(std::make_shared<ColTable>(col), extracted);
                for (int i = 0; i < extracted.size(); i++)
                {
                    vec[i].*(ptr) = std::move(extracted[i]);
                }
                break;
            }
        }
    }
    template <typename T>
    static void assign_all(std::vector<T> & /*vec*/, const std::vector<ColTable> & /*structCols*/)
    {
        // End of recursion
    }
    template <typename T, typename StringType, typename FieldType, typename... Rest>
    static void assign_all(std::vector<T> &vec, const std::vector<ColTable> &cols, const StringType &name, FieldType T::*ptr, Rest... rest)
    {
        assign_column(vec, cols, name, ptr); // Do the first one
        assign_all(vec, cols, rest...);      // Recursively call for the rest
    }
    // Base Case
    template <typename T>
    static void extract_all(const std::vector<T> & /*vec*/, std::vector<ColTable> & /*members*/, std::vector<std::string> & /*keys*/) {}

    // Recursive Case (Pulls TWO arguments off the list: 'name' and 'ptr')
    template <typename T, typename StringType, typename FieldType, typename... Rest>
    static void extract_all(const std::vector<T> &vec, std::vector<ColTable> &members, std::vector<std::string> &keys, const StringType &name, FieldType T::*ptr, Rest... rest)
    {
        members.push_back(extract_column(vec, name, ptr));
        keys.push_back(name);
        extract_all(vec, members, keys, rest...); // Recurse with whatever is left
    }

public:
    template <typename T, typename... Args>
    static ColTable Serialize(const std::vector<T> &vec, Args... args)
    {
        std::vector<ColTable> fieldsTable;
        std::vector<std::string> keys;
        // Construct column from struct
        extract_all(vec, fieldsTable, keys, args...);
        // Pass keys map to variable to ColTable
        return ColTable::FromStruct(fieldsTable, keys);
    }

    template <typename T, typename... Args>
    static void Deserialize(std::shared_ptr<ColTable> coltable, std::vector<T> &outVec, Args... args)
    {
        std::vector<ColTable> coltableStruct;
        ColTable::convertToStruct(coltable, coltableStruct);
        size_t len = coltableStruct.size();
        outVec.resize(len);
        assign_all(outVec, coltableStruct, args...);
    }
};

// --- 2D Vectors (Array of Arrays) ---

template <typename T>
inline void ColTable::convertToArray(std::shared_ptr<ColTable> table, std::vector<std::vector<T>> &outResult)
{
    outResult.clear();
    auto data = table->getData();
    auto offsetPtr = data->offsetBuffer;

    // 1. Safety check
    if (table->getField()->Format != format::AoA)
    {
        throw std::runtime_error("Table is not an Array of Arrays!");
    }
    if (!offsetPtr)
        return;

    const int64_t *offsetBuffer = static_cast<const int64_t *>(offsetPtr->get());
    size_t count = data->length;

    auto childTable = std::make_shared<ColTable>(table->getField()->children[0], data->children[0], table->getField()->children[0]->expected_type);

    // Recursively call convertToArray using the out-parameter for the 1D flat vector
    std::vector<T> flatVec;
    ColTable::convertToArray(childTable, flatVec);

    outResult.reserve(count);
    for (size_t i = 0; i < count; i++)
    {
        size_t start = offsetBuffer[i];
        size_t end = offsetBuffer[i + 1];

        // Emplace back directly constructs the sublist avoiding unnecessary copies
        outResult.emplace_back(flatVec.begin() + start, flatVec.begin() + end);
    }
}

template <typename T>
inline ColTable ColTable::FromVector(const std::vector<std::vector<T>> &nestedVec)
{
    size_t length = nestedVec.size();

    size_t totalInnerElements = 0;
    for (const auto &vec : nestedVec)
    {
        totalInnerElements += vec.size();
    }

    auto offsetBufferSize = (length + 1) * sizeof(int64_t);
    auto offsetBuffer = std::make_shared<Buffer>(offsetBufferSize);
    int64_t *offsets = static_cast<int64_t *>(offsetBuffer->get());

    std::vector<T> flattened;
    flattened.reserve(totalInnerElements);

    int64_t currentOffset = 0;
    for (size_t i = 0; i < length; ++i)
    {
        offsets[i] = currentOffset;
        currentOffset += nestedVec[i].size();
        flattened.insert(flattened.end(), nestedVec[i].begin(), nestedVec[i].end());
    }
    offsets[length] = currentOffset;

    ColTable childTable = ColTable::FromVector<T>(flattened);

    auto parentField = std::make_shared<ColTableField>("AoA", format::AoA, typeid(T));
    parentField->addChild(childTable.getField());

    auto parentData = std::make_shared<ColTableData>(length);
    parentData->addOffsetBuffer(offsetBuffer);
    parentData->addChild(childTable.getData());
    parentData->nullCount = -1;
    parentData->length = length; // Fixed minor syntax error

    return ColTable(parentField, parentData, typeid(T));
}

inline ColTable ColTable::FromStruct(const std::vector<ColTable> &members, const std::vector<std::string> keys)
{
    if (members.size() != keys.size())
    {
        throw std::invalid_argument("Members and keys must have the same size");
    }
    size_t num_columns = members.size();

    size_t num_rows = (num_columns > 0) ? members[0].getData()->length : 0;

    auto parentData = std::make_shared<ColTableData>(num_rows);
    auto parentField = std::make_shared<ColTableField>("STRUCT", format::STRUCT, typeid(void));
    for (int i = 0; i < num_columns; i++)
    {
        members[i].field->key = keys[i];
        parentField->addChild(members[i].getField());
        parentData->addChild(members[i].getData());
    }

    parentData->length = num_rows; // Fixed minor syntax error

    return ColTable(parentField, parentData, typeid(void));
}

inline void ColTable::convertToStruct(std::shared_ptr<ColTable> table, std::vector<ColTable> &outResult)
{
    outResult.clear();

    // 1. Safety check
    if (table->getField()->Format != format::STRUCT)
    {
        throw std::runtime_error("Table format is not a STRUCT!");
    }

    auto data = table->getData();
    if (data->children.empty())
        return;
    auto field = table->getField();

    for (int i = 0; i < data->nChildren; i++)
    {
        outResult.push_back(ColTable(field->children[i], data->children[i], field->children[i]->expected_type));
    }
}

template <> inline format ColTable::getFormat<int8_t>()   { return format::INT8; }
template <> inline format ColTable::getFormat<int16_t>()  { return format::INT16; }
template <> inline format ColTable::getFormat<int32_t>()  { return format::INT32; }
template <> inline format ColTable::getFormat<int64_t>()  { return format::INT64; }

template <> inline format ColTable::getFormat<long long>() { return format::LONG_LONG;}
template <> inline format ColTable::getFormat<unsigned long long>() { return format::UNSIGNED_LONG_LONG;}

template <> inline format ColTable::getFormat<uint8_t>()  { return format::UINT8; }
template <> inline format ColTable::getFormat<uint16_t>() { return format::UINT16; }
template <> inline format ColTable::getFormat<uint32_t>() { return format::UINT32; }
template <> inline format ColTable::getFormat<uint64_t>() { return format::UINT64; }

template <> inline format ColTable::getFormat<float>()    { return format::FLOAT; }
template <> inline format ColTable::getFormat<double>()   { return format::DOUBLE; }
template <> inline format ColTable::getFormat<bool>()     { return format::BOOL; }

template <typename T>
ColTable ColTable::FromVector(const std::vector<T>& vec) {
    auto field = std::make_shared<ColTableField>("", getFormat<T>(),
        typeid(T));
    auto data = std::make_shared<ColTableData>(vec.size());

    auto buffer = std::make_shared<Buffer>(vec.size() * sizeof(T));
    if constexpr (std::is_same_v<T,bool>){
        std::vector<uint8_t> booldata;
        for (int i=0;i<vec.size();i++){
            booldata.push_back(vec[i]);
        }
        std::memcpy(buffer->get(), booldata.data(), vec.size() * sizeof(T));
    }else{
        std::memcpy(buffer->get(), vec.data(), vec.size() * sizeof(T));
    }

    data->addBuffer(buffer);

    return ColTable(field, data,
        typeid(T));
}

template <typename T>
void ColTable::convertToArray(
    std::shared_ptr<ColTable> colTable,
    std::vector<T>& outResult
) {
    auto data = colTable->getData();

    outResult.resize(data->length);

    auto buffer = static_cast<T*>(data->valueBuffer->get());

    if constexpr (std::is_same_v<T, bool>) {
        for (size_t i = 0; i < data->length; ++i) {
            outResult[i] = buffer[i];
        }
    } else {
        std::memcpy(
            outResult.data(),
            buffer,
            data->length * sizeof(T)
        );
    }
}
// --- Strings ---

static size_t getLength(const std::vector<std::optional<std::string>> &vec)
{
    size_t totalStringLength = 0;
    for (const auto &v : vec)
    {
        if (v.has_value())
        {
            totalStringLength += v->length();
        }
    }
    return totalStringLength;
}

template <>
inline ColTable ColTable::FromVector<std::optional<std::string>>(const std::vector<std::optional<std::string>> &vec)
{
    size_t totalStringLength = getLength(vec);
    size_t offset_bytes = totalStringLength + sizeof(int64_t);
    std::vector<uint8_t> validity_bits;
    validity_bits.reserve(vec.size());

    auto offsetBuffersize = (vec.size() + 1) * sizeof(int64_t);
    auto offsetBuffer = std::make_shared<Buffer>(offsetBuffersize);
    int64_t *offsetBuffer_ptr = static_cast<int64_t *>(offsetBuffer->get());

    auto buffer = std::make_shared<Buffer>(offset_bytes);
    char *buffer_ptr = static_cast<char *>(buffer->get());
    int64_t offset = 0;

    for (size_t i = 0; i < vec.size(); ++i)
    {
        offsetBuffer_ptr[i] = offset;
        if (vec[i].has_value())
        {
            std::memcpy((buffer_ptr + offset), vec[i]->data(), vec[i]->size());
            offset += vec[i]->size();
            validity_bits.push_back(1);
        }
        else
        {
            validity_bits.push_back(0);
        }
    }
    offsetBuffer_ptr[vec.size()] = offset;

    auto field = std::make_shared<ColTableField>("STRING", format::STRING, typeid(std::optional<std::string>));
    auto data = std::make_shared<ColTableData>(vec.size());

    auto validityBitmapsBuffer = std::make_shared<Buffer>(vec.size());
    validityBitmapsBuffer->buffer = reinterpret_cast<uint8_t *>(validityBitmapsBuffer->get());
    validityBitmapsBuffer->insert_at(
        {0},
        validity_bits.data(),
        validity_bits.size());
    data->addBuffer(buffer);
    data->addOffsetBuffer(offsetBuffer);
    data->addValidityBuffer(validityBitmapsBuffer);
    data->nullCount = -1;
    return ColTable(field, data, typeid(std::optional<std::string>));
}

template <>
inline void ColTable::convertToArray(std::shared_ptr<ColTable> colTable, std::vector<std::optional<std::string>> &outResult)
{
    outResult.clear();
    auto dataPtr = colTable->getData()->valueBuffer;
    auto offsetPtr = colTable->getData()->offsetBuffer;
    uint8_t *validitybitmap = static_cast<uint8_t *>(colTable->getData()->validityBitmapsBuffer->get());

    if (!dataPtr || !offsetPtr)
        return;

    char *buffer = static_cast<char *>(dataPtr->get());
    int64_t *offsetBuffer = static_cast<int64_t *>(offsetPtr->get());
    size_t count = colTable->getData()->length;

    outResult.resize(count);
    for (size_t i = 0; i < count; i++)
    {
        // Optimization: C++ string constructor instantly builds from a start pointer and length
        size_t start = offsetBuffer[i];
        size_t end = offsetBuffer[i + 1];
        bool is_valid = validitybitmap[i];
        if (!is_valid)
        {
            outResult[i] = std::nullopt;
        }
        else
        {
            outResult[i] = std::string(buffer + start, end - start);
        }
    }
}

template <typename T>
inline void ColTable::convertToArray(std::shared_ptr<ColTable> table, std::list<T> &outResult)
{
    outResult.clear();

    // 1. Safety check
    if (table->getField()->Format != format::LIST)
    {
        throw std::runtime_error("Table format is not a LIST!");
    }

    auto data = table->getData();
    if (data->children.empty())
        return;

    // 2. Reconstruct the 1D child table that actually holds the flat data
    auto childTable = std::make_shared<ColTable>(table->getField()->children[0], data->children[0], table->getField()->children[0]->expected_type);

    // 3. Extract the flat data into a temporary vector using your existing 1D function
    std::vector<T> tempVec;
    ColTable::convertToArray(childTable, tempVec);

    // 4. Assign the extracted data directly into the output list
    outResult.assign(tempVec.begin(), tempVec.end());
}

// --- 1D Lists ---

template <typename T>
inline ColTable ColTable::FromVector(const std::list<T> &lst)
{
    size_t length = lst.size();

    std::vector<T> flattened;
    flattened.reserve(length);

    for (const auto &item : lst)
    {
        flattened.push_back(item);
    }

    ColTable childTable = ColTable::FromVector<T>(flattened);

    auto parentField = std::make_shared<ColTableField>("LIST", format::LIST, typeid(T));
    parentField->addChild(childTable.getField());

    auto parentData = std::make_shared<ColTableData>(length);
    parentData->addChild(childTable.getData());
    parentData->nullCount = -1;
    parentData->length = length; // Fixed minor syntax error

    return ColTable(parentField, parentData, typeid(T));
}
template <typename T>
inline ColTable ColTable::FromVector(const std::vector<std::list<T>> &nestedList)
{
    size_t length = nestedList.size();

    // 1. Calculate total elements across all lists
    size_t totalInnerElements = 0;
    for (const auto &lst : nestedList)
    {
        totalInnerElements += lst.size();
    }

    // 2. Set up the offset buffer
    auto offsetBufferSize = (length + 1) * sizeof(int64_t);
    auto offsetBuffer = std::make_shared<Buffer>(offsetBufferSize);
    int64_t *offsets = static_cast<int64_t *>(offsetBuffer->get());

    // 3. Flatten the data into a 1D vector
    std::vector<T> flattened;
    flattened.reserve(totalInnerElements);

    int64_t currentOffset = 0;
    for (size_t i = 0; i < length; ++i)
    {
        offsets[i] = currentOffset;
        currentOffset += nestedList[i].size();
        // Extract items from the list into the flat vector
        for (const auto &item : nestedList[i])
        {
            flattened.push_back(item);
        }
    }
    offsets[length] = currentOffset;

    // 4. Pass the flattened 1D data to the standard FromVector
    ColTable childTable = ColTable::FromVector<T>(flattened);

    // 5. Build the parent Table (treated as an Array of Arrays)
    auto parentField = std::make_shared<ColTableField>("AoA", format::AoA, typeid(T));
    parentField->addChild(childTable.getField());

    auto parentData = std::make_shared<ColTableData>(length);
    parentData->addOffsetBuffer(offsetBuffer);
    parentData->addChild(childTable.getData());
    parentData->nullCount = -1;
    parentData->length = length;

    return ColTable(parentField, parentData, typeid(T));
}
template <typename T>
inline void ColTable::convertToArray(std::shared_ptr<ColTable> table, std::vector<std::list<T>> &outResult)
{
    outResult.clear();
    auto data = table->getData();
    auto offsetPtr = data->offsetBuffer;

    // 1. Safety checks
    if (table->getField()->Format != format::AoA)
    {
        throw std::runtime_error("Table is not an Array of Arrays!");
    }
    if (!offsetPtr)
        return;

    const int64_t *offsetBuffer = static_cast<const int64_t *>(offsetPtr->get());
    size_t count = data->length;

    auto childTable = std::make_shared<ColTable>(table->getField()->children[0], data->children[0], table->getField()->children[0]->expected_type);

    // 2. Recursively extract the 1D flat vector
    std::vector<T> flatVec;
    ColTable::convertToArray(childTable, flatVec);

    // 3. Reconstruct the lists using the offsets
    outResult.reserve(count);
    for (size_t i = 0; i < count; i++)
    {
        size_t start = offsetBuffer[i];
        size_t end = offsetBuffer[i + 1];

        // Construct the list from the flat vector iterators
        outResult.emplace_back(flatVec.begin() + start, flatVec.begin() + end);
    }
}
#endif // COLTABLE_INTERFACE