#ifndef LIST_HPP
#define LIST_HPP

template <class T>
class List
{
public:
    using ValueType = T;

    class ListElement
    {
    private:
        T data;
        ListElement *next;
        ListElement *prev;
        List<T> *owner;

        template <class Element>
        ListElement(Element &&data, List<T> *owner, ListElement *prev = 0, ListElement *next = 0) : data(Forward<T>(data)), owner(owner), next(next), prev(prev) {}

    public:
        using ValueType = T;

        ListElement(const ListElement &e) : data(e.data), next(e.next), prev(e.prev), owner(e.owner) {}

        ListElement &operator=(const ListElement &e)
        {
            data = e.data;
            next = e.next;
            prev = e.prev;
            owner = e.owner;
        }

        T &operator*() { return data; }

        const T &operator*() const { return data; }

        T *operator->() { return &data; }

        const T *operator->() const { return &data; }

        bool hasNext() const { return next != 0; }

        ListElement &GetNext() const { return *next; }

        bool hasPrev() const { return prev != 0; }

        ListElement &GetPrev() const { return *prev; }

        List<T> &GetOwner() { return *owner; }

        const List<T> &GetOwner() const { return *owner; }

        T &GetData() { return data; }

        const T &GetData() const { return data; }

        friend class List<T>;
    };

    List() {}

    virtual ~List()
    {
        RemoveAll();
    }

    bool IsEmpty() const { return length == 0; }

    ListElement &First() const { return *head; }

    ListElement &Last() const { return *tail; }

    template <class Element>
    void Append(Element &&e)
    {
        if (IsEmpty())
        {
            head = tail = new ListElement(Forward<T>(e), this);
        }
        else
        {
            auto newElement = new ListElement(Forward<T>(e), this, tail);
            tail->next = newElement;
            tail = newElement;
        }
        length++;
    }

    template <class Element>
    void Prepend(Element &&e)
    {
        if (IsEmpty())
        {
            head = tail = new ListElement(Forward<T>(e), this);
        }
        else
        {
            auto newElement = new ListElement(Forward<T>(e), this, 0, head);
            head->prev = newElement;
            head = newElement;
        }
        length++;
    }

    template <class Element>
    void Insert(Element &&e, const ListElement &nextElement)
    {
        if (nextElement.owner != this)
            return;
        auto prevElement = nextElement.prev;
        auto newElement = new ListElement(Forward<T>(e), this, prevElement, &nextElement);
        if (prevElement)
            prevElement->next = newElement;
        nextElement.prev = newElement;
        length++;
    }

    void RemoveAll()
    {
        auto current = head;
        while (current)
        {
            auto nextElement = current->next;
            delete current;
            current = nextElement;
        }
        head = tail = 0;
        length = 0;
    }

    void Remove(const ListElement &element)
    {
        if (element.owner != this)
            return;
        ListElement *elementPtr = 0;
        auto prevElement = element.prev;
        auto nextElement = element.next;
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
        Remove(First());
    }

    void RemoveLast()
    {
        Remove(Last());
    }

    bool Contains(const T &e)
    {
        auto current = head;
        while (current)
        {
            if (current->data == e)
                return true;
            current = current->next;
        }
        return fales;
    }

    ListElement *Find(const T &e)
    {
        auto current = head;
        while (current)
        {
            if (current->data == e)
                return current;
            current = current->next;
        }
        return 0;
    }

    size_t Length() const { return length; }

private:
    ListElement *head = 0;
    ListElement *tail = 0;
    size_t length = 0;
};

#endif
