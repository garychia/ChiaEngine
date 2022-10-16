#include "Border.hpp"

Border::Length::Length(float value, bool relative) : value(value), relative(relative)
{
}

bool Border::Length::IsRelative() const
{
    return relative;
}

float Border::Length::operator*(float num) const
{
    return value * num;
}

float Border::Length::operator*(const Length &length) const
{
    return value * length.value;
}

Border::Distances::Distances(const Length &top, const Length &bottom, const Length &left, const Length &right)
    : top(top), bottom(bottom), left(left), right(right)
{
}

Border::Border(const Length &xPos, const Length &yPos, const Length &width, const Length &height,
               const Distances &margin, const Distances &padding)
    : xPos(xPos), yPos(yPos), width(width), height(height), margin(margin), padding(padding)
{
}
