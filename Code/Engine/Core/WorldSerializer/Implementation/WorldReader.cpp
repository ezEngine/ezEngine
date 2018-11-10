#include <PCH.h>

#include <Core/WorldSerializer/WorldReader.h>

ezWorldReader::ezWorldReader()
{
  m_pStream = nullptr;
  m_pWorld = nullptr;
  m_uiMaxComponents = 0;
  m_uiVersion = 0;
}

void ezWorldReader::ReadWorldDescription(ezStreamReader& stream)
{
  m_HandleReadContext.Reset();

  m_pStream = &stream;

  m_uiVersion = 0;
  stream >> m_uiVersion;

  EZ_ASSERT_DEV(m_uiVersion <= 7, "Invalid version {0}", m_uiVersion);

  if (m_uiVersion >= 3)
  {
    // add tags from the stream
    ezTagRegistry::GetGlobalRegistry().Load(stream);
  }

  ezUInt32 uiNumRootObjects = 0;
  stream >> uiNumRootObjects;

  ezUInt32 uiNumChildObjects = 0;
  stream >> uiNumChildObjects;

  ezUInt32 uiNumComponentTypes = 0;
  stream >> uiNumComponentTypes;

  stream >> m_uiMaxComponents;

  m_RootObjectsToCreate.Reserve(uiNumRootObjects);
  m_ChildObjectsToCreate.Reserve(uiNumChildObjects);

  m_IndexToGameObjectHandle.Reserve(uiNumRootObjects + uiNumChildObjects + 1);
  m_IndexToComponentHandle.Reserve(m_uiMaxComponents + 1);

  for (ezUInt32 i = 0; i < uiNumRootObjects; ++i)
  {
    ReadGameObjectDesc(m_RootObjectsToCreate.ExpandAndGetRef());
  }

  for (ezUInt32 i = 0; i < uiNumChildObjects; ++i)
  {
    ReadGameObjectDesc(m_ChildObjectsToCreate.ExpandAndGetRef());
  }

  m_ComponentTypes.SetCount(uiNumComponentTypes);
  m_ComponentTypeVersions.Reserve(uiNumComponentTypes);
  for (ezUInt32 i = 0; i < uiNumComponentTypes; ++i)
  {
    ReadComponentInfo(i);
  }

  // read all component data
  {
    m_HandleReadContext.BeginReadingFromStream(m_pStream);

    ezMemoryStreamWriter memWriter(&m_ComponentStream);

    ezUInt8 Temp[4096];

    for (ezUInt32 i = 0; i < m_ComponentTypes.GetCount(); ++i)
    {
      ezUInt32 uiAllComponentsSize = 0;
      stream >> uiAllComponentsSize;

      memWriter << uiAllComponentsSize;

      while (uiAllComponentsSize > 0)
      {
        const ezUInt64 uiRead = stream.ReadBytes(Temp, ezMath::Min<ezUInt32>(uiAllComponentsSize, EZ_ARRAY_SIZE(Temp)));

        memWriter.WriteBytes(Temp, uiRead);

        uiAllComponentsSize -= (ezUInt32)uiRead;
      }
    }

    m_HandleReadContext.EndReadingFromStream(&stream);
  }
}

void ezWorldReader::InstantiateWorld(ezWorld& world, const ezUInt16* pOverrideTeamID)
{
  Instantiate(world, false, ezTransform(), ezGameObjectHandle(), nullptr, nullptr, pOverrideTeamID);
}

void ezWorldReader::InstantiatePrefab(ezWorld& world, const ezTransform& rootTransform, ezGameObjectHandle hParent,
                                      ezHybridArray<ezGameObject*, 8>* out_CreatedRootObjects,
                                      ezHybridArray<ezGameObject*, 8>* out_CreatedChildObjects, const ezUInt16* pOverrideTeamID)
{
  Instantiate(world, true, rootTransform, hParent, out_CreatedRootObjects, out_CreatedChildObjects, pOverrideTeamID);
}

