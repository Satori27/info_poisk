#include <gtest/gtest.h>
#include <vector>
#include <string>

#include "Tokenizer.cpp"

TEST(TokenizeTest, BasicLatin) {
    std::string text = "Hello world 123!";
    auto tokens = tokenize(text);
    std::vector<std::string> expected = {"hello", "world", "123"};
    EXPECT_EQ(tokens, expected);
}

TEST(TokenizeTest, Cyrillic) {
    std::string text = "Привет мир 45";
    auto tokens = tokenize(text);
    std::vector<std::string> expected = {"привет", "мир", "45"};
    EXPECT_EQ(tokens, expected);
}

TEST(TokenizeTest, Mixed) {
    std::string text = "Пример n6.2 и 3.9.2.1 главы";
    auto tokens = tokenize(text);
    std::vector<std::string> expected = {"пример", "n6", "2", "и", "3", "9", "2", "1", "главы"};
    EXPECT_EQ(tokens, expected);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
