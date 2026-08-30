#ifndef SELECTION_HPP
#define SELECTION_HPP

#include <cstdint>

// ADR-0001 D2:editor 選取的唯一真相來源。
// Panel 擁有並維護這一份;InspectorLayer 只持有指標並讀取,不複製 entityIndex。
// 避免 Panel::selectedEntity 與 InspectorLayer::selectedEntityIndex 雙份狀態不同步。
struct Selection
{
    uint32_t entityIndex = 0;
    bool hasSelection = false;
};

#endif // SELECTION_HPP
