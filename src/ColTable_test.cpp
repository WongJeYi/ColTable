#include <gtest/gtest.h>
#include "ColTable.hpp"
#include <cstdint>

// TEST(SuiteName, TestName)
TEST(ColTableTest, Int32RoundTrip) {
    // 1. Setup original data
    std::vector<int32_t> myVector = {1, 2, 3, 4, 5};
    std::vector<int32_t> myVector2 = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    // 2. Convert TO the ColTable format
    ColTable table = ColTable::FromVector<int32_t>(myVector);
    ColTable table2 = ColTable::FromVector<int32_t>(myVector2);

    // 3. Convert BACK to vectors
    // Note: If convertToArray is a template, you may need <int32_t> here
    std::vector<int32_t> returnVector = ColTable::convertToArray<int32_t>(std::make_shared<ColTable>(table));
    std::vector<int32_t> returnVector2 = ColTable::convertToArray<int32_t>(std::make_shared<ColTable>(table2));

    // 4. Verify results
    // Check if the sizes match first
    ASSERT_EQ(returnVector.size(), myVector.size());
    ASSERT_EQ(returnVector2.size(), myVector2.size());

    // Check if the contents are identical
    EXPECT_EQ(returnVector, myVector);
    EXPECT_EQ(returnVector2, myVector2);
}

TEST(ColTableTest,StringRoundTrip){
    std::vector<std::string> myVector3 = {"ssd","memory","gpu","bios","cpu","lm"};
    ColTable table3 = ColTable::FromVector<std::string>(myVector3);
    std::vector<std::string> returnVector3 = ColTable::convertToArray<std::string>(std::make_shared<ColTable>(table3));
    ASSERT_EQ(returnVector3.size(), myVector3.size());
    EXPECT_EQ(returnVector3, myVector3);
}