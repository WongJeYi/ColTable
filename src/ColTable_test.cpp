#include <gtest/gtest.h>
#include "ColTable.hpp"
#include <cstdint>
#include <random>
#include <type_traits> // Required for std::is_floating_point_v, etc.
#include <string>
#include <list>

template<typename T>
std::vector<T> randomGenerator() {
    std::random_device rd;  
    std::mt19937 gen(rd());

    // 1. Properly generate the random length
    std::uniform_int_distribution<> len_dist(1, 99); 
    int length = len_dist(gen);
    
    std::vector<T> vec;
    vec.reserve(length);

    // 2. Declare 'i' properly
    for (int i = 0; i < length; i++) {
        
        // 3. Correct 'if constexpr' syntax
        if constexpr (std::is_floating_point_v<T>) {
            std::uniform_real_distribution<T> val_dist(0.0, 100.0);
            vec.push_back(val_dist(gen));
        } 
        else if constexpr (std::is_same_v<T, bool>) {
            std::bernoulli_distribution bool_dist(0.5); // 50% true, 50% false
            vec.push_back(bool_dist(gen));
        } 
        else if constexpr (std::is_unsigned_v<T>) {
            std::uniform_int_distribution<uint64_t> val_dist(0, 100); 
            vec.push_back(static_cast<T>(val_dist(gen)));
        } 
        else {
            std::uniform_int_distribution<int64_t> val_dist(-100, 100); 
            vec.push_back(static_cast<T>(val_dist(gen)));
        }
    }
    return vec;
}

// ==========================================
// Generic Test Runner
// ==========================================
template <typename T>
void TestRoundTrip(const std::string& typeName) {
    // 1. Setup random data
    std::vector<T> original = randomGenerator<T>();

    std::vector<T> add = randomGenerator<T>();

    // 2. Convert TO the ColTable format
    ColTable table = ColTable::FromVector<T>(original);
    
    std::vector<int64_t> len_ori={original.size()};
    table.insert_at(len_ori,add);
    size_t len={add.size()};
    table.remove_at(len_ori,len);
    // 3. Convert BACK to vectors (Using the Out-Parameter)
    std::vector<T> recovered;
    ColTable::convertToArray(std::make_shared<ColTable>(table), recovered);

    // 4. Verify results
    ASSERT_EQ(recovered.size(), original.size()) << "Size mismatch for type: " << typeName;
    EXPECT_EQ(recovered, original) << "Data mismatch for type: " << typeName;
}

// ==========================================
// The Actual GTest Suite
// ==========================================
TEST(ColTableTest, RoundTripAllNumericTypes_add_remove) {
    
    // --- Explicitly Sized Signed Integers ---
    TestRoundTrip<int8_t>("int8_t");
    TestRoundTrip<int16_t>("int16_t");
    TestRoundTrip<int32_t>("int32_t");
    TestRoundTrip<int64_t>("int64_t");

    // --- C++ Native Signed Integers (Only those strictly unique) ---
    // TestRoundTrip<short>("short"); 
    // TestRoundTrip<long>("long");
    TestRoundTrip<long long>("long long");

    // --- Explicitly Sized Unsigned Integers ---
    TestRoundTrip<uint8_t>("uint8_t");
    TestRoundTrip<uint16_t>("uint16_t");
    TestRoundTrip<uint32_t>("uint32_t");
    TestRoundTrip<uint64_t>("uint64_t");

    // --- C++ Native Unsigned Integers (Only those strictly unique) ---
    // TestRoundTrip<unsigned short>("unsigned short");
    // TestRoundTrip<unsigned long>("unsigned long");
    TestRoundTrip<unsigned long long>("unsigned long long");

    // --- Floating Point ---
    TestRoundTrip<float>("float");
    TestRoundTrip<double>("double");
}

TEST(ColTableTest, StringRoundTrip_add_remove) {
    std::vector<std::optional<std::string>> myVector3 = {"ssd", "memory", "gpu", "bios", "cpu", "lm"};
    std::vector<std::optional<std::string>> add = {"ssd", "memory", "gpu"};
    
    ColTable table3 = ColTable::FromVector<std::optional<std::string>>(myVector3);
    std::vector<int64_t> len={5};
    printValueBuffer(table3.getData(),format::STRING);
    table3.insert_at(len,add);
    printValueBuffer(table3.getData(),format::STRING);
    table3.remove_at(len,3);
    printValueBuffer(table3.getData(),format::STRING);

    // Using the Out-Parameter
    std::vector<std::optional<std::string>> returnVector3;
    ColTable::convertToArray(std::make_shared<ColTable>(table3), returnVector3);
    
    ASSERT_EQ(returnVector3.size(), myVector3.size());
    // 2. Loop through and compare the actual values safely
    for (size_t i = 0; i < myVector3.size(); ++i) {
        // If they both have a value, compare the underlying strings
        if (myVector3[i].has_value()) {
            EXPECT_EQ(returnVector3[i].value(), myVector3[i].value()) << "Mismatch at index " << i;
        }
    }
}

