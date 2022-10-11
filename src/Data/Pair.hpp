#ifndef PAIR_HPP
#define PAIR_HPP

template <class T, class U> class Pair
{
  private:
    T key;

    U value;

  public:
    T &Key()
    {
        return key;
    }

    const T &Key() const
    {
        return key;
    }

    U &Value()
    {
        return value;
    }

    const U &Value() const
    {
        return value;
    }

    Pair &operator=(const Pair &other)
    {
        key = other.key;
        value = other.value;
        return *this;
    }
};

#endif // PAIR_HPP
