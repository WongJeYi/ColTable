#include "ColTable.hpp"
#include <iostream>
#include <cstring>
#include <stdexcept>

void printValueBuffer(const std::shared_ptr<ColTableData>& data, format type) {
    if (!data || !data->valueBuffer) {
        std::cout << "[Empty or Null Buffer]" << std::endl;
        return;
    }

    void* raw = data->valueBuffer->get();
    size_t len = data->length;

    std::cout << "ValueBuffer (Length " << len << "): [ ";

    switch (type) {
        case format::INT32: {
            int32_t* ptr = static_cast<int32_t*>(raw);
            for (size_t i = 0; i < len; ++i) {
                std::cout << ptr[i] << (i == len - 1 ? "" : ", ");
            }
            break;
        }
        case format::INT64: {
            int64_t* ptr = static_cast<int64_t*>(raw);
            for (size_t i = 0; i < len; ++i) {
                std::cout << ptr[i] << (i == len - 1 ? "" : ", ");
            }
            break;
        }
        case format::FLOAT: {
            float* ptr = static_cast<float*>(raw);
            for (size_t i = 0; i < len; ++i) {
                std::cout << ptr[i] << (i == len - 1 ? "" : ", ");
            }
            break;
        }
        case format::DOUBLE: {
            double* ptr = static_cast<double*>(raw);
            for (size_t i = 0; i < len; ++i) {
                std::cout << ptr[i] << (i == len - 1 ? "" : ", ");
            }
            break;
        }
        case format::STRING: {
            char* ptr = static_cast<char*>(raw);
            int64_t* offsetBuffer_ptr = static_cast<int64_t*>(data->offsetBuffer->get());
            int64_t length = offsetBuffer_ptr[len];
            int64_t count = 0;
            for (size_t i = 0; i < length; ++i) {
                std::cout << ptr[i] << (i == offsetBuffer_ptr[count]-1 ? ", " : "");
                if (offsetBuffer_ptr[count] < i) {
                    count += 1;
                }
            }
            break;
        }
        default:
            std::cout << "Cannot print: Unknown or Nested Type";
            break;
    }

    std::cout << " ]" << std::endl;
}

// --- Explicitly Sized Signed Integers ---

