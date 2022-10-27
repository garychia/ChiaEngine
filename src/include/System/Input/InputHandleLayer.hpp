#ifndef INPUT_HANDLE_LAYER_HPP
#define INPUT_HANDLE_LAYER_HPP

#include "KeyCombination.hpp"
#include "MouseInfo.hpp"
#include "System/Operation/Event.hpp"

class InputHandleLayer
{
  public:
    Callback<bool(const KeyCombination &)> keyInputCallback;

    Callback<bool(const MouseInfo &)> mouseInputCallback;

    InputHandleLayer();

    bool ProcessKeyboardInput(const KeyCombination &combination);

    bool ProcessMouseInput(const MouseInfo &info);
};

#endif
