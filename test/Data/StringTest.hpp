#include "Test.hpp"

class StringTest : public Test
{
  public:
    StringTest(const std::string &name = "");

    bool Run() override;
};