template<>
ColTable ColTable::FromVector<int8_t>(const std::vector<int8_t>& vec) {
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
void ColTable::convertToArray(std::shared_ptr<ColTable> colTable, std::vector<int8_t>& outResult) {
    outResult.clear();
    auto dataPtr = colTable->getData()->valueBuffer;
    if (!dataPtr) return;

    int8_t* buffer = static_cast<int8_t*>(dataPtr->get());
    size_t count = colTable->getData()->length;
    outResult.resize(count);
    for (size_t i = 0; i < count; i++) outResult[i] = buffer[i];
}

template<>
ColTable ColTable::FromVector<int16_t>(const std::vector<int16_t>& vec) {
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
void ColTable::convertToArray(std::shared_ptr<ColTable> colTable, std::vector<int16_t>& outResult) {
    outResult.clear();
    auto dataPtr = colTable->getData()->valueBuffer;
    if (!dataPtr) return;

    int16_t* buffer = static_cast<int16_t*>(dataPtr->get());
    size_t count = colTable->getData()->length;
    outResult.resize(count);
    for (size_t i = 0; i < count; i++) outResult[i] = buffer[i];
}

template<>
ColTable ColTable::FromVector<int32_t>(const std::vector<int32_t>& vec) {
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
void ColTable::convertToArray(std::shared_ptr<ColTable> colTable, std::vector<int32_t>& outResult) {
    outResult.clear();
    auto dataPtr = colTable->getData()->valueBuffer;
    if (!dataPtr) return;

    int32_t* buffer = static_cast<int32_t*>(dataPtr->get());
    size_t count = colTable->getData()->length;
    outResult.resize(count);
    for (size_t i = 0; i < count; i++) outResult[i] = buffer[i];
}

template<>
ColTable ColTable::FromVector<int64_t>(const std::vector<int64_t>& vec) {
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
void ColTable::convertToArray(std::shared_ptr<ColTable> colTable, std::vector<int64_t>& outResult) {
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
ColTable ColTable::FromVector<long long>(const std::vector<long long>& vec) {
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
void ColTable::convertToArray(std::shared_ptr<ColTable> colTable, std::vector<long long>& outResult) {
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
ColTable ColTable::FromVector<uint8_t>(const std::vector<uint8_t>& vec) {
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
void ColTable::convertToArray(std::shared_ptr<ColTable> colTable, std::vector<uint8_t>& outResult) {
    outResult.clear();
    auto dataPtr = colTable->getData()->valueBuffer;
    if (!dataPtr) return;

    uint8_t* buffer = static_cast<uint8_t*>(dataPtr->get());
    size_t count = colTable->getData()->length;
    outResult.resize(count);
    for (size_t i = 0; i < count; i++) outResult[i] = buffer[i];
}

template<>
ColTable ColTable::FromVector<uint16_t>(const std::vector<uint16_t>& vec) {
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
void ColTable::convertToArray(std::shared_ptr<ColTable> colTable, std::vector<uint16_t>& outResult) {
    outResult.clear();
    auto dataPtr = colTable->getData()->valueBuffer;
    if (!dataPtr) return;

    uint16_t* buffer = static_cast<uint16_t*>(dataPtr->get());
    size_t count = colTable->getData()->length;
    outResult.resize(count);
    for (size_t i = 0; i < count; i++) outResult[i] = buffer[i];
}

template<>
ColTable ColTable::FromVector<uint32_t>(const std::vector<uint32_t>& vec) {
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
void ColTable::convertToArray(std::shared_ptr<ColTable> colTable, std::vector<uint32_t>& outResult) {
    outResult.clear();
    auto dataPtr = colTable->getData()->valueBuffer;
    if (!dataPtr) return;

    uint32_t* buffer = static_cast<uint32_t*>(dataPtr->get());
    size_t count = colTable->getData()->length;
    outResult.resize(count);
    for (size_t i = 0; i < count; i++) outResult[i] = buffer[i];
}

template<>
ColTable ColTable::FromVector<uint64_t>(const std::vector<uint64_t>& vec) {
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
void ColTable::convertToArray(std::shared_ptr<ColTable> colTable, std::vector<uint64_t>& outResult) {
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
ColTable ColTable::FromVector<unsigned long long>(const std::vector<unsigned long long>& vec) {
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
void ColTable::convertToArray(std::shared_ptr<ColTable> colTable, std::vector<unsigned long long>& outResult) {
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
ColTable ColTable::FromVector<float>(const std::vector<float>& vec) {
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
void ColTable::convertToArray(std::shared_ptr<ColTable> colTable, std::vector<float>& outResult) {
    outResult.clear();
    auto dataPtr = colTable->getData()->valueBuffer;
    if (!dataPtr) return;

    float* buffer = static_cast<float*>(dataPtr->get());
    size_t count = colTable->getData()->length;
    outResult.resize(count);
    for (size_t i = 0; i < count; i++) outResult[i] = buffer[i];
}

template<>
ColTable ColTable::FromVector<double>(const std::vector<double>& vec) {
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
void ColTable::convertToArray(std::shared_ptr<ColTable> colTable, std::vector<double>& outResult) {
    outResult.clear();
    auto dataPtr = colTable->getData()->valueBuffer;
    if (!dataPtr) return;

    double* buffer = static_cast<double*>(dataPtr->get());
    size_t count = colTable->getData()->length;
    outResult.resize(count);
    for (size_t i = 0; i < count; i++) outResult[i] = buffer[i];
}

// --- Strings ---

template <typename T>
static size_t getLength(const std::vector<T>& vec) {
    size_t totalStringLength = 0;
    for (const std::string& v : vec) {
        totalStringLength += v.length();
    }
    return totalStringLength;
}

template<>
ColTable ColTable::FromVector<std::string>(const std::vector<std::string>& vec) {
    size_t totalStringLength = getLength(vec);
    size_t offset_bytes = totalStringLength + sizeof(int64_t);

    auto offsetBuffersize = (vec.size() + 1) * sizeof(int64_t);
    auto offsetBuffer = std::make_shared<Buffer>(offsetBuffersize);
    int64_t* offsetBuffer_ptr = static_cast<int64_t*>(offsetBuffer->get());
    
    auto buffer = std::make_shared<Buffer>(offset_bytes);
    char* buffer_ptr = static_cast<char*>(buffer->get());
    int64_t offset = 0;

    for (size_t i = 0; i < vec.size(); ++i) {
        offsetBuffer_ptr[i] = offset;
        std::memcpy((buffer_ptr + offset), vec[i].data(), vec[i].size());
        offset += vec[i].size();
    } 
    offsetBuffer_ptr[vec.size()] = offset;

    auto field = std::make_shared<ColTableField>("STRING", format::STRING);
    auto data  = std::make_shared<ColTableData>(vec.size());
    
    data->addBuffer(buffer);
    data->addOffsetBuffer(offsetBuffer);
    data->nullCount = -1; 
    return ColTable(field, data);
}

template <>
void ColTable::convertToArray(std::shared_ptr<ColTable> colTable, std::vector<std::string>& outResult) {
    outResult.clear();
    auto dataPtr = colTable->getData()->valueBuffer;
    auto offsetPtr = colTable->getData()->offsetBuffer;
    
    if (!dataPtr || !offsetPtr) return;
    
    char* buffer = static_cast<char*>(dataPtr->get());
    int64_t* offsetBuffer = static_cast<int64_t*>(offsetPtr->get());
    size_t count = colTable->getData()->length;
    
    outResult.resize(count);
    for (size_t i = 0; i < count; i++) {
        // Optimization: C++ string constructor instantly builds from a start pointer and length
        size_t start = offsetBuffer[i];
        size_t end = offsetBuffer[i+1];
        outResult[i] = std::string(buffer + start, end - start);
    }
}
