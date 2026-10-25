#include "AllModules.hpp"

#include <iostream>

// #110: the exit code is the only signal CI has. Run() already aggregates every
// module's result, so propagate it rather than discarding it — otherwise a failing
// assertion can never fail the build.
int main(void)
{
    AllModules moduleTest;
    const bool allSuccessful = moduleTest.Run();
    if (!allSuccessful)
    {
        std::cerr << "TESTS FAILED" << std::endl;
    }
    return allSuccessful ? 0 : 1;
}
