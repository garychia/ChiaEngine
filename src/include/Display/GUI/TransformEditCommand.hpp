#ifndef TRANSFORM_EDIT_COMMAND_HPP
#define TRANSFORM_EDIT_COMMAND_HPP

#include "Display/GUI/EditorSession.hpp"
#include "Display/GUI/InspectorButton.hpp"
#include "Scene/SceneSystem.hpp"
#include "Scene/TransformComponent.hpp"

// ADR-0001 D5:editor-time transform edit as an undoable command.
// Holds the scene pointer + target + axis + direction; Apply/Revert are
// inverse delta edits (Revert == apply with flipped sign), so no snapshot
// of the old value is needed and the command stays trivially copyable.
//
// NOTE: this header intentionally lives OUTSIDE EditorSession.hpp — it is the
// GUI-editing layer that couples the command to SceneSystem. EditorSession
// itself stays window/scene-agnostic and only knows UndoStack<EditorCommand*>.
class TransformEditCommand : public EditorCommand
{
  public:
    TransformEditCommand(SceneSystem *pScene, uint32_t entityIndex, InspectorAxis axis, float sign)
        : mpScene(pScene), mEntityIndex(entityIndex), mAxis(axis), mSign(sign)
    {
    }

    void Apply() override
    {
        EditTransformComponent(mpScene, mEntityIndex, mAxis, mSign);
    }

    void Revert() override
    {
        // Inverse edit: subtract what Apply added.
        EditTransformComponent(mpScene, mEntityIndex, mAxis, -mSign);
    }

    String Description() const override
    {
        return String(u"EditTransform");
    }

  private:
    SceneSystem *mpScene;
    uint32_t mEntityIndex;
    InspectorAxis mAxis;
    float mSign;
};

#endif // TRANSFORM_EDIT_COMMAND_HPP