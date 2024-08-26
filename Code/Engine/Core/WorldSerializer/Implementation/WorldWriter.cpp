#include <Core/CorePCH.h>

#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/StringDeduplicationContext.h>

void ezWorldWriter::Clear()
{
  m_AllRootObjects.Clear();
  m_AllChildObjects.Clear();
  m_AllComponents.Clear();

  m_pStream = nullptr;
  m_pExclude = nullptr;

  // invalid handles
  {
    m_WrittenGameObjectHandles.Clear();
    m_WrittenGameObjectHandles[ezGameObjectHandle()] = 0;
  }
}

void ezWorldWriter::WriteWorld(ezStreamWriter& inout_stream, ezWorld& ref_world, const ezTagSet* pExclude)
{
  Clear();

  m_pStream = &inout_stream;
  m_pExclude = pExclude;

  EZ_LOCK(ref_world.GetReadMarker());

  ref_world.Traverse(ezMakeDelegate(&ezWorldWriter::ObjectTraverser, this), ezWorld::TraversalMethod::DepthFirst);

  WriteToStream().IgnoreResult();
}

void ezWorldWriter::WriteObjects(ezStreamWriter& inout_stream, const ezDeque<const ezGameObject*>& rootObjects)
{
  Clear();

  m_pStream = &inout_stream;

  for (const ezGameObject* pObject : rootObjects)
  {
    // traversal function takes a non-const object, but we only read it anyway
    Traverse(const_cast<ezGameObject*>(pObject));
  }

  WriteToStream().IgnoreResult();
}

void ezWorldWriter::WriteObjects(ezStreamWriter& inout_stream, ezArrayPtr<const ezGameObject*> rootObjects)
{
  Clear();

  m_pStream = &inout_stream;

  for (const ezGameObject* pObject : rootObjects)
  {
    // traversal function takes a non-const object, but we only read it anyway
    Traverse(const_cast<ezGameObject*>(pObject));
  }

  WriteToStream().IgnoreResult();
}

ezResult ezWorldWriter::WriteToStream()
{
  const ezUInt8 uiVersion = 10;
  *m_pStream << uiVersion;

  // version 8: use string dedup instead of handle writer
  ezStringDeduplicationWriteContext stringDedupWriteContext(*m_pStream);
  m_pStream = &stringDedupWriteContext.Begin();

  IncludeAllComponentBaseTypes();

  ezUInt32 uiNumRootObjects = m_AllRootObjects.GetCount();
  ezUInt32 uiNumChildObjects = m_AllChildObjects.GetCount();
  ezUInt32 uiNumComponentTypes = m_AllComponents.GetCount();

  *m_pStream << uiNumRootObjects;
  *m_pStream << uiNumChildObjects;
  *m_pStream << uiNumComponentTypes;

  // this is used to sort all component types by name, to make the file serialization deterministic
  ezMap<ezString, const ezRTTI*> sortedTypes;

  for (auto it = m_AllComponents.GetIterator(); it.IsValid(); ++it)
  {
    sortedTypes[it.Key()->GetTypeName()] = it.Key();
  }

  AssignGameObjectIndices();
  AssignComponentHandleIndices(sortedTypes);

  for (const auto* pObject : m_AllRootObjects)
  {
    WriteGameObject(pObject);
  }

  for (const auto* pObject : m_AllChildObjects)
  {
    WriteGameObject(pObject);
  }

  for (auto it = sortedTypes.GetIterator(); it.IsValid(); ++it)
  {
    WriteComponentTypeInfo(it.Value());
  }

  for (auto it = sortedTypes.GetIterator(); it.IsValid(); ++it)
  {
    WriteComponentCreationData(m_AllComponents[it.Value()].m_Components);
  }

  for (auto it = sortedTypes.GetIterator(); it.IsValid(); ++it)
  {
    WriteComponentSerializationData(m_AllComponents[it.Value()].m_Components);
  }

  EZ_SUCCEED_OR_RETURN(stringDedupWriteContext.End());
  m_pStream = &stringDedupWriteContext.GetOriginalStream();

  return EZ_SUCCESS;
}


