#include <CorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/Progress.h>

ezWorldReader::FindComponentTypeCallback ezWorldReader::s_FindComponentTypeCallback;

ezWorldReader::ezWorldReader() = default;
ezWorldReader::~ezWorldReader() = default;

ezResult ezWorldReader::ReadWorldDescription(ezStreamReader& stream)
{
  m_pStream = &stream;

  m_uiVersion = 0;
  stream >> m_uiVersion;

  if (m_uiVersion < 8 || m_uiVersion > 9)
  {
    ezLog::Error("Invalid world version (got {}).", m_uiVersion);
    return EZ_FAILURE;
  }

  // destroy old context first
  m_pStringDedupReadContext = nullptr;
  m_pStringDedupReadContext = EZ_DEFAULT_NEW(ezStringDeduplicationReadContext, stream);

  if (m_uiVersion == 8)
  {
    // add tags from the stream
    EZ_SUCCEED_OR_RETURN(ezTagRegistry::GetGlobalRegistry().Load(stream));
  }

  ezUInt32 uiNumRootObjects = 0;
  stream >> uiNumRootObjects;

  ezUInt32 uiNumChildObjects = 0;
  stream >> uiNumChildObjects;

  ezUInt32 uiNumComponentTypes = 0;
  stream >> uiNumComponentTypes;

  if (uiNumComponentTypes > ezMath::MaxValue<ezUInt16>())
  {
    ezLog::Error("World description has too many component types, got {0} - maximum allowed are {1}", uiNumComponentTypes, ezMath::MaxValue<ezUInt16>());
    return EZ_FAILURE;
  }

  m_RootObjectsToCreate.Reserve(uiNumRootObjects);
  m_ChildObjectsToCreate.Reserve(uiNumChildObjects);

  m_IndexToGameObjectHandle.SetCountUninitialized(uiNumRootObjects + uiNumChildObjects + 1);

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
    ReadComponentTypeInfo(i);
  }

  // read all component data
  ReadComponentDataToMemStream();
  m_pStringDedupReadContext->SetActive(false);

  return EZ_SUCCESS;
}

ezUniquePtr<ezWorldReader::InstantiationContextBase> ezWorldReader::InstantiateWorld(ezWorld& world, const ezUInt16* pOverrideTeamID, ezTime maxStepTime,
  ezProgress* pProgress)
{
  return Instantiate(world, false, ezTransform(), ezGameObjectHandle(), nullptr, nullptr, pOverrideTeamID, false,
    maxStepTime, pProgress);
}

ezUniquePtr<ezWorldReader::InstantiationContextBase> ezWorldReader::InstantiatePrefab(ezWorld& world, const ezTransform& rootTransform, ezGameObjectHandle hParent,
  ezHybridArray<ezGameObject*, 8>* out_CreatedRootObjects, ezHybridArray<ezGameObject*, 8>* out_CreatedChildObjects,
  const ezUInt16* pOverrideTeamID, bool bForceDynamic, ezTime maxStepTime, ezProgress* pProgress)
{
  return Instantiate(world, true, rootTransform, hParent, out_CreatedRootObjects, out_CreatedChildObjects, pOverrideTeamID, bForceDynamic,
    maxStepTime, pProgress);
}

ezGameObjectHandle ezWorldReader::ReadGameObjectHandle()
{
  ezUInt32 idx = 0;
  *m_pStream >> idx;

  return m_IndexToGameObjectHandle[idx];
}

