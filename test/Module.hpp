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
    Module::Module() : tests()
    {
    }

    Module::Module(const Module &m) = delete;

    Module::Module(Module &&m) noexcept : tests(Move(m.tests))
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
            allSuccessful = allSuccessful && test->Run();
        }
        return allSuccessful;
    }
};

#endif