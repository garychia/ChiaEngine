#ifndef KEY_COMBINATION_HPP
#define KEY_COMBINATION_HPP

#include "Data/Array.hpp"
#include "KeyCodes.hpp"

struct KeyCombination
{
    Array<KeyCode> keys;

    KeyCombination(const Array<KeyCode> &keyCombination = Array<KeyCode>());

    KeyCombination(const KeyCombination &other);

    KeyCombination(KeyCombination &&other) noexcept;

    KeyCombination &operator=(const KeyCombination &other);

    bool operator==(const KeyCombination &other) const;
};

#endif // KEY_COMBINATION_HPP
