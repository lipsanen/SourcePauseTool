#include "gtest/gtest.h"
#include "spt/utils/logfile.hpp"

#include <filesystem>


std::string GetStringFromFile(const char* filepath)
{
    std::ifstream stream;
    stream.open(filepath);

    std::ostringstream oss;
    oss << stream.rdbuf();

    return oss.str();
}

TEST(LogFile, LoggingWorks)
{
    std::filesystem::remove_all("./temp/");

    {
        LogFile file;
        file.Log("./temp/test.log", "This is a test");
        file.Log("./temp/test2.log", "This is a test2");
    }

    std::string output = GetStringFromFile("./temp/test.log");
    std::string output2 = GetStringFromFile("./temp/test2.log");
    EXPECT_EQ(output, "This is a test");
    EXPECT_EQ(output2, "This is a test2");
}

TEST(LogFile, LoggingWorksWithEditedFiles)
{
    std::filesystem::remove_all("./temp/");

    {
        LogFile file;
        file.Log("./temp/test.log", "This is a test");
        std::filesystem::resize_file("./temp/test.log", 0);
        file.Log("./temp/test.log", "This is a test2");

    }

    std::string output = GetStringFromFile("./temp/test.log");
    EXPECT_EQ(output, "This is a test2");
}


TEST(LogFile, WtfWhyIsThisBroken)
{
    std::ofstream stream;
    stream.open("./temp/test.txt", std::ios::app);
    stream.write("xd!", 3);
    stream.flush();
}