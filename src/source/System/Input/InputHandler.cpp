#include "System/Input/InputHandler.hpp"

InputHandler::InputHandler() : layers()
{
}

InputHandler &InputHandler::GetSingleton()
{
    static InputHandler singleton;
    return singleton;
}

void InputHandler::AddInputLayer(InputHandleLayer *layer)
{
    layers.Append(layer);
}

bool InputHandler::HandleKeyboardInput(const KeyCombination &keys)
{
    for (size_t i = 0; i < layers.Length(); i++)
    {
        if (layers[i]->ProcessKeyboardInput(keys))
            return true;
    }
    return false;
}

bool InputHandler::HandleMouseInput(const MouseInfo &info)
{
    for (size_t i = 0; i < layers.Length(); i++)
    {
        if (layers[i]->ProcessMouseInput(info))
            return true;
    }
    return false;
}