void ezWorldReader::ReadComponentHandle(ezComponentHandle& out_hComponent)
{
  ezUInt16 uiTypeIndex = 0;
  ezUInt32 uiIndex = 0;

  *m_pStream >> uiTypeIndex;
  *m_pStream >> uiIndex;

  out_hComponent.Invalidate();

  if (uiTypeIndex < m_ComponentTypes.GetCount())
  {
    auto& indexToHandle = m_ComponentTypes[uiTypeIndex].m_ComponentIndexToHandle;
    if (uiIndex < indexToHandle.GetCount())
    {
      out_hComponent = indexToHandle[uiIndex];
    }
  }
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

  m_RootObjectsToCreate.Clear();
  m_RootObjectsToCreate.Compact();

  m_ChildObjectsToCreate.Clear();
  m_ChildObjectsToCreate.Compact();

  m_ComponentTypes.Clear();
  m_ComponentTypes.Compact();

  m_ComponentTypeVersions.Clear();
  m_ComponentTypeVersions.Compact();

  m_ComponentCreationStream.Clear();
  m_ComponentCreationStream.Compact();

  m_ComponentDataStream.Clear();
  m_ComponentDataStream.Compact();
}

ezUInt64 ezWorldReader::GetHeapMemoryUsage() const
{
  return m_IndexToGameObjectHandle.GetHeapMemoryUsage() +
         m_RootObjectsToCreate.GetHeapMemoryUsage() + m_ChildObjectsToCreate.GetHeapMemoryUsage() +
         m_ComponentTypes.GetHeapMemoryUsage() + m_ComponentTypeVersions.GetHeapMemoryUsage() +
         m_ComponentCreationStream.GetHeapMemoryUsage() + m_ComponentDataStream.GetHeapMemoryUsage();
}

ezUInt32 ezWorldReader::GetRootObjectCount() const
{
  return m_RootObjectsToCreate.GetCount();
}


ezUInt32 ezWorldReader::GetChildObjectCount() const
{
  return m_ChildObjectsToCreate.GetCount();
}

void ezWorldReader::ReadGameObjectDesc(GameObjectToCreate& godesc)
{
  ezGameObjectDesc& desc = godesc.m_Desc;
  ezStringBuilder sName, sGlobalKey;

  *m_pStream >> godesc.m_uiParentHandleIdx;
  *m_pStream >> sName;

  *m_pStream >> sGlobalKey;
  godesc.m_sGlobalKey = sGlobalKey;

  *m_pStream >> desc.m_LocalPosition;
  *m_pStream >> desc.m_LocalRotation;
  *m_pStream >> desc.m_LocalScaling;
  *m_pStream >> desc.m_LocalUniformScaling;

  *m_pStream >> desc.m_bActiveFlag;
  *m_pStream >> desc.m_bDynamic;

  desc.m_Tags.Load(*m_pStream, ezTagRegistry::GetGlobalRegistry());

  *m_pStream >> desc.m_uiTeamID;

  desc.m_sName.Assign(sName.GetData());
}

void ezWorldReader::ReadComponentTypeInfo(ezUInt32 uiComponentTypeIdx)
{
  ezStreamReader& s = *m_pStream;

  ezStringBuilder sRttiName;
  ezUInt32 uiRttiVersion = 0;

  s >> sRttiName;
  s >> uiRttiVersion;

  const ezRTTI* pRtti = nullptr;

  if (s_FindComponentTypeCallback.IsValid())
  {
    pRtti = s_FindComponentTypeCallback(sRttiName);
  }
  else
  {
    pRtti = ezRTTI::FindTypeByName(sRttiName);

    if (pRtti == nullptr)
    {
      ezLog::Error("Unknown component type '{0}'. Components of this type will be skipped.", sRttiName);
    }
  }

  m_ComponentTypes[uiComponentTypeIdx].m_pRtti = pRtti;
  m_ComponentTypeVersions[pRtti] = uiRttiVersion;
}

