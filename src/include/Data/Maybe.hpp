#ifndef MAYBE_HPP
#define MAYBE_HPP

template <class T> class Maybe
{
  private:
    bool valid;

    T data;

  public:
    Maybe() : valid(false), data()
    {
    }

    template <class U> Maybe(const U &value) : valid(true), data(value)
    {
    }

    bool IsValid() const
    {
        return valid;
    }

    void Remove()
    {
        valid = false;
    }

    T &Get()
    {
        return data;
    }

    const T &Get() const
    {
        return data;
    }

    operator bool() const
    {
        return valid;
    }

    template <class U> Maybe<T> &operator=(const U &value)
    {
        valid = true;
        data = value;
        return *this;
    }

    template <class U> Maybe<T> &operator=(const Maybe<U> &other)
    {
        valid = other.valid;
        data = other.data;
        return *this;
    }

    template <class U> bool operator==(const Maybe<U> &other) const
    {
        return valid && other.valid && data == other.data;
    }

    template <class U> friend class Maybe;
};

#endif
