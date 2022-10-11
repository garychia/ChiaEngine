#ifndef LIST_HPP
#define LIST_HPP

template <class T> class List
{
  private:
    struct Element
    {
        T data;
        List<T> *owner;
        Element *next;
        Element *prev;

        template <class DataType>
        Element(DataType &&data, List<T> *owner, Element *prev = 0, Element *next = 0)
            : data(Forward<decltype(data)>(data)), owner(owner), next(next), prev(prev)
        {
        }

        Element(const Element &e) : data(e.data), owner(e.owner), next(e.next), prev(e.prev)
        {
        }

        template <class ElementType> Element &operator=(ElementType &&e)
        {
            data = Forward<decltype(e)>(e.data);
            next = e.next;
            prev = e.prev;
            owner = e.owner;
        }

        T &operator*()
        {
            return data;
        }

        const T &operator*() const
        {
            return data;
        }

        T *operator->()
        {
            return &data;
        }

        const T *operator->() const
        {
            return &data;
        }

        friend class List<T>;
        friend class List<T>::Iterator;
    };

  public:
    using ValueType = T;

    class Iterator
    {
      private:
        List<T> *owner;
        Element *prev;
        Element *current;

      public:
        Iterator(List<T> *pList = nullptr) : owner(pList), prev(nullptr), current(nullptr)
        {
            if (pList)
                current = pList->head;
        }

        Iterator(Element *pElement) : owner(nullptr), prev(nullptr), current(nullptr)
        {
            if (pElement)
            {
                owner = pElement->owner;
                prev = pElement->prev;
                current = pElement;
            }
        }

        Iterator(const Iterator &other) : owner(other.owner), current(other.current)
        {
        }

        T &operator*()
        {
            return current->data;
        }

        T *operator->()
        {
            return &current->data;
        }

        Iterator &operator++()
        {
            if (current)
            {
                prev = current;
                current = current->next;
            }
            return *this;
        }

        Iterator operator++(int)
        {
            if (current)
            {
                prev = current;
                current = current->next;
            }
            return *this;
        }

        Iterator &operator--()
        {
            if (prev)
            {
                current = prev;
                prev = prev->prev;
            }
            return *this;
        }

        Iterator operator--(int)
        {
            if (prev)
            {
                current = prev;
                prev = prev->prev;
            }
            return *this;
        }

        bool operator==(const Element &other) const
        {
            return owner == other.owner && current == other.current;
        }

        bool operator!=(const Element &other) const
        {
            return owner != other.owner || current != other.current;
        }

        friend class List<T>;
    };

    List()
    {
    }

    virtual ~List()
    {
        RemoveAll();
    }

    bool IsEmpty() const
    {
        return length == 0;
    }

    Iterator First() const
    {
        return Iterator(this);
    }

    Iterator Last() const
    {
        Iterator itr(this);
        itr.prev = tail;
        itr.current = nullptr;
        return itr;
    }

    template <class DataType> void Append(DataType &&e)
    {
        if (IsEmpty())
        {
            head = tail = new Element(Forward<decltype(e)>(e), this);
        }
        else
        {
            auto newElement = new Element(Forward<decltype(e)>(e), this, tail);
            tail->next = newElement;
            tail = newElement;
        }
        length++;
    }

    template <class Element> void Prepend(Element &&e)
    {
        if (IsEmpty())
        {
            head = tail = new Element(Forward<T>(e), this);
        }
        else
        {
            auto newElement = new Element(Forward<T>(e), this, 0, head);
            head->prev = newElement;
            head = newElement;
        }
        length++;
    }

    template <class DataType> void Insert(DataType &&e, const Iterator &nextItr)
    {
        if (nextElement.owner != this)
            return;
        Element *nextElement = nextItr.current;
        Element *prevElement = nextElement->prev;
        auto newElement = new Element(Forward<decltype(e)>(e), this, prevElement, nextElement);
        if (prevElement)
            prevElement->next = newElement;
        if (nextElement)
            nextElement->prev = newElement;
        if (IsEmpty())
            head = tail = newElement;
        length++;
    }

    void RemoveAll()
    {
        auto current = head;
        while (current)
        {
            Element *nextElement = current->next;
            delete current;
            current = nextElement;
        }
        head = tail = 0;
        length = 0;
    }

    void Remove(const Iterator &itr)
    {
        if (itr.owner != this || !itr.current)
            return;
        const Element &element = *itr.current;
        Element *elementPtr = 0;
        Element *prevElement = element.prev;
        Element *nextElement = element.next;
        if (prevElement)
        {
            elementPtr = prevElement->next;
            prevElement->next = nextElement;
        }
        if (nextElement)
        {
            elementPtr = nextElement->prev;
            nextElement->prev = prevElement;
        }
        if (!elementPtr)
            elementPtr = head;
        delete elementPtr;
        length--;
        if (IsEmpty())
            head = tail = 0;
    }

    void RemoveFirst()
    {
        if (IsEmpty())
            return;
        Remove(First());
    }

    void RemoveLast()
    {
        if (IsEmpty())
            return;
        Remove(Last());
    }

    bool Contains(const T &e) const
    {
        auto current = head;
        while (current)
        {
            if (current->data == e)
                return true;
            current = current->next;
        }
        return false;
    }

    Iterator Find(const T &e) const
    {
        Iterator itr = First();
        while (itr != Last())
        {
            if (*itr == e)
                return itr;
            itr++;
        }
        return itr;
    }

    size_t Length() const
    {
        return length;
    }

  private:
    Element *head = 0;
    Element *tail = 0;
    size_t length = 0;
};

#endif
