#ifndef DATA_MODULE_HPP
#define DATA_MODULE_HPP

#include "Module.hpp"
#include "StringTest.hpp"
#include "ArrayTest.hpp"
#include "DynamicArrayTest.hpp"
#include "ListTest.hpp"
#include "SetTest.hpp"
#include "HashTableTest.hpp"
#include "PointersTest.hpp"

class DataModule : public Module
{
  public:
    DataModule();
};

#endif