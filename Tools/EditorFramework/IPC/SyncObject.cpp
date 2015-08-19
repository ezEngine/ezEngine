#include <PCH.h>
#include <EditorFramework/IPC/SyncObject.h>
#include <ToolsFoundation/Document/Document.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorEngineSyncObject, ezReflectedClass, 1, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("SyncGuid", m_SyncObjectGuid)
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezEditorEngineSyncObject::ezEditorEngineSyncObject()
{
  m_SyncObjectGuid.CreateNewUuid();
  m_bModified = true;
  m_pOwner = nullptr;
  m_pOwnerEngine = nullptr;
}

ezEditorEngineSyncObject::~ezEditorEngineSyncObject()
{
  if (m_pOwner)
  {
    m_pOwner->RemoveSyncObject(this);
  }

  if (m_pOwnerEngine)
  {
    m_pOwnerEngine->RemoveSyncObject(this);
  }
}

void ezEditorEngineSyncObject::SetOwner(ezDocumentWindow3D* pOwner)
{ 
  EZ_ASSERT_DEV(pOwner != nullptr, "invalid owner");
  m_pOwner = pOwner;
  m_pOwner->AddSyncObject(this);
}


void ezEditorEngineSyncObject::SetOwner(ezEngineProcessDocumentContext* pOwner)
{
  EZ_ASSERT_DEV(pOwner != nullptr, "invalid owner");
  m_pOwnerEngine = pOwner;
  m_pOwnerEngine->AddSyncObject(this);
}

ezUuid ezEditorEngineSyncObject::GetDocumentGuid() const
{
  if (m_pOwner)
    return m_pOwner->GetDocument()->GetGuid();

  if (m_pOwnerEngine)
    return m_pOwnerEngine->GetDocumentGuid();

  EZ_REPORT_FAILURE("Owner not set");
  return ezUuid();
}
