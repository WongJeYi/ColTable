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
                std::cout << offsetBuffer_ptr[count] << (i == offsetBuffer_ptr[count]-1 ? ", " : "");
                if (offsetBuffer_ptr[count] < i) {
                    count += 1;
                }
            }
            length = data->length+1;
            count = 0;
            std::cout <<std::endl;
            std::cout <<"OffsetBuffer"<<length<<std::endl;
            for (size_t i = 0; i < length; ++i) {
                std::cout << offsetBuffer_ptr[i] << ", ";
                if (offsetBuffer_ptr[count] < i) {
                    count += 1;
                }
            }
            break;
        }
        case format::AoA: { 
            // NOTE: C++ needs a concrete type to read the raw memory. 
            // I am using double* here as a placeholder. If your AoA supports multiple 
            // inner types, you will need to switch on the expected inner type here.
            double* ptr = static_cast<double*>(raw);
            int64_t* offsetBuffer_ptr = static_cast<int64_t*>(data->offsetBuffer->get());
            
            int64_t num_rows = data->length;
            
            // Since we removed the trailing offset in the insert_at function, 
            // the total number of flattened elements is needed to know where the very last row ends.
            // Adjust "data->valueBuffer->size()" to whatever method gets your buffer's byte capacity.
            int64_t total_elements = data->valueBuffer->bufferSize / sizeof(double); 

            for (size_t i = 0; i < num_rows; ++i) {
                int64_t start_idx = offsetBuffer_ptr[i];
                
                // The end index is the start of the next row, OR the end of the flat array for the final row
                int64_t end_idx = (i + 1 < num_rows) ? offsetBuffer_ptr[i + 1] : total_elements;

                std::cout << "[";
                for (int64_t j = start_idx; j < end_idx; ++j) {
                    std::cout << ptr[j] << (j == end_idx - 1 ? "" : ", ");
                }
                std::cout << "]" << (i == num_rows - 1 ? "" : ", ");
            }
            
            // Print the offset buffer for debugging purposes, just like your String implementation
            std::cout << std::endl;
            std::cout << "OffsetBuffer (Length " << num_rows << "): [ ";
            for (size_t i = 0; i < num_rows; ++i) {
                 std::cout << offsetBuffer_ptr[i] << (i == num_rows - 1 ? "" : ", ");
            }
            break;
        }
        default:
            std::cout << "Cannot print: Unknown or Nested Type";
            break;
    }

    std::cout << " ]" << std::endl;
}

