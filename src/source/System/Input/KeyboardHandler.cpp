#include "System/Input/KeyboardHandler.hpp"
#include "Types/Types.hpp"

KeyboardHandler::KeyboardHandler() : lastChar(0), combination(), numOfKeysPressed(0)
{
    for (size_t i = 0; i < NumOfKeys; i++)
        pressedKeys[i] = 0;
}

KeyboardHandler &KeyboardHandler::GetSingleton()
{
    static KeyboardHandler handler;
    return handler;
}

void KeyboardHandler::ProcessKeyDown(KeyCode keyCode)
{
    if (pressedKeys[keyCode])
        return;
    pressedKeys[keyCode] = ++numOfKeysPressed;
    combination.Append(keyCode);
}

void KeyboardHandler::ProcessKeyUp(KeyCode keyCode)
{
    if (numOfKeysPressed)
        numOfKeysPressed--;
    pressedKeys[keyCode] = 0;
    DynamicArray<KeyCode> newCombination;
    for (size_t i = 0; i < combination.Length(); i++)
    {
        if (combination[i] == keyCode)
            continue;
        newCombination.Append(combination[i]);
    }
    combination = Types::Move(newCombination);
}

void KeyboardHandler::RecordChar(String::CharType c)
{
    lastChar = c;
}

void KeyboardHandler::OnWindowLoseFocus()
{
    combination = DynamicArray<KeyCode>();
    for (size_t i = 0; i < NumOfKeys; i++)
        pressedKeys[i] = 0;
}

KeyCombination KeyboardHandler::GetKeyCombination() const
{
    return KeyCombination(combination);
}
