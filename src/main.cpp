#include "ColTable.hpp"

int main(){
    std::vector<int32_t> myVector = {1, 2, 3, 4, 5};
    std::vector<int32_t> myVector2 = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    // 2. Convert TO the ColTable format
    ColTable table = ColTable::FromVector<int32_t>(myVector);
    ColTable table2 = ColTable::FromVector<int32_t>(myVector2);

    // 3. Convert BACK to vectors
    // Note: If convertToArray is a template, you may need <int32_t> here
    std::vector<int32_t> returnVector = ColTable::convertToArray<int32_t>(std::make_shared<ColTable>(table));
    std::vector<int32_t> returnVector2 = ColTable::convertToArray<int32_t>(std::make_shared<ColTable>(table2));
    std::cout <<"Return";
    for (const auto& element : returnVector) {
        std::cout << element << " ";
    }

    //test for int32
    std::shared_ptr<ColTableData> dataPtr=table.getData();
    std::shared_ptr<ColTableData> dataPtr2=table2.getData();
    printValueBuffer(dataPtr,format::INT32);
    printValueBuffer(dataPtr2,format::INT32);
    
    //test for int64
    std::vector<std::int64_t> myVector64 = {1, 2, 3, 4, 5};
    std::vector<std::int64_t> myVector264 = {1, 2, 3, 4, 5,6,7,8,9};
    ColTable table64 = ColTable::FromVector<int64_t>(myVector64);
    ColTable table264= ColTable::FromVector<int64_t>(myVector264);
    std::shared_ptr<ColTableData> dataPtr64=table64.getData();
    std::shared_ptr<ColTableData> dataPtr264=table264.getData();
    printValueBuffer(dataPtr64,format::INT64);
    printValueBuffer(dataPtr264,format::INT64);

    std::vector<std::string> myVector3 = {"ssd","memory","gpu","bios","cpu","lm"};
    ColTable table3 = ColTable::FromVector<std::string>(myVector3);
    std::shared_ptr<ColTableData> dataPtr3 =table3.getData();
    printValueBuffer(dataPtr3,format::STRING);
    auto buffer1= dataPtr3->getBuffer();
    char* data = buffer1->as<char>(format::STRING);
    std::cout<<data;
    
    return 0;
}