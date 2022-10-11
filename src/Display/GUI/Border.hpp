#ifndef BORDER_HPP
#define BORDER_HPP

struct Border
{
    struct Distances
    {
        float top;
        float bottom;
        float left;
        float right;

        Distances(float top = 0.f, float bottom = 0.f, float left = 0.f, float right = 0.f)
            : top(top), bottom(bottom), left(left), right(right)
        {
        }
    };

    bool sizeFixed;
    bool relative;

    float width;
    float height;

    Distances margin;
    Distances padding;

    Border(bool sizeFixed = false, float width = 0.f, float height = 0.f, bool relative = false,
           Distances margin = Distances(), Distances padding = Distances());
};

Border::Border(bool sizeFixed, float width, float height, bool relative, Distances margin, Distances padding)
    : sizeFixed(sizeFixed), width(width), height(height), relative(relative), margin(margin), padding(padding)
{
}

#endif
