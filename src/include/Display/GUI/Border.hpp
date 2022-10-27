#ifndef BORDER_HPP
#define BORDER_HPP

struct Border
{
    struct Length
    {
        float value;

        bool relative;

        Length(float value = 0.f, bool relative = false);

        bool IsRelative() const;

        float operator*(float num) const;

        float operator*(const Length &length) const;

        operator float() const
        {
            return value;
        }

        Length &operator=(float newValue)
        {
            value = newValue;
            return *this;
        }
    };

    struct Distances
    {
        Length top;
        Length bottom;
        Length left;
        Length right;

        Distances(const Length &top = Length(0.f), const Length &bottom = Length(0.f), const Length &left = Length(0.f),
                  const Length &right = Length(0.f));
    };

    Length xPos;
    Length yPos;

    Length width;
    Length height;

    Distances margin;
    Distances padding;

    Border(const Length &xPos = Length(), const Length &yPos = Length(), const Length &width = Length(),
           const Length &height = Length(), const Distances &margin = Distances(),
           const Distances &padding = Distances());
};

#endif
