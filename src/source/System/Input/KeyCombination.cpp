#include "System/Input/KeyCombination.hpp"
#include "Types/Types.hpp"

KeyCombination::KeyCombination(const Array<KeyCode> &keyCombination) : keys(keyCombination)
{
}

KeyCombination::KeyCombination(const KeyCombination &other) : keys(other.keys)
{
}

KeyCombination::KeyCombination(KeyCombination &&other) noexcept : keys(Types::Move(other.keys))
{
}

KeyCombination &KeyCombination::operator=(const KeyCombination &other)
{
    keys = other.keys;
    return *this;
}

bool KeyCombination::operator==(const KeyCombination &other) const
{
    if (keys.Length() != other.keys.Length())
        return false;
    for (size_t i = 0; i < keys.Length(); i++)
    {
        if (keys[i] != other.keys[i])
            return false;
    }
    return true;
}
