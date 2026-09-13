#include "Display/GUI/EditorSession.hpp"

EditorSession::EditorSession(SceneSystem *pSceneSystem, CameraController *pCameraController,
                             SimRecorder *pSimRecorder)
    : mpSceneSystem(pSceneSystem), mpCameraController(pCameraController), mpSimRecorder(pSimRecorder),
      mSelection(), mUndoStack(), mPanes()
{
}

EditorSession::~EditorSession()
{
    mUndoStack.Clear();
}

void EditorSession::Select(uint32_t entityIndex)
{
    mSelection.entityIndex = entityIndex;
    mSelection.hasSelection = true;
}

void EditorSession::ClearSelection()
{
    mSelection = Selection();
}

bool EditorSession::HandleEditorShortcut(bool hasCtrl, bool hasZ, bool hasY)
{
    if (hasCtrl && hasZ)
    {
        mUndoStack.Undo();
        return true;
    }
    if (hasCtrl && hasY)
    {
        mUndoStack.Redo();
        return true;
    }
    return false;
}

PanelPane *EditorSession::RegisterPane(SharedPtr<PanelPane> pPane)
{
    if (!pPane)
        return nullptr;
    if (PanelPane *pExisting = GetPane(pPane->GetTitle()))
        return pExisting;
    mPanes.Append(pPane);
    return pPane.GetRaw();
}

PanelPane *EditorSession::GetPane(const String &title)
{
    for (size_t i = 0; i < mPanes.GetNElements(); i++)
        if (mPanes[i]->GetTitle() == title)
            return mPanes[i].GetRaw();
    return nullptr;
}

void EditorSession::SetPaneCollapsed(const String &title, bool collapsed)
{
    if (PanelPane *pPane = GetPane(title))
        pPane->SetCollapsed(collapsed);
}

void EditorSession::Reset()
{
    mSelection = Selection();
    mUndoStack.Clear();
    mPanes.RemoveAll();
}