void ezWorldReader::Instantiate(ezWorld& world, bool bUseTransform, const ezTransform& rootTransform, ezGameObjectHandle hParent,
                                ezHybridArray<ezGameObject*, 8>* out_CreatedRootObjects,
                                ezHybridArray<ezGameObject*, 8>* out_CreatedChildObjects, const ezUInt16* pOverrideTeamID)
{
  m_pWorld = &world;

  m_IndexToGameObjectHandle.Clear();
  m_IndexToComponentHandle.Clear();

  m_IndexToGameObjectHandle.PushBack(ezGameObjectHandle());
  m_IndexToComponentHandle.SetCount(m_uiMaxComponents + 1); // initialize with 'invalid' handles to be able to skip unknown components

  EZ_LOCK(m_pWorld->GetWriteMarker());

  if (bUseTransform)
  {
    CreateGameObjects(m_RootObjectsToCreate, rootTransform, hParent, out_CreatedRootObjects, pOverrideTeamID);
  }
  else
  {
    CreateGameObjects(m_RootObjectsToCreate, hParent, out_CreatedRootObjects, pOverrideTeamID);
  }

  CreateGameObjects(m_ChildObjectsToCreate, ezGameObjectHandle(), out_CreatedChildObjects, pOverrideTeamID);

  // read component data from copied memory stream
  if (m_ComponentStream.GetStorageSize() > 0)
  {
    ezMemoryStreamReader memReader(&m_ComponentStream);
    ezStreamReader* pPrevReader = m_pStream;
    m_pStream = &memReader;

    m_HandleReadContext.BeginRestoringHandles(m_pStream);

    for (ezUInt32 i = 0; i < m_ComponentTypes.GetCount(); ++i)
    {
      ReadComponentsOfType(i);
    }

    m_HandleReadContext.EndRestoringHandles();

    m_pStream = pPrevReader;
  }

  FulfillComponentHandleRequets();
}


ezGameObjectHandle ezWorldReader::ReadGameObjectHandle()
{
  ezUInt32 idx = 0;
  *m_pStream >> idx;

  return m_IndexToGameObjectHandle[idx];
}

void ezWorldReader::ReadComponentHandle(ezComponentHandle* out_hComponent)
{
  ezUInt32 idx = 0;
  *m_pStream >> idx;

  CompRequest r;
  r.m_pWriteToComponent = out_hComponent;
  r.m_uiComponentIndex = idx;

  m_ComponentHandleRequests.PushBack(r);
}


ezUInt32 ezWorldReader::GetComponentTypeVersion(const ezRTTI* pRtti) const
{
  ezUInt32 uiVersion = 0xFFFFFFFF;
  m_ComponentTypeVersions.TryGetValue(pRtti, uiVersion);

  return uiVersion;
}


void ezWorldReader::ClearAndCompact()
{
  m_IndexToGameObjectHandle.Clear();
  m_IndexToGameObjectHandle.Compact();

  m_IndexToComponentHandle.Clear();
  m_IndexToComponentHandle.Compact();

  m_RootObjectsToCreate.Clear();
  m_RootObjectsToCreate.Compact();

  m_ChildObjectsToCreate.Clear();
  m_ChildObjectsToCreate.Compact();

  m_ComponentHandleRequests.Clear();
  m_ComponentHandleRequests.Compact();

  m_ComponentTypes.Clear();
  m_ComponentTypes.Compact();

  m_ComponentTypeVersions.Clear();
  m_ComponentTypeVersions.Compact();

  m_ComponentStream.Clear();
  m_ComponentStream.Compact();
}


ezUInt64 ezWorldReader::GetHeapMemoryUsage() const
{
  return m_IndexToGameObjectHandle.GetHeapMemoryUsage() + m_IndexToComponentHandle.GetHeapMemoryUsage() +
         m_RootObjectsToCreate.GetHeapMemoryUsage() + m_ChildObjectsToCreate.GetHeapMemoryUsage() +
         m_ComponentHandleRequests.GetHeapMemoryUsage() + m_ComponentTypes.GetHeapMemoryUsage() +
         m_ComponentTypeVersions.GetHeapMemoryUsage() + m_ComponentStream.GetHeapMemoryUsage();
}