// Helper to generate a fully randomized 2D vector of double
std::vector<std::vector<double>> generateRandom2DVector(int maxRows, int maxCols) {
    std::random_device rd;  
    std::mt19937 gen(rd());
    
    // Distributions for our random numbers
    std::uniform_int_distribution<> row_dist(1, maxRows);
    std::uniform_int_distribution<> col_dist(1, maxCols); // 0 allows testing empty inner lists!
    std::uniform_real_distribution<double> val_dist(-1000, 1000); // Random data values

    int numRows = row_dist(gen);
    std::vector<std::vector<double>> result;
    result.reserve(numRows);
    
    for (int i = 0; i < numRows; ++i) {
        int numCols = col_dist(gen);
        std::vector<double> row;
        row.reserve(numCols);
        
        for (int j = 0; j < numCols; ++j) {
            row.push_back(val_dist(gen));
        }
        result.push_back(std::move(row));
    }
    
    return result;
}

TEST(ColTableTest, AoARoundTripRandom) {
    // 1. Generate random 2D data (Up to 2 rows, up to 3 items per row)
    std::vector<std::vector<double>> myVector = generateRandom2DVector(2, 3);
    std::vector<std::vector<double>> add = generateRandom2DVector(4, 3);
    
    // 2. Serialize to Columnar Format (Fixed extra '>' typo here)
    ColTable table = ColTable::FromVector<double>(myVector);
    int64_t len= myVector.size();
    std::vector<int64_t> lens={len};
    printValueBuffer(table.getData(),format::AoA);
    table.insert_at(lens,add);
    printValueBuffer(table.getData(),format::AoA);
    table.remove_at(lens,add.size());
    printValueBuffer(table.getData(),format::AoA);

    auto tablePtr = std::make_shared<ColTable>(table);
    
    // 3. Deserialize back to C++ Arrays using the Out-Parameter
    std::vector<std::vector<double>> returnVector;
    ColTable::convertToArray(tablePtr, returnVector);
    
    // 4. Assert exact structural and data match
    ASSERT_EQ(returnVector.size(), myVector.size());
    EXPECT_EQ(returnVector, myVector);
}

// Helper to generate a fully randomized 1D list of integers
std::list<int32_t> generateRandomList(int maxLength) {
    std::random_device rd;  
    std::mt19937 gen(rd());
    
    // Distributions for length and values
    std::uniform_int_distribution<> len_dist(0, maxLength); // 0 allows testing empty lists!
    std::uniform_int_distribution<int32_t> val_dist(-1000, 1000);

    int numElements = len_dist(gen);
    std::list<int32_t> result;
    for (int i = 0; i < numElements; ++i) {
        result.push_back(val_dist(gen));
    }
    
    return result;
}

TEST(ColTableTest, ListRoundTripRandom) {
    // 1. Generate random 1D list (Up to 50 items)
    std::list<int32_t> myList = generateRandomList(50);
    
    // 2. Serialize to Columnar Format
    ColTable table = ColTable::FromVector(myList);
    auto tablePtr = std::make_shared<ColTable>(table);
    
    // 3. Deserialize back using the Out-Parameter
    std::list<int32_t> returnVec;
    ColTable::convertToArray(tablePtr, returnVec);
    std::list<int32_t> returnList(returnVec.begin(), returnVec.end());
    
    // 4. Assert exact size and data match
    ASSERT_EQ(returnList.size(), myList.size());
    EXPECT_EQ(returnList, myList);
}

TEST(ColTableTest, StructRoundTripRandom) {
    // 1. Pure Data Struct (No ColTable logic inside!)
    struct ColTable_test {
        std::list<int32_t> m_myList;
        int64_t m_count;
        
        ColTable_test() = default;
        
        ColTable_test(std::list<int32_t> myList, int64_t count) {
            m_myList = myList;
            m_count = count;
        }

        bool operator==(const ColTable_test& other) const {
            return m_count == other.m_count && m_myList == other.m_myList;
        }
    };
    // original data
    ColTable_test p1(generateRandomList(5), 5);
    ColTable_test p2(generateRandomList(3), 3);
    std::vector<ColTable_test> structVec = {p1, p2};

    //insert data
    ColTable_test p3(generateRandomList(4), 4);
    ColTable_test p4(generateRandomList(2), 2);
    std::vector<ColTable_test> addVec = {p3, p4};

    ColTable table = ColTableStructMapper::Serialize(
        structVec,
        "m_myList", &ColTable_test::m_myList,
        "m_count", &ColTable_test::m_count
    );
    std::vector<int64_t> insert_indices = { static_cast<int64_t>(structVec.size()) };
    
    table.insert_struct_column(insert_indices, addVec, "m_myList", &ColTable_test::m_myList);
    table.insert_struct_column(insert_indices, addVec, "m_count", &ColTable_test::m_count);
    
    table.remove_at(insert_indices, addVec.size());
    
    auto tablePtr = std::make_shared<ColTable>(table);
    
    // 3. Deserialize back
    std::vector<ColTable_test> returnStruct;
    ColTableStructMapper::Deserialize(
        tablePtr, 
        returnStruct,
        "m_myList", &ColTable_test::m_myList,
        "m_count", &ColTable_test::m_count
    );
    
    // 4. Assert exact size and data match
    ASSERT_EQ(returnStruct.size(), structVec.size());
    EXPECT_EQ(returnStruct.data()->m_myList, structVec.data()->m_myList);
    EXPECT_EQ(returnStruct.data()->m_count, structVec.data()->m_count);
}