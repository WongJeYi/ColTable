
#include "ColTable.hpp"


/**
 * enum class format {

INT32, INT64, DOUBLE, FLOAT, STRING, LIST, STRUCT, DICT, UNKNOWN
};
 */


void printValueBuffer(const std::shared_ptr<ColTableData>& data, format type) {
            if (!data || !data->valueBuffer) {
                std::cout << "[Empty or Null Buffer]" << std::endl;
                return;
            }

            // Get the 64-byte aligned raw pointer
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
                    // Note: If STRING, the valueBuffer usually contains raw char bytes
                    char* ptr = static_cast<char*>(raw);
                    int64_t* offsetBuffer_ptr = static_cast<int64_t*>(data->offsetBuffer->get());
                    int64_t length = offsetBuffer_ptr[len];
                    int64_t count=0;
                    for (size_t i = 0; i < length; ++i) {
                        std::cout << ptr[i] <<(i==offsetBuffer_ptr[count]-1?", " : "");
                        if(offsetBuffer_ptr[count]<i){
                            count+=1;
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
template <>
std::vector<int32_t> ColTable::convertToArray(std::shared_ptr<ColTable> colTable) {
    auto dataPtr=colTable->getData()->valueBuffer;
    int32_t* buffer = static_cast<int32_t*>(dataPtr->get());
    if (!dataPtr) {
        return {}; // Return empty if no data
    }else{
        size_t count = colTable->getData()->length;
        std::vector<int32_t> vector(count);
        for (size_t i=0; i<count; i++){
            vector[i]=buffer[i];
        }
        return vector;
    }
}

template <>
std::vector<int64_t> ColTable::convertToArray(std::shared_ptr<ColTable> colTable) {
    auto dataPtr=colTable->getData()->valueBuffer;
    int64_t* buffer = static_cast<int64_t*>(dataPtr->get());
    if (!dataPtr) {
        return {}; // Return empty if no data
    }else{
        size_t count = colTable->getData()->length;
        std::vector<int64_t> vector(count);
        for (size_t i=0; i<count; i++){
            vector[i]=buffer[i];
        }
        return vector;
    }
}

template <>
std::vector<double> ColTable::convertToArray(std::shared_ptr<ColTable> colTable) {
    auto dataPtr=colTable->getData()->valueBuffer;
    double* buffer = static_cast<double*>(dataPtr->get());
    if (!dataPtr) {
        return {}; // Return empty if no data
    }else{
        size_t count = colTable->getData()->length;
        std::vector<double> vector(count);
        for (size_t i=0; i<count; i++){
            vector[i]=buffer[i];
        }
        return vector;
    }
}

template <>
std::vector<float> ColTable::convertToArray(std::shared_ptr<ColTable> colTable) {
    auto dataPtr=colTable->getData()->valueBuffer;
    float* buffer = static_cast<float*>(dataPtr->get());
    if (!dataPtr) {
        return {}; // Return empty if no data
    }else{
        size_t count = colTable->getData()->length;
        std::vector<float> vector(count);
        for (size_t i=0; i<count; i++){
            vector[i]=buffer[i];
        }
        return vector;
    }
}


template <>
std::vector<std::string> ColTable::convertToArray(std::shared_ptr<ColTable> colTable) {
    auto dataPtr=colTable->getData()->valueBuffer;
    char* buffer = static_cast<char*>(dataPtr->get());
    auto offsetPtr = colTable->getData()->offsetBuffer;
    int64_t* offsetBuffer = static_cast<int64_t*>(offsetPtr->get());
    if (!dataPtr) {
        return {}; // Return empty if no data
    }else{
        size_t count = colTable->getData()->length;
        std::vector<std::string> vector(count);
        int64_t offset=0;
        for (size_t i=0; i<count; i++){
            offset=offsetBuffer[i];
            std::string cat = "";
            for (size_t j=offset;j<offsetBuffer[i+1];j++){
                cat=cat+buffer[j];
            }
            vector[i]=cat;
        }
        return vector;
    }
}

template<>
ColTable ColTable::FromVector<int32_t>(const std::vector<int32_t>& vec) {

    format format= format::INT32;
    
    auto field = std::make_shared<ColTableField>("INT32",format::INT32);
    
    size_t byteSize = vec.size() * sizeof(int32_t);;

    int64_t nullCount;

    int64_t validityBitmapsBuffer;
    //make shared buffer for initiating buffer 
    auto buffer = std::make_shared<Buffer>(byteSize);

    std::memcpy(buffer->get(), vec.data(), byteSize);

    //creating ColTableData object
    auto data  = std::make_shared<ColTableData>(vec.size());
    //link buffer to data
    data->addBuffer(buffer);
    
    // 5. Handle Nulls
    data->nullCount = -1; 
    return ColTable(field, data);
};

template<>
ColTable ColTable::FromVector<int64_t>(const std::vector<int64_t>& vec) {

    format format= format::INT64;
    
    auto field = std::make_shared<ColTableField>("INT64",format::INT64);
    
    size_t byteSize = vec.size() * sizeof(int64_t);;

    int64_t nullCount;

    int64_t validityBitmapsBuffer;
    //make shared buffer for initiating buffer 
    auto buffer = std::make_shared<Buffer>(byteSize);

    std::memcpy(buffer->get(), vec.data(), byteSize);

    //creating ColTableData object
    auto data  = std::make_shared<ColTableData>(vec.size());
    //link buffer to data
    data->addBuffer(buffer);
    
    //no null count
    data->nullCount = -1; 
    return ColTable(field, data);
};

template<>
ColTable ColTable::FromVector<double>(const std::vector<double>& vec) {

    format format= format::DOUBLE;
    
    auto field = std::make_shared<ColTableField>("DOUBLE",format::DOUBLE);
    
    size_t byteSize = vec.size() * sizeof(double);;

    int64_t nullCount;

    int64_t validityBitmapsBuffer;
    //make shared buffer for initiating buffer 
    auto buffer = std::make_shared<Buffer>(byteSize);

    std::memcpy(buffer->get(), vec.data(), byteSize);

    //creating ColTableData object
    auto data  = std::make_shared<ColTableData>(vec.size());
    //link buffer to data
    data->addBuffer(buffer);
    
    // no null count
    data->nullCount = -1; 
    return ColTable(field, data);
};

template<>
ColTable ColTable::FromVector<float>(const std::vector<float>& vec) {

    format format= format::FLOAT;
    
    auto field = std::make_shared<ColTableField>("FLOAT",format::FLOAT);
    
    size_t byteSize = vec.size() * sizeof(float);;

    int64_t nullCount;

    int64_t validityBitmapsBuffer;
    //make shared buffer for initiating buffer 
    auto buffer = std::make_shared<Buffer>(byteSize);

    std::memcpy(buffer->get(), vec.data(), byteSize);

    //creating ColTableData object
    auto data  = std::make_shared<ColTableData>(vec.size());
    //link buffer to data
    data->addBuffer(buffer);
    
    //no null count
    data->nullCount = -1; 
    return ColTable(field, data);
};
template <typename T>
static size_t getLength(const std::vector<T>& vec){
    size_t totalStringLength = 0;
    for (const std::string& v: vec){
        totalStringLength+=v.length();
    }
    return totalStringLength;
}

template<>
ColTable ColTable::FromVector<std::string>(const std::vector<std::string>& vec) {
    //calculate the total lenght for string
    size_t totalStringLength = getLength(vec);
    // Offset byte for buffer
    size_t offset_bytes = totalStringLength + sizeof(int64_t);
    

    //creating offset buffer
    auto offsetBuffersize = (vec.size() + 1) * sizeof(int64_t);
    auto offsetBuffer = std::make_shared<Buffer>(offsetBuffersize);
    int64_t* offsetBuffer_ptr = static_cast<int64_t*>(offsetBuffer->get());
    
    //make shared buffer for initiating buffer 
    auto buffer = std::make_shared<Buffer>(offset_bytes);
    //get pointer to buffer for pointer arithmatic
    char* buffer_ptr = static_cast<char*>(buffer->get());
    int64_t offset=0;

    for (size_t i = 0; i < vec.size(); ++i) {
        offsetBuffer_ptr[i]=offset;
        std::memcpy((buffer_ptr+offset), vec[i].data(), vec[i].size());
        offset+=vec[i].size();
    } 
    offsetBuffer_ptr[vec.size()] = offset;

    format format= format::STRING;
    
    auto field = std::make_shared<ColTableField>("STRING",format::STRING);

    int64_t nullCount;

    int64_t validityBitmapsBuffer;

    //creating ColTableData object
    auto data  = std::make_shared<ColTableData>(vec.size());
    
    //link buffer to data
    data->addBuffer(buffer);
    data->addOffsetBuffer(offsetBuffer);
    
    //No Null count
    data->nullCount = -1; 
    return ColTable(field, data);
};


