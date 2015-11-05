#include <Core/PCH.h>
#include <Core/WorldSerializer/WorldWriter.h>


void ezWorldWriter::Write(ezStreamWriter& stream, ezWorld& world, const ezTagSet* pExclude)
{
  m_pStream = &stream;
  m_pWorld = &world;
  m_pExclude = pExclude;

  EZ_LOCK(m_pWorld->GetReadMarker());

  const ezUInt8 uiVersion = 1;
  stream << uiVersion;

  m_AllRootObjects.Clear();
  m_AllChildObjects.Clear();
  m_AllComponents.Clear();
  m_uiNumComponents = 0;

  // invalid handles
  {
    m_WrittenGameObjectHandles.Clear();
    m_WrittenComponentHandles.Clear();

    m_WrittenGameObjectHandles[ezGameObjectHandle()] = 0;
    m_WrittenComponentHandles[ezComponentHandle()] = 0;
  }

  m_pWorld->Traverse(ezMakeDelegate(&ezWorldWriter::ObjectTraverser, this), ezWorld::TraversalMethod::DepthFirst);

  stream << m_AllRootObjects.GetCount();
  stream << m_AllChildObjects.GetCount();
  stream << m_AllComponents.GetCount();
  stream << m_uiNumComponents;

  for (const auto* pObj : m_AllRootObjects)
  {
    WriteGameObject(pObj);
  }

  for (const auto* pObj : m_AllChildObjects)
  {
    WriteGameObject(pObj);
  }

  for (auto it = m_AllComponents.GetIterator(); it.IsValid(); ++it)
  {
    WriteComponentsOfType(it.Key(), it.Value());
  }
}

void ezWorldWriter::WriteHandle(const ezGameObjectHandle& hObject)
{
  auto it = m_WrittenGameObjectHandles.Find(hObject);

  ezUInt32 uiIndex = 0;

  if (it.IsValid())
    uiIndex = it.Value();

  *m_pStream << uiIndex;
}

void ezWorldWriter::WriteHandle(const ezComponentHandle& hComponent)
{
  auto it = m_WrittenComponentHandles.Find(hComponent);

  if (it.IsValid())
    *m_pStream << it.Value();
  else
  {
    const ezUInt32 uiInvalid = 0;
    *m_pStream << uiInvalid;
  }
}

bool ezWorldWriter::ObjectTraverser(ezGameObject* pObject)
{
  if (m_pExclude && pObject->GetTags().IsAnySet(*m_pExclude))
    return true;

  if (pObject->GetParent())
    m_AllChildObjects.PushBack(pObject);
  else
    m_AllRootObjects.PushBack(pObject);

  const ezUInt32 uiObjectIdx = m_WrittenGameObjectHandles.GetCount();
  m_WrittenGameObjectHandles[pObject->GetHandle()] = uiObjectIdx;

  auto components = pObject->GetComponents();

  for (const auto* pComp : components)
  {
    m_AllComponents[pComp->GetDynamicRTTI()].PushBack(pComp);
    const ezUInt32 uiComponentIdx = m_WrittenComponentHandles.GetCount();
    m_WrittenComponentHandles[pComp->GetHandle()] = uiComponentIdx;
  }

  m_uiNumComponents += components.GetCount();

  return true;
}

void ezWorldWriter::WriteGameObject(const ezGameObject* pObject)
{
  if (pObject->GetParent())
    WriteHandle(pObject->GetParent()->GetHandle());
  else
    WriteHandle(ezGameObjectHandle());

  ezStreamWriter& s = *m_pStream;

  s << pObject->GetName();
  s << pObject->GetLocalPosition();
  s << pObject->GetLocalRotation();
  s << pObject->GetLocalScaling();
  s << pObject->IsActive();
  s << pObject->IsDynamic();

  /// \todo
  // tags
  // write strings only once
}

void ezWorldWriter::WriteComponentsOfType(const ezRTTI* pRtti, const ezDeque<const ezComponent*>& components)
{
  ezStreamWriter& s = *m_pStream;

  s << pRtti->GetTypeName();
  s << pRtti->GetTypeVersion();
  s << components.GetCount();

  for (const auto* pComp : components)
  {
    /// \todo redirect to memory stream, record data size

    WriteHandle(pComp->GetOwner()->GetHandle());

    s << pComp->IsActive();
    s << pComp->IsDynamic();

    pComp->SerializeComponent(*this);
  }

}



EZ_STATICLINK_FILE(Core, Core_WorldSerializer_Implementation_WorldWriter);

