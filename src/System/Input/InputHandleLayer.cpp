#include "InputHandleLayer.hpp"

InputHandleLayer::InputHandleLayer() : keyInputCallback(), mouseInputCallback()
{
}

bool InputHandleLayer::ProcessKeyboardInput(const KeyCombination &combination)
{
    return keyInputCallback.Valid() && keyInputCallback(combination);
}

bool InputHandleLayer::ProcessMouseInput(const MouseInfo &info)
{
    return mouseInputCallback.Valid() && mouseInputCallback(info);
}
