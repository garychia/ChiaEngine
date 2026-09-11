#include "Display/GUI/EditorSession.hpp"

EditorSession::EditorSession() = default;

EditorSession::~EditorSession()
{
    mUndoStack.Clear();
}

void EditorSession::SetSelection(uint32_t entityIndex, bool hasSelection)
{
    mSelection.entityIndex = entityIndex;
    mSelection.hasSelection = hasSelection;
}

void EditorSession::ClearSelection()
{
    mSelection.entityIndex = 0;
    mSelection.hasSelection = false;
}

void EditorSession::Reset()
{
    mSelection = Selection();
    mUndoStack.Clear();
}