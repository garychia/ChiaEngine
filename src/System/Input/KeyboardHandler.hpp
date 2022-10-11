#ifndef KEYBOARD_HANDLER_HPP
#define KEYBOARD_HANDLER_HPP

#include "Data/String.hpp"
#include "Data/DynamicArray.hpp"
#include "System/Input/KeyCodes.hpp"
#include "System/Input/KeyCombination.hpp"

constexpr unsigned int NumOfKeys = 256;

class KeyboardHandler
{
  private:
    String::CharType lastChar;

    DynamicArray<KeyCode> combination;

    unsigned int pressedKeys[NumOfKeys];

    unsigned int numOfKeysPressed;

    KeyboardHandler();

  public:
    static KeyboardHandler &GetSingleton();

    void ProcessKeyDown(KeyCode keyCode);

    void ProcessKeyUp(KeyCode keyCode);

    void RecordChar(String::CharType c);

    void OnWindowLoseFocus();

    KeyCombination GetKeyCombination() const;
};

#endif