void ezWorldReader::ReadGameObjectDesc(GameObjectToCreate& godesc)
{
  ezGameObjectDesc& desc = godesc.m_Desc;
  ezStringBuilder sName, sGlobalKey;

  *m_pStream >> godesc.m_uiParentHandleIdx;
  *m_pStream >> sName;

  if (m_uiVersion >= 4)
  {
    *m_pStream >> sGlobalKey;
    godesc.m_sGlobalKey = sGlobalKey;
  }

  *m_pStream >> desc.m_LocalPosition;
  *m_pStream >> desc.m_LocalRotation;
  *m_pStream >> desc.m_LocalScaling;
  *m_pStream >> desc.m_LocalUniformScaling;

  *m_pStream >> desc.m_bActive;
  *m_pStream >> desc.m_bDynamic;

  if (m_uiVersion >= 3)
    desc.m_Tags.Load(*m_pStream, ezTagRegistry::GetGlobalRegistry());

  if (m_uiVersion >= 6)
  {
    *m_pStream >> desc.m_uiTeamID;
  }

  desc.m_sName.Assign(sName.GetData());
}


void ezWorldReader::ReadComponentInfo(ezUInt32 uiComponentTypeIdx)
{
  ezStreamReader& s = *m_pStream;

  ezStringBuilder sRttiName;
  ezUInt32 uiRttiVersion = 0;

  s >> sRttiName;
  s >> uiRttiVersion;

  const ezRTTI* pRtti = ezRTTI::FindTypeByName(sRttiName);

  if (pRtti == nullptr)
  {
    ezLog::Error("Unknown component type '{0}'. Components of this type will be skipped.", sRttiName);
  }

  m_ComponentTypes[uiComponentTypeIdx] = pRtti;
  m_ComponentTypeVersions[pRtti] = uiRttiVersion;
}

void ezWorldReader::ReadComponentsOfType(ezUInt32 uiComponentTypeIdx)
{
  ezStreamReader& s = *m_pStream;

  ezUInt32 uiAllComponentsSize = 0;
  s >> uiAllComponentsSize;

  bool bSkip = false;

  ezComponentManagerBase* pManager = nullptr;
  const ezRTTI* pRtti = m_ComponentTypes[uiComponentTypeIdx];

  if (pRtti == nullptr)
  {
    bSkip = true;
    ezLog::Warning("Skipping components of unknown type");
  }
  else
  {
    pManager = m_pWorld->GetOrCreateComponentManager(pRtti);
  }

  if (bSkip)
  {
    m_pStream->SkipBytes(uiAllComponentsSize);
  }
  else
  {
    ezUInt32 uiNumComponents = 0;
    s >> uiNumComponents;

    // will be the case for all abstract component types
    if (uiNumComponents == 0)
      return;

    // only check this after we know that we actually need to create any of this type
    EZ_ASSERT_DEV(pManager != nullptr, "Cannot create components of type '{0}', manager is not available.", pRtti->GetTypeName());

    for (ezUInt32 i = 0; i < uiNumComponents; ++i)
    {
      const ezGameObjectHandle hOwner = ReadGameObjectHandle();

      ezUInt32 uiComponentIdx = 0;
      *m_pStream >> uiComponentIdx;

      bool bActive = true;
      *m_pStream >> bActive;

      if (m_uiVersion <= 4)
      {
        bool bDynamic = true;
        *m_pStream >> bDynamic;
      }

      ezUInt8 userFlags = 0;
      if (m_uiVersion >= 7)
      {
        *m_pStream >> userFlags;
      }

      ezGameObject* pParentObject = nullptr;
      m_pWorld->TryGetObject(hOwner, pParentObject);

      ezComponent* pComponent = nullptr;
      auto hComponent = pManager->CreateComponent(pParentObject, pComponent);
      m_IndexToComponentHandle[uiComponentIdx] = hComponent;

      pComponent->SetActive(bActive);

      for (ezUInt32 i = 0; i < 8; ++i)
      {
        pComponent->SetUserFlag(i, (userFlags & EZ_BIT(i)) != 0);
      }

      pComponent->DeserializeComponent(*this);
    }
  }
}

