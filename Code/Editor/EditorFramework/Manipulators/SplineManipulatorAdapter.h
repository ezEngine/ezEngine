#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <Core/Graphics/Spline.h>
#include <EditorFramework/Gizmos/ClickGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct ezGizmoEvent;

/// \brief Makes spline nodes editable in the editor by providing small helper gizmos to easily add new nodes.
///
/// Enabled by attaching the ezSplineManipulatorAttribute.
class ezSplineManipulatorAdapter : public ezManipulatorAdapter
{
public:
  ezSplineManipulatorAdapter();
  ~ezSplineManipulatorAdapter();

  static ezResult BuildSpline(const ezDocumentObject* pSplineComponent, ezStringView sNodesPropertyName, ezStringView sClosedPropertyName, ezSpline& out_spline, ezStringView sNodeName = ezStringView(), ezUInt32* out_pNodeIndex = nullptr);
  static const ezDocumentObject* FindSplineNodeComponent(const ezDocumentObject* pSplineObject, ezStringView sNodeName);
  static ezResult FillControlPointFromNodeComponent(const ezDocumentObject* pNodeComponent, ezSpline::ControlPoint& out_cp);

protected:
  virtual void Finalize() override;

  virtual void Update() override;
  void ClickGizmoEventHandler(const ezGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  void BuildSpline();
  void ConfigureGizmos();
  ezString MakeUniqueName(ezStringView sSuggestedName);

  ezSpline m_Spline;

  ezDeque<ezClickGizmo> m_Gizmos;
};