void ezWorldReader::ReadComponentDataToMemStream()
{
  auto WriteToMemStream = [&](ezMemoryStreamWriter& writer, bool bReadNumComponents) {
    ezUInt8 Temp[4096];
    for (auto& compTypeInfo : m_ComponentTypes)
    {
      ezUInt32 uiAllComponentsSize = 0;
      *m_pStream >> uiAllComponentsSize;

      if (compTypeInfo.m_pRtti == nullptr)
      {
        ezLog::Warning("Skipping components of unknown type");

        m_pStream->SkipBytes(uiAllComponentsSize);
      }
      else
      {
        if (bReadNumComponents)
        {
          *m_pStream >> compTypeInfo.m_uiNumComponents;
          uiAllComponentsSize -= sizeof(ezUInt32);

          m_uiTotalNumComponents += compTypeInfo.m_uiNumComponents;
        }

        while (uiAllComponentsSize > 0)
        {
          const ezUInt64 uiRead = m_pStream->ReadBytes(Temp, ezMath::Min<ezUInt32>(uiAllComponentsSize, EZ_ARRAY_SIZE(Temp)));

          writer.WriteBytes(Temp, uiRead);

          uiAllComponentsSize -= (ezUInt32)uiRead;
        }
      }
    }
  };

  {
    ezMemoryStreamWriter writer(&m_ComponentCreationStream);
    WriteToMemStream(writer, true);
  }

  {
    ezMemoryStreamWriter writer(&m_ComponentDataStream);
    WriteToMemStream(writer, false);
  }
}

void ezWorldReader::ClearHandles()
{
  m_IndexToGameObjectHandle.Clear();
  m_IndexToGameObjectHandle.PushBack(ezGameObjectHandle());

  for (auto& compTypeInfo : m_ComponentTypes)
  {
    compTypeInfo.m_ComponentIndexToHandle.Clear();
    compTypeInfo.m_ComponentIndexToHandle.PushBack(ezComponentHandle());
  }
}

ezUniquePtr<ezWorldReader::InstantiationContextBase> ezWorldReader::Instantiate(ezWorld& world, bool bUseTransform,
  const ezTransform& rootTransform, ezGameObjectHandle hParent,
  ezHybridArray<ezGameObject*, 8>* out_CreatedRootObjects, ezHybridArray<ezGameObject*, 8>* out_CreatedChildObjects,
  const ezUInt16* pOverrideTeamID, bool bForceDynamic, ezTime maxStepTime, ezProgress* pProgress)
{
  m_pWorld = &world;

  ClearHandles();

  if (maxStepTime <= ezTime::Zero())
  {
    InstantiationContext context = InstantiationContext(*this, bUseTransform, rootTransform, hParent, out_CreatedRootObjects, out_CreatedChildObjects,
      pOverrideTeamID, bForceDynamic, maxStepTime, pProgress);

    EZ_VERIFY(context.Step(), "Instantiation should be completed after this call");
    return nullptr;
  }

  ezUniquePtr<InstantiationContext> pContext = EZ_DEFAULT_NEW(InstantiationContext, *this, bUseTransform, rootTransform,
    hParent, out_CreatedRootObjects, out_CreatedChildObjects, pOverrideTeamID, bForceDynamic, maxStepTime, pProgress);

  return std::move(pContext);
}

