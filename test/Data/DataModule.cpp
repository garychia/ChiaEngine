#include "DataModule.hpp"

DataModule::DataModule() : Module()
{
    AddTest<StringTest>();
}
