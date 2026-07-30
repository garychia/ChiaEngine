#include "DataModule.hpp"

DataModule::DataModule() : Module()
{
    AddTest<StringTest>();
    AddTest<ArrayTest>();
    AddTest<DynamicArrayTest>();
    AddTest<ListTest>();
    AddTest<SetTest>();
    AddTest<HashTableTest>();
    AddTest<PointersTest>();
}