#include "Display/GUI/HierarchyRow.hpp"

HierarchyRow::HierarchyRow(const Point2D &windowSize, const Border &border, const Entity &entity)
    : Button(windowSize, border), entity(entity), rowClicked()
{
}

void HierarchyRow::OnClicked(const Point2D &coordinates)
{
    Button::OnClicked(coordinates); // 維持 WithIn guard + clickEvent
    if (WithIn(coordinates))
        rowClicked.Invoke(entity);
}

Entity HierarchyRow::GetEntity() const
{
    return entity;
}
