#include "DataModule.hpp"

DataModule::DataModule() : Module()
{
    AddTest<StringTest>();
    AddTest<ArrayTest>();
    AddTest<DynamicArrayTest>();
    AddTest<ListTest>();
    AddTest<HashTableTest>();
    AddTest<PointersTest>();
    AddTest<OwnershipTest>();
}