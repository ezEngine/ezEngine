#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/ClickGizmo.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>
#include <Foundation/Containers/DynamicArray.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>

struct ezGizmoEvent;

/// \brief Makes an array of ezExposedBone properties editable in the viewport
///
/// Enabled by attaching the ezBoneManipulatorAttribute.
class ezBoneManipulatorAdapter : public ezManipulatorAdapter
{
public:
  ezBoneManipulatorAdapter();
  ~ezBoneManipulatorAdapter();

protected:
  virtual void Finalize() override;

  void MigrateSelection();

  virtual void Update() override;
  void RotateGizmoEventHandler(const ezGizmoEvent& e);
  void ClickGizmoEventHandler(const ezGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  struct ElementGizmo
  {
    ezMat4 m_Offset;
    ezMat4 m_InverseOffset;
    ezRotateGizmo m_RotateGizmo;
    ezClickGizmo m_ClickGizmo;
  };

  ezVariantArray m_Keys;
  ezDynamicArray<ezExposedBone> m_Bones;
  ezDeque<ElementGizmo> m_Gizmos;
  ezTransform m_RootTransform = ezTransform::IdentityTransform();

  void RetrieveBones();
  void ConfigureGizmos();
  void SetTransform(ezUInt32 uiBone, const ezTransform& value);
  ezMat4 ComputeFullTransform(ezUInt32 uiBone) const;
  ezMat4 ComputeParentTransform(ezUInt32 uiBone) const;

  static ezString s_LastSelectedBone;
};
