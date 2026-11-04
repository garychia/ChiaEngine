#include "System/Input/InputRuntime.hpp"

#include <cstring>

InputRuntime::InputRuntime()
{
    std::memset(mKeyCurrent, 0, sizeof(mKeyCurrent));
    std::memset(mKeyPrevious, 0, sizeof(mKeyPrevious));
}

InputRuntime::~InputRuntime()
{
}

void InputRuntime::RegisterBinding(InputAction action, const KeyCombination &keys)
{
    mBindings.Append(InputBinding(action, keys));
}

void InputRuntime::UnregisterBinding(InputAction action, const KeyCombination &keys)
{
    for (size_t i = 0; i < mBindings.GetNElements(); i++)
    {
        if (mBindings[i].action == action && mBindings[i].keys == keys)
        {
            size_t last = mBindings.GetNElements() - 1;
            if (i != last)
                mBindings[i] = mBindings[last];
            mBindings.RemoveLast();
            return;
        }
    }
}

void InputRuntime::SetKeyDown(unsigned int keyCode)
{
    if (keyCode < 256)
        mKeyCurrent[keyCode] = true;
}

void InputRuntime::SetKeyUp(unsigned int keyCode)
{
    if (keyCode < 256)
        mKeyCurrent[keyCode] = false;
}

void InputRuntime::BeginFrame()
{
    std::memcpy(mKeyPrevious, mKeyCurrent, sizeof(mKeyCurrent));
}

void InputRuntime::EndFrame()
{
    // previous 已在 BeginFrame 時擷取,current 在幀中透過 SetKeyDown/SetKeyUp 更新
}

bool InputRuntime::IsCombinationDown(const KeyCombination &keys) const
{
    if (keys.keys.Length() == 0)
        return false;
    for (size_t i = 0; i < keys.keys.Length(); i++)
    {
        unsigned int k = static_cast<unsigned int>(keys.keys[i]);
        if (k >= 256 || !mKeyCurrent[k])
            return false;
    }
    return true;
}

bool InputRuntime::IsActionPressed(InputAction action) const
{
    for (size_t i = 0; i < mBindings.GetNElements(); i++)
    {
        if (mBindings[i].action == action)
        {
            bool cur = IsCombinationDown(mBindings[i].keys);
            bool prev = true;
            for (size_t j = 0; j < mBindings[i].keys.keys.Length(); j++)
            {
                unsigned int k = static_cast<unsigned int>(mBindings[i].keys.keys[j]);
                if (k >= 256 || !mKeyPrevious[k])
                {
                    prev = false;
                    break;
                }
            }
            if (cur && !prev)
                return true;
        }
    }
    return false;
}

bool InputRuntime::IsActionReleased(InputAction action) const
{
    for (size_t i = 0; i < mBindings.GetNElements(); i++)
    {
        if (mBindings[i].action == action)
        {
            bool cur = IsCombinationDown(mBindings[i].keys);
            bool prev = true;
            for (size_t j = 0; j < mBindings[i].keys.keys.Length(); j++)
            {
                unsigned int k = static_cast<unsigned int>(mBindings[i].keys.keys[j]);
                if (k >= 256 || !mKeyPrevious[k])
                {
                    prev = false;
                    break;
                }
            }
            if (!cur && prev)
                return true;
        }
    }
    return false;
}

bool InputRuntime::IsActionHeld(InputAction action) const
{
    for (size_t i = 0; i < mBindings.GetNElements(); i++)
    {
        if (mBindings[i].action == action && IsCombinationDown(mBindings[i].keys))
            return true;
    }
    return false;
}