ezWorldReader::InstantiationContext::InstantiationContext(ezWorldReader& worldReader, bool bUseTransform, const ezTransform& rootTransform,
  ezGameObjectHandle hParent, ezHybridArray<ezGameObject*, 8>* out_CreatedRootObjects, ezHybridArray<ezGameObject*, 8>* out_CreatedChildObjects,
  const ezUInt16* pOverrideTeamID, bool bForceDynamic, ezTime maxStepTime, ezProgress* pProgress)
  : m_WorldReader(worldReader)
  , m_bUseTransform(bUseTransform)
  , m_RootTransform(rootTransform)
  , m_hParent(hParent)
  , m_pCreatedRootObjects(out_CreatedRootObjects)
  , m_pCreatedChildObjects(out_CreatedChildObjects)
  , m_pOverrideTeamID(pOverrideTeamID)
  , m_bForceDynamic(bForceDynamic)
  , m_MaxStepTime(maxStepTime.IsPositive() ? maxStepTime : ezTime::Hours(10000))
{
  m_Phase = Phase::CreateRootObjects;

  if (maxStepTime.IsPositive())
  {
    m_hComponentInitBatch = worldReader.m_pWorld->CreateComponentInitBatch("WorldReaderBatch", maxStepTime.IsPositive() ? false : true);
  }

  if (pProgress != nullptr)
  {
    m_pOverallProgressRange = EZ_DEFAULT_NEW(ezProgressRange, "Instantiate", Phase::Count, false, pProgress);
    m_pOverallProgressRange->SetStepWeighting(Phase::CreateRootObjects, m_WorldReader.m_RootObjectsToCreate.GetCount() / 100.0f);
    m_pOverallProgressRange->SetStepWeighting(Phase::CreateChildObjects, m_WorldReader.m_ChildObjectsToCreate.GetCount() / 100.0f);
    m_pOverallProgressRange->SetStepWeighting(Phase::CreateComponents, m_WorldReader.m_uiTotalNumComponents / 100.0f);
    m_pOverallProgressRange->SetStepWeighting(Phase::DeserializeComponents, m_WorldReader.m_uiTotalNumComponents / 100.0f);
    // Ten times more weight since init components takes way longer than the rest
    m_pOverallProgressRange->SetStepWeighting(Phase::InitComponents, m_WorldReader.m_uiTotalNumComponents / 10.0f);

    m_pOverallProgressRange->BeginNextStep("CreateRootObjects");
  }
}

ezWorldReader::InstantiationContext::~InstantiationContext()
{
  if (!m_hComponentInitBatch.IsInvalidated())
  {
    m_WorldReader.m_pWorld->DeleteComponentInitBatch(m_hComponentInitBatch);
    m_hComponentInitBatch.Invalidate();
  }
}

bool ezWorldReader::InstantiationContext::Step()
{
  EZ_ASSERT_DEV(m_Phase != Phase::Invalid, "InstantiationContext cannot be re-used.");

  EZ_PROFILE_SCOPE("ezWorldReader::InstContext::Step");

  EZ_LOCK(m_WorldReader.m_pWorld->GetWriteMarker());

  ezTime endTime = ezTime::Now() + m_MaxStepTime;

  if (m_Phase == Phase::CreateRootObjects)
  {
    if (m_bUseTransform)
    {
      if (!CreateGameObjects<true>(m_WorldReader.m_RootObjectsToCreate, m_hParent, m_pCreatedRootObjects, endTime))
        return false;
    }
    else
    {
      if (!CreateGameObjects<false>(m_WorldReader.m_RootObjectsToCreate, m_hParent, m_pCreatedRootObjects, endTime))
        return false;
    }

    m_Phase = Phase::CreateChildObjects;
    BeginNextProgressStep("CreateChildObjects");
  }

  if (m_Phase == Phase::CreateChildObjects)
  {
    if (!CreateGameObjects<false>(m_WorldReader.m_ChildObjectsToCreate, ezGameObjectHandle(), m_pCreatedChildObjects, endTime))
      return false;

    m_CurrentReader.SetStorage(&m_WorldReader.m_ComponentCreationStream);
    m_Phase = Phase::CreateComponents;
    BeginNextProgressStep("CreateComponents");
  }

  if (m_Phase == Phase::CreateComponents)
  {
    if (m_WorldReader.m_ComponentCreationStream.GetStorageSize() > 0)
    {
      m_WorldReader.m_pStringDedupReadContext->SetActive(true);

      ezStreamReader* pPrevReader = m_WorldReader.m_pStream;
      m_WorldReader.m_pStream = &m_CurrentReader;

      EZ_SCOPE_EXIT(m_WorldReader.m_pStream = pPrevReader; m_WorldReader.m_pStringDedupReadContext->SetActive(false););

      if (!CreateComponents(endTime))
        return false;
    }

    m_CurrentReader.SetStorage(&m_WorldReader.m_ComponentDataStream);
    m_Phase = Phase::DeserializeComponents;
    BeginNextProgressStep("DeserializeComponents");
  }

  if (m_Phase == Phase::DeserializeComponents)
  {
    if (m_WorldReader.m_ComponentDataStream.GetStorageSize() > 0)
    {
      m_WorldReader.m_pStringDedupReadContext->SetActive(true);

      ezStreamReader* pPrevReader = m_WorldReader.m_pStream;
      m_WorldReader.m_pStream = &m_CurrentReader;

      EZ_SCOPE_EXIT(m_WorldReader.m_pStream = pPrevReader; m_WorldReader.m_pStringDedupReadContext->SetActive(false););

      if (!DeserializeComponents(endTime))
        return false;
    }

    m_CurrentReader.SetStorage(nullptr);
    m_Phase = Phase::AddComponentsToBatch;
    BeginNextProgressStep("AddComponentsToBatch");
  }

  if (m_Phase == Phase::AddComponentsToBatch)
  {
    if (!AddComponentsToBatch(endTime))
      return false;

    m_Phase = Phase::InitComponents;
    BeginNextProgressStep("InitComponents");
  }

  if (m_Phase == Phase::InitComponents)
  {
    if (!m_hComponentInitBatch.IsInvalidated())
    {
      double fCompletionFactor = 0.0;
      if (!m_WorldReader.m_pWorld->IsComponentInitBatchCompleted(m_hComponentInitBatch, &fCompletionFactor))
      {
        SetSubProgressCompletion(fCompletionFactor);
        return false;
      }
    }

    m_Phase = Phase::Invalid;
    m_pSubProgressRange = nullptr;
    m_pOverallProgressRange = nullptr;
  }

  return true;
}

