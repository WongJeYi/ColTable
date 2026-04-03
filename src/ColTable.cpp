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

