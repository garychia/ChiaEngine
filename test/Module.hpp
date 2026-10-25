#ifndef MODULE_HPP
#define MODULE_HPP

#include <vector>

#include "Test.hpp"
#include "Types/Types.hpp"

class Module
{
  private:
    std::vector<Test *> tests;

  public:
    Module() : tests()
    {
    }

    Module(const Module &m) = delete;

    Module(Module &&m) noexcept : tests(Types::Move(m.tests))
    {
        m.tests.clear();
    }

    virtual ~Module()
    {
        for (auto &test : tests)
            delete test;
        tests.clear();
    }

    template <class T> void AddTest()
    {
        tests.push_back(new T());
    }

    template <class T, class... Args> void AddTest(const Args &...args)
    {
        tests.push_back(new T(args...));
    }

    virtual bool Run()
    {
        bool allSuccessful = true;
        for (auto &test : tests)
        {
            // Run every test even after a failure: with `allSuccessful && test->Run()`
            // the first failure would silently skip the rest of the module, hiding
            // further breakage behind it (#110). test->Run() must therefore come first.
            allSuccessful = test->Run() && allSuccessful;
        }
        return allSuccessful;
    }
};

#endif