void ezWorldReader::InstantiationContext::Cancel()
{
  if (!m_hComponentInitBatch.IsInvalidated())
  {
    m_WorldReader.m_pWorld->CancelComponentInitBatch(m_hComponentInitBatch);
  }

  m_Phase = Phase::Invalid;
  m_pSubProgressRange = nullptr;
  m_pOverallProgressRange = nullptr;
}

template <bool UseTransform>
bool ezWorldReader::InstantiationContext::CreateGameObjects(const ezDynamicArray<GameObjectToCreate>& objects, ezGameObjectHandle hParent,
  ezHybridArray<ezGameObject*, 8>* out_CreatedObjects, ezTime endTime)
{
  EZ_PROFILE_SCOPE("ezWorldReader::CreateGameObjects");

  while (m_uiCurrentIndex < objects.GetCount())
  {
    auto& godesc = objects[m_uiCurrentIndex];

    ezGameObjectDesc desc = godesc.m_Desc; // make a copy
    desc.m_hParent = hParent.IsInvalidated() ? m_WorldReader.m_IndexToGameObjectHandle[godesc.m_uiParentHandleIdx] : hParent;
    desc.m_bDynamic |= m_bForceDynamic;

    if (m_pOverrideTeamID != nullptr)
    {
      desc.m_uiTeamID = *m_pOverrideTeamID;
    }

    if (UseTransform)
    {
      ezTransform tChild(desc.m_LocalPosition, desc.m_LocalRotation, desc.m_LocalScaling);
      ezTransform tFinal;
      tFinal.SetGlobalTransform(m_RootTransform, tChild);

      desc.m_LocalPosition = tFinal.m_vPosition;
      desc.m_LocalRotation = tFinal.m_qRotation;
      desc.m_LocalScaling = tFinal.m_vScale;
    }

    ezGameObject* pObject = nullptr;
    m_WorldReader.m_IndexToGameObjectHandle.PushBack(m_WorldReader.m_pWorld->CreateObject(desc, pObject));

    if (!godesc.m_sGlobalKey.IsEmpty())
    {
      pObject->SetGlobalKey(godesc.m_sGlobalKey);
    }

    if (out_CreatedObjects)
    {
      out_CreatedObjects->PushBack(pObject);
    }

    ++m_uiCurrentIndex;

    // exit here to ensure that we at least did some work
    if (ezTime::Now() >= endTime)
    {
      SetSubProgressCompletion(static_cast<double>(m_uiCurrentIndex) / objects.GetCount());
      return false;
    }
  }

  m_uiCurrentIndex = 0;

  return true;
}

