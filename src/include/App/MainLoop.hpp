#ifndef MAIN_LOOP_HPP
#define MAIN_LOOP_HPP

#include "pch.hpp"

class MainLoop
{
private:
    bool shouldContinue;

public:
    MainLoop();

    ~MainLoop();

    bool ShouldContinue() const;

    void Execute(WindowHandle mainWindowHandle);
};

#endif