void ezWorldWriter::AssignGameObjectIndices()
{
  ezUInt32 uiGameObjectIndex = 1;
  for (const auto* pObject : m_AllRootObjects)
  {
    m_WrittenGameObjectHandles[pObject->GetHandle()] = uiGameObjectIndex;
    ++uiGameObjectIndex;
  }

  for (const auto* pObject : m_AllChildObjects)
  {
    m_WrittenGameObjectHandles[pObject->GetHandle()] = uiGameObjectIndex;
    ++uiGameObjectIndex;
  }
}

void ezWorldWriter::AssignComponentHandleIndices(const ezMap<ezString, const ezRTTI*>& sortedTypes)
{
  ezUInt16 uiTypeIndex = 0;

  EZ_ASSERT_DEV(m_AllComponents.GetCount() <= ezMath::MaxValue<ezUInt16>(), "Too many types for world writer");

  // assign the component handle indices in the order in which the components are written
  for (auto it = sortedTypes.GetIterator(); it.IsValid(); ++it)
  {
    auto& components = m_AllComponents[it.Value()];

    components.m_uiSerializedTypeIndex = uiTypeIndex;
    ++uiTypeIndex;

    ezUInt32 uiComponentIndex = 1;
    components.m_HandleToIndex[ezComponentHandle()] = 0;

    for (const ezComponent* pComp : components.m_Components)
    {
      components.m_HandleToIndex[pComp->GetHandle()] = uiComponentIndex;
      ++uiComponentIndex;
    }
  }
}


void ezWorldWriter::IncludeAllComponentBaseTypes()
{
  ezDynamicArray<const ezRTTI*> allNow;
  allNow.Reserve(m_AllComponents.GetCount());
  for (auto it = m_AllComponents.GetIterator(); it.IsValid(); ++it)
  {
    allNow.PushBack(it.Key());
  }

  for (auto pRtti : allNow)
  {
    IncludeAllComponentBaseTypes(pRtti->GetParentType());
  }
}


void ezWorldWriter::IncludeAllComponentBaseTypes(const ezRTTI* pRtti)
{
  if (pRtti == nullptr || !pRtti->IsDerivedFrom<ezComponent>() || m_AllComponents.Contains(pRtti))
    return;

  // this is actually used to insert the type, but we have no component of this type
  m_AllComponents[pRtti];

  IncludeAllComponentBaseTypes(pRtti->GetParentType());
}


void ezWorldWriter::Traverse(ezGameObject* pObject)
{
  if (ObjectTraverser(pObject) == ezVisitorExecution::Continue)
  {
    for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
    {
      Traverse(&(*it));
    }
  }
}

void ezWorldWriter::WriteGameObjectHandle(const ezGameObjectHandle& hObject)
{
  auto it = m_WrittenGameObjectHandles.Find(hObject);

  ezUInt32 uiIndex = 0;

  EZ_ASSERT_DEV(it.IsValid(), "Referenced object does not exist in the scene. This can happen, if it was optimized away, because it had no name, no children and no essential components.");

  if (it.IsValid())
    uiIndex = it.Value();

  *m_pStream << uiIndex;
}

void ezWorldWriter::WriteComponentHandle(const ezComponentHandle& hComponent)
{
  ezUInt16 uiTypeIndex = 0;
  ezUInt32 uiIndex = 0;

  ezComponent* pComponent = nullptr;
  if (ezWorld::GetWorld(hComponent)->TryGetComponent(hComponent, pComponent))
  {
    if (auto* components = m_AllComponents.GetValue(pComponent->GetDynamicRTTI()))
    {
      auto it = components->m_HandleToIndex.Find(hComponent);
      EZ_ASSERT_DEBUG(it.IsValid(), "Handle should always be in the written map at this point");

      if (it.IsValid())
      {
        uiTypeIndex = components->m_uiSerializedTypeIndex;
        uiIndex = it.Value();
      }
    }
  }

  *m_pStream << uiTypeIndex;
  *m_pStream << uiIndex;
}