bool ezWorldReader::InstantiationContext::CreateComponents(ezTime endTime)
{
  EZ_PROFILE_SCOPE("ezWorldReader::CreateComponents");

  ezStreamReader& s = *m_WorldReader.m_pStream;

  for (; m_uiCurrentComponentTypeIndex < m_WorldReader.m_ComponentTypes.GetCount(); ++m_uiCurrentComponentTypeIndex)
  {
    auto& compTypeInfo = m_WorldReader.m_ComponentTypes[m_uiCurrentComponentTypeIndex];

    // will be the case for all abstract component types
    if (compTypeInfo.m_pRtti == nullptr || compTypeInfo.m_uiNumComponents == 0)
      continue;

    ezComponentManagerBase* pManager = m_WorldReader.m_pWorld->GetOrCreateManagerForComponentType(compTypeInfo.m_pRtti);
    EZ_ASSERT_DEV(pManager != nullptr, "Cannot create components of type '{0}', manager is not available.", compTypeInfo.m_pRtti->GetTypeName());

    while (m_uiCurrentIndex < compTypeInfo.m_uiNumComponents)
    {
      const ezGameObjectHandle hOwner = m_WorldReader.ReadGameObjectHandle();

      ezUInt32 uiComponentIdx = 0;
      s >> uiComponentIdx;

      bool bActive = true;
      s >> bActive;

      ezUInt8 userFlags = 0;
      s >> userFlags;

      ezGameObject* pOwnerObject = nullptr;
      m_WorldReader.m_pWorld->TryGetObject(hOwner, pOwnerObject);

      EZ_ASSERT_DEBUG(pOwnerObject != nullptr, "Owner object must be not null");

      ezComponent* pComponent = nullptr;
      auto hComponent = pManager->CreateComponentNoInit(pOwnerObject, pComponent);

      pComponent->SetActiveFlag(bActive);

      for (ezUInt8 j = 0; j < 8; ++j)
      {
        pComponent->SetUserFlag(j, (userFlags & EZ_BIT(j)) != 0);
      }

      EZ_ASSERT_DEBUG(uiComponentIdx == compTypeInfo.m_ComponentIndexToHandle.GetCount(), "Component index doesn't match");
      compTypeInfo.m_ComponentIndexToHandle.PushBack(hComponent);

      ++m_uiCurrentIndex;
      ++m_uiCurrentNumComponentsProcessed;

      // exit here to ensure that we at least did some work
      if (ezTime::Now() >= endTime)
      {
        SetSubProgressCompletion((double)m_uiCurrentNumComponentsProcessed / m_WorldReader.m_uiTotalNumComponents);
        return false;
      }
    }

    m_uiCurrentIndex = 0;
  }

  m_uiCurrentIndex = 0;
  m_uiCurrentComponentTypeIndex = 0;
  m_uiCurrentNumComponentsProcessed = 0;

  return true;
}

