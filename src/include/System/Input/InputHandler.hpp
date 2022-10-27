#ifndef INPUT_HANDLER_HPP
#define INPUT_HANDLER_HPP

#include "Data/DynamicArray.hpp"
#include "InputHandleLayer.hpp"
#include "KeyCodes.hpp"
#include "Types/Types.hpp"

class InputHandler
{
  private:
    DynamicArray<InputHandleLayer *> layers;

    InputHandler();

  public:
    static InputHandler &GetSingleton();

    void AddInputLayer(InputHandleLayer *layer);

    bool HandleKeyboardInput(const KeyCombination &keys);

    bool HandleMouseInput(const MouseInfo &info);
};

#endif
