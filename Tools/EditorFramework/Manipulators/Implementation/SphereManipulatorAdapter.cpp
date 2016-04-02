#include <PCH.h>
#include <EditorFramework/Manipulators/SphereManipulatorAdapter.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>

ezSphereManipulatorAdapter::ezSphereManipulatorAdapter()
{
}

ezSphereManipulatorAdapter::~ezSphereManipulatorAdapter()
{
}

void ezSphereManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument();

  auto* pWindow = ezQtDocumentWindow::FindWindowByDocument(pDoc);

  ezQtEngineDocumentWindow* pEngineWindow = qobject_cast<ezQtEngineDocumentWindow*>(pWindow);
  EZ_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  ezMat4 t;
  t.SetIdentity();
  t.SetTranslationMatrix(ezVec3(0, 0, 5));
  m_Gizmo.SetTransformation(t);
  m_Gizmo.SetVisible(true);

  m_Gizmo.SetOwner(pEngineWindow, nullptr);
}

void ezSphereManipulatorAdapter::Update()
{
  const ezSphereManipulatorAttribute* pAttr = static_cast<const ezSphereManipulatorAttribute*>(m_pManipulatorAttr);

  ezVariant value = m_pObject->GetTypeAccessor().GetValue(ezPropertyPath(pAttr->GetInnerRadiusProperty()));
  m_Gizmo.SetInnerSphere(value.ConvertTo<float>());
}
