#ifndef TEST_HPP
#define TEST_HPP

#include <iostream>
#include <string>


#define PRINT_ERR(msg)                                                                                                 \
    std::cerr << msg << std::endl                                                                                      \
              << "In file " << __FILE__ << ", "                                                                        \
              << "line " << __LINE__ << "." << std::endl
#define EXPECT_TRUE(cond, msg, return_false)                                                                           \
    if (!(cond))                                                                                                       \
    {                                                                                                                  \
        std::cerr << "Expected '" #cond "' to be true." << std::endl;                                                  \
        PRINT_ERR(msg);                                                                                                \
        if ((return_false))                                                                                            \
        {                                                                                                              \
            return false;                                                                                              \
        }                                                                                                              \
    }

#define TEST_MESSAGE(test_name) std::cout << "Testing "##test_name "..." << std::endl
#define SUCCESS_MESSAGE(test_name) std::cout << ##test_name " - DONE" << std::endl

class Test
{
  private:
    std::string name;

  public:
    Test(const std::string &name = "") : name(name)
    {
    }

    std::string GetName() const
    {
        return name;
    }

    virtual bool Run() = 0;
};

#endif