ezVisitorExecution::Enum ezWorldWriter::ObjectTraverser(ezGameObject* pObject)
{
  if (m_pExclude && pObject->GetTags().IsAnySet(*m_pExclude))
    return ezVisitorExecution::Skip;
  if (pObject->WasCreatedByPrefab())
    return ezVisitorExecution::Skip;

  if (pObject->GetParent())
    m_AllChildObjects.PushBack(pObject);
  else
    m_AllRootObjects.PushBack(pObject);

  auto components = pObject->GetComponents();

  for (const ezComponent* pComp : components)
  {
    if (pComp->WasCreatedByPrefab())
      continue;

    m_AllComponents[pComp->GetDynamicRTTI()].m_Components.PushBack(pComp);
  }

  return ezVisitorExecution::Continue;
}

void ezWorldWriter::WriteGameObject(const ezGameObject* pObject)
{
  if (pObject->GetParent())
    WriteGameObjectHandle(pObject->GetParent()->GetHandle());
  else
    WriteGameObjectHandle(ezGameObjectHandle());

  ezStreamWriter& s = *m_pStream;

  s << pObject->GetName();
  s << pObject->GetGlobalKey();
  s << pObject->GetLocalPosition();
  s << pObject->GetLocalRotation();
  s << pObject->GetLocalScaling();
  s << pObject->GetLocalUniformScaling();
  s << pObject->GetActiveFlag();
  s << pObject->IsDynamic();
  pObject->GetTags().Save(s);
  s << pObject->GetTeamID();
  s << pObject->GetStableRandomSeed();
}

void ezWorldWriter::WriteComponentTypeInfo(const ezRTTI* pRtti)
{
  ezStreamWriter& s = *m_pStream;

  s << pRtti->GetTypeName();
  s << pRtti->GetTypeVersion();
}

void ezWorldWriter::WriteComponentCreationData(const ezDeque<const ezComponent*>& components)
{
  ezDefaultMemoryStreamStorage storage;
  ezMemoryStreamWriter memWriter(&storage);

  ezStreamWriter* pPrevStream = m_pStream;
  m_pStream = &memWriter;

  // write to memory stream
  {
    ezStreamWriter& s = *m_pStream;
    s << components.GetCount();

    ezUInt32 uiComponentIndex = 1;
    for (auto pComponent : components)
    {
      WriteGameObjectHandle(pComponent->GetOwner()->GetHandle());
      s << uiComponentIndex;
      ++uiComponentIndex;

      s << pComponent->GetActiveFlag();

      // version 7
      {
        ezUInt8 userFlags = 0;
        for (ezUInt8 i = 0; i < 8; ++i)
        {
          userFlags |= pComponent->GetUserFlag(i) ? EZ_BIT(i) : 0;
        }

        s << userFlags;
      }
    }
  }

  m_pStream = pPrevStream;

  // write result to actual stream
  {
    ezStreamWriter& s = *m_pStream;
    s << storage.GetStorageSize32();

    EZ_ASSERT_ALWAYS(storage.GetStorageSize64() <= ezMath::MaxValue<ezUInt32>(), "Slight file format change and version increase needed to support > 4GB worlds.");

    storage.CopyToStream(s).IgnoreResult();
  }
}

void ezWorldWriter::WriteComponentSerializationData(const ezDeque<const ezComponent*>& components)
{
  ezDefaultMemoryStreamStorage storage;
  ezMemoryStreamWriter memWriter(&storage);

  ezStreamWriter* pPrevStream = m_pStream;
  m_pStream = &memWriter;

  // write to memory stream
  for (auto pComp : components)
  {
    pComp->SerializeComponent(*this);
  }

  m_pStream = pPrevStream;

  // write result to actual stream
  {
    ezStreamWriter& s = *m_pStream;
    s << storage.GetStorageSize32();

    EZ_ASSERT_ALWAYS(storage.GetStorageSize64() <= ezMath::MaxValue<ezUInt32>(), "Slight file format change and version increase needed to support > 4GB worlds.");

    storage.CopyToStream(s).IgnoreResult();
  }
}