void ezWorldReader::FulfillComponentHandleRequets()
{
  for (const auto& req : m_ComponentHandleRequests)
  {
    *req.m_pWriteToComponent = m_IndexToComponentHandle[req.m_uiComponentIndex];
  }

  m_ComponentHandleRequests.Clear();
}

void ezWorldReader::CreateGameObjects(const ezDynamicArray<GameObjectToCreate>& objects, ezGameObjectHandle hParent,
                                      ezHybridArray<ezGameObject*, 8>* out_CreatedObjects, const ezUInt16* pOverrideTeamID)
{
  if (hParent.IsInvalidated())
  {
    for (const auto& godesc : objects)
    {
      ezGameObjectDesc desc = godesc.m_Desc; // make a copy
      desc.m_hParent = m_IndexToGameObjectHandle[godesc.m_uiParentHandleIdx];

      if (pOverrideTeamID != nullptr)
        desc.m_uiTeamID = *pOverrideTeamID;

      ezGameObject* pObject;
      m_IndexToGameObjectHandle.PushBack(m_pWorld->CreateObject(desc, pObject));

      if (!godesc.m_sGlobalKey.IsEmpty())
      {
        pObject->SetGlobalKey(godesc.m_sGlobalKey);
      }

      if (out_CreatedObjects)
        out_CreatedObjects->PushBack(pObject);
    }
  }
  else
  {
    for (const auto& godesc : objects)
    {
      ezGameObjectDesc desc = godesc.m_Desc; // make a copy
      desc.m_hParent = hParent;

      ezGameObject* pObject;
      m_IndexToGameObjectHandle.PushBack(m_pWorld->CreateObject(desc, pObject));

      if (!godesc.m_sGlobalKey.IsEmpty())
      {
        pObject->SetGlobalKey(godesc.m_sGlobalKey);
      }

      if (out_CreatedObjects)
        out_CreatedObjects->PushBack(pObject);
    }
  }
}


void ezWorldReader::CreateGameObjects(const ezDynamicArray<GameObjectToCreate>& objects, const ezTransform& rootTransform,
                                      ezGameObjectHandle hParent, ezHybridArray<ezGameObject*, 8>* out_CreatedRootObjects,
                                      const ezUInt16* pOverrideTeamID)
{
  for (const auto& godesc : objects)
  {
    ezGameObjectDesc desc = godesc.m_Desc; // make a copy
    desc.m_hParent = hParent;

    if (pOverrideTeamID != nullptr)
      desc.m_uiTeamID = *pOverrideTeamID;

    ezTransform tChild(desc.m_LocalPosition, desc.m_LocalRotation, desc.m_LocalScaling);
    ezTransform tFinal;
    tFinal.SetGlobalTransform(rootTransform, tChild);

    desc.m_LocalPosition = tFinal.m_vPosition;
    desc.m_LocalRotation = tFinal.m_qRotation;
    desc.m_LocalScaling = tFinal.m_vScale;

    ezGameObject* pObject;

    m_IndexToGameObjectHandle.PushBack(m_pWorld->CreateObject(desc, pObject));

    if (!godesc.m_sGlobalKey.IsEmpty())
    {
      pObject->SetGlobalKey(godesc.m_sGlobalKey);
    }

    if (out_CreatedRootObjects)
      out_CreatedRootObjects->PushBack(pObject);
  }
}

EZ_STATICLINK_FILE(Core, Core_WorldSerializer_Implementation_WorldReader);
