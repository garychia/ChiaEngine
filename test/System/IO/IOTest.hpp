#ifndef IO_TEST_HPP
#define IO_TEST_HPP

#include "Data/String.hpp"
#include "Test.hpp"


class IOTest : public Test
{
  private:
    String testPath;

  public:
    IOTest(const String &testPath);
    virtual bool Run() override;
};

#endif