bool ezWorldReader::InstantiationContext::DeserializeComponents(ezTime endTime)
{
  EZ_PROFILE_SCOPE("ezWorldReader::DeserializeComponents");

  ezStreamReader& s = *m_WorldReader.m_pStream;

  for (; m_uiCurrentComponentTypeIndex < m_WorldReader.m_ComponentTypes.GetCount(); ++m_uiCurrentComponentTypeIndex)
  {
    auto& compTypeInfo = m_WorldReader.m_ComponentTypes[m_uiCurrentComponentTypeIndex];
    if (compTypeInfo.m_pRtti == nullptr)
      continue;

    while (m_uiCurrentIndex < compTypeInfo.m_ComponentIndexToHandle.GetCount())
    {
      ezComponent* pComponent = nullptr;
      if (m_WorldReader.m_pWorld->TryGetComponent(compTypeInfo.m_ComponentIndexToHandle[m_uiCurrentIndex], pComponent))
      {
        pComponent->DeserializeComponent(m_WorldReader);
      }

      ++m_uiCurrentIndex;
      ++m_uiCurrentNumComponentsProcessed;

      // exit here to ensure that we at least did some work
      if (ezTime::Now() >= endTime)
      {
        SetSubProgressCompletion((double)m_uiCurrentNumComponentsProcessed / m_WorldReader.m_uiTotalNumComponents);
        return false;
      }
    }

    m_uiCurrentIndex = 0;
  }

  m_uiCurrentIndex = 0;
  m_uiCurrentComponentTypeIndex = 0;
  m_uiCurrentNumComponentsProcessed = 0;

  return true;
}

bool ezWorldReader::InstantiationContext::AddComponentsToBatch(ezTime endTime)
{
  EZ_PROFILE_SCOPE("ezWorldReader::AddComponentsToBatch");

  ezUInt32 uiInitializedComponents = 0;

  if (!m_hComponentInitBatch.IsInvalidated())
  {
    m_WorldReader.m_pWorld->BeginAddingComponentsToInitBatch(m_hComponentInitBatch);
  }

  for (; m_uiCurrentComponentTypeIndex < m_WorldReader.m_ComponentTypes.GetCount(); ++m_uiCurrentComponentTypeIndex)
  {
    auto& compTypeInfo = m_WorldReader.m_ComponentTypes[m_uiCurrentComponentTypeIndex];
    if (compTypeInfo.m_pRtti == nullptr)
      continue;

    while (m_uiCurrentIndex < compTypeInfo.m_ComponentIndexToHandle.GetCount())
    {
      ezComponent* pComponent = nullptr;
      if (m_WorldReader.m_pWorld->TryGetComponent(compTypeInfo.m_ComponentIndexToHandle[m_uiCurrentIndex], pComponent))
      {
        pComponent->GetOwningManager()->InitializeComponent(pComponent);
        ++uiInitializedComponents;
      }

      ++m_uiCurrentIndex;
      ++m_uiCurrentNumComponentsProcessed;

      // exit here to ensure that we at least did some work
      if (ezTime::Now() >= endTime)
      {
        SetSubProgressCompletion((double)m_uiCurrentNumComponentsProcessed / m_WorldReader.m_uiTotalNumComponents);

        if (!m_hComponentInitBatch.IsInvalidated())
        {
          m_WorldReader.m_pWorld->EndAddingComponentsToInitBatch(m_hComponentInitBatch);
        }
        return false;
      }
    }

    m_uiCurrentIndex = 0;
  }

  if (!m_hComponentInitBatch.IsInvalidated())
  {
    m_WorldReader.m_pWorld->SubmitComponentInitBatch(m_hComponentInitBatch);
  }

  m_uiCurrentIndex = 0;
  m_uiCurrentComponentTypeIndex = 0;
  m_uiCurrentNumComponentsProcessed = 0;

  return true;
}

void ezWorldReader::InstantiationContext::BeginNextProgressStep(const char* szName)
{
  if (m_pOverallProgressRange != nullptr)
  {
    m_pOverallProgressRange->BeginNextStep(szName);
    m_pSubProgressRange = nullptr;
    m_pSubProgressRange = EZ_DEFAULT_NEW(ezProgressRange, szName, false, m_pOverallProgressRange->GetProgressbar());
  }
}

void ezWorldReader::InstantiationContext::SetSubProgressCompletion(double fCompletion)
{
  if (m_pSubProgressRange != nullptr)
  {
    m_pSubProgressRange->SetCompletion(fCompletion);
  }
}

EZ_STATICLINK_FILE(Core, Core_WorldSerializer_Implementation_WorldReader);
