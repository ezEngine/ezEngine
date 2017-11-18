#include <PCH.h>
#include <EditorPluginScene/EditTools/GreyBoxEditTool.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGreyBoxEditTool, 1, ezRTTIDefaultAllocator<ezGreyBoxEditTool>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezGreyBoxEditTool::ezGreyBoxEditTool()
{

}

ezGreyBoxEditTool::~ezGreyBoxEditTool()
{

}

ezEditorInputContext* ezGreyBoxEditTool::GetEditorInputContextOverride()
{
  if (IsActive())
    return &m_DrawBoxGizmo;

  return nullptr;
}

ezEditToolSupportedSpaces ezGreyBoxEditTool::GetSupportedSpaces() const
{
  return ezEditToolSupportedSpaces::WorldSpaceOnly;
}

bool ezGreyBoxEditTool::GetSupportsMoveParentOnly() const
{
  return false;
}

void ezGreyBoxEditTool::UpdateGizmoState()
{
  ezManipulatorManager::GetSingleton()->HideActiveManipulator(GetDocument(), GetDocument()->GetActiveEditTool() != nullptr);

  m_DrawBoxGizmo.SetVisible(IsActive());
  m_DrawBoxGizmo.SetTransformation(ezTransform::Identity());
}

void ezGreyBoxEditTool::GameObjectEventHandler(const ezGameObjectEvent& e)
{
  switch (e.m_Type)
  {
  case ezGameObjectEvent::Type::ActiveEditToolChanged:
    UpdateGizmoState();
    break;
  }
}

void ezGreyBoxEditTool::ManipulatorManagerEventHandler(const ezManipulatorManagerEvent& e)
{
  if (!IsActive())
    return;

  // make sure the gizmo is deactivated when a manipulator becomes active
  if (e.m_pDocument == GetDocument() && e.m_pManipulator != nullptr && e.m_pSelection != nullptr && !e.m_pSelection->IsEmpty() && !e.m_bHideManipulators)
  {
    GetDocument()->SetActiveEditTool(nullptr);
  }
}

void ezGreyBoxEditTool::OnConfigured()
{
  GetDocument()->m_GameObjectEvents.AddEventHandler(ezMakeDelegate(&ezGreyBoxEditTool::GameObjectEventHandler, this));
  ezManipulatorManager::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezGreyBoxEditTool::ManipulatorManagerEventHandler, this));

  m_DrawBoxGizmo.SetOwner(GetWindow(), nullptr);
}
