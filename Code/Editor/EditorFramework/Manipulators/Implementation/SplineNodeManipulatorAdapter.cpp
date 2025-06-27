#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Manipulators/SplineManipulatorAdapter.h>
#include <EditorFramework/Manipulators/SplineNodeManipulatorAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezSplineNodeManipulatorAdapter::ezSplineNodeManipulatorAdapter() = default;
ezSplineNodeManipulatorAdapter::~ezSplineNodeManipulatorAdapter() = default;

void ezSplineNodeManipulatorAdapter::Finalize()
{
  ConfigureGizmos();
}

void ezSplineNodeManipulatorAdapter::Update()
{
  BuildSpline();
  UpdateGizmoTransform();
}

void ezSplineNodeManipulatorAdapter::UpdateGizmoTransform()
{
}

void ezSplineNodeManipulatorAdapter::BuildSpline()
{
  const ezDocumentObject* pSplineObject = nullptr;
  {
    const ezDocumentObject* pSplineNodeObject = m_pObject->GetParent();
    if (pSplineNodeObject->GetParent() == nullptr)
      return;

    pSplineObject = pSplineNodeObject->GetParent();
  }

  const ezDocumentObject* pSplineComponent = nullptr;
  {
    ezVariantArray componentUuids;
    if (!pSplineObject->GetTypeAccessor().GetValues("Components", componentUuids))
      return;

    for (const auto& v : componentUuids)
    {
      if (v.IsA<ezUuid>())
      {
        pSplineComponent = pSplineObject->GetDocumentObjectManager()->GetObject(v.Get<ezUuid>());
        if (pSplineComponent != nullptr && pSplineComponent->GetType()->GetTypeName() == "ezSplineComponent")
          break;
      }
    }

    if (pSplineComponent == nullptr)
      return;
  }

  ezSplineManipulatorAdapter::BuildSpline(pSplineComponent, "Nodes", "Closed", m_Spline).AssertSuccess();
}

void ezSplineNodeManipulatorAdapter::ConfigureGizmos()
{
}
