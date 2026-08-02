// View 的方法實作 — 必須在 World 完整定義之後才可見。
// 由 World.hpp 在 class World 之後 #include 本檔。

template <class... Components> template <class C>
bool View<Components...>::HasAll(uint32_t entityIndex) const
{
    return pWorld->template HasComponentByIndex<C>(entityIndex);
}

template <class... Components>
bool View<Components...>::AllComponentsPresent(uint32_t entityIndex) const
{
    return (true && ... && HasAll<Components>(entityIndex));
}

template <class... Components> template <class C>
C *View<Components...>::GetComponentPtr(uint32_t entityIndex) const
{
    return pWorld->template GetComponentByIndex<C>(entityIndex);
}

template <class... Components>
typename View<Components...>::Entry View<Components...>::MakeEntry(uint32_t entityIndex) const
{
    return Entry(pWorld->GetEntityByIndex(entityIndex), GetComponentPtr<Components>(entityIndex)...);
}

template <class... Components>
View<Components...>::View(World *pWorld) : pWorld(pWorld), pDriver(nullptr)
{
    pDriver = pWorld->template GetPool<DriverType>();
}

template <class... Components>
bool View<Components...>::Iterator::Valid() const
{
    if (!pView->pDriver)
        return false;
    if (position >= pView->pDriver->GetNElements())
        return false;
    return pView->AllComponentsPresent(pView->pDriver->OwnerAt(position));
}

template <class... Components>
void View<Components...>::Iterator::SkipInvalid()
{
    while (position < pView->pDriver->GetNElements() && !Valid())
        position++;
}

template <class... Components>
View<Components...>::Iterator::Iterator(View *pView, size_t position) : pView(pView), position(position)
{
    SkipInvalid();
}

template <class... Components>
typename View<Components...>::Entry View<Components...>::Iterator::operator*() const
{
    return pView->MakeEntry(pView->pDriver->OwnerAt(position));
}

template <class... Components>
typename View<Components...>::Iterator &View<Components...>::Iterator::operator++()
{
    position++;
    SkipInvalid();
    return *this;
}

template <class... Components>
bool View<Components...>::Iterator::operator==(const Iterator &other) const
{
    return position == other.position;
}

template <class... Components>
bool View<Components...>::Iterator::operator!=(const Iterator &other) const
{
    return position != other.position;
}

template <class... Components>
typename View<Components...>::Iterator View<Components...>::begin()
{
    return Iterator(this, 0);
}

template <class... Components>
typename View<Components...>::Iterator View<Components...>::end()
{
    const size_t length = pDriver ? pDriver->GetNElements() : 0;
    return Iterator(this, length);
}
