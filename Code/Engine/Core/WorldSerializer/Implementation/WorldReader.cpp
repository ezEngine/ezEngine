#include <Core/PCH.h>
#include <Core/WorldSerializer/WorldReader.h>


void ezWorldReader::Read(ezStreamReader& stream, ezWorld& world, const ezVec3& vRootPosition, const ezQuat& qRootRotation, const ezVec3& vRootScale)
{
  m_pStream = &stream;
  m_pWorld = &world;

  m_vRootPosition = vRootPosition;
  m_qRootRotation = qRootRotation;
  m_vRootScale = vRootScale;

  EZ_LOCK(m_pWorld->GetWriteMarker());

  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion == 1, "Invalid version");

  ezUInt32 uiNumRootObjects = 0;
  stream >> uiNumRootObjects;

  ezUInt32 uiNumChildObjects = 0;
  stream >> uiNumChildObjects;

  ezUInt32 uiNumComponentTypes = 0;
  stream >> uiNumComponentTypes;

  ezUInt32 uiNumComponents = 0;
  stream >> uiNumComponents;

  m_IndexToGameObjectHandle.Reserve(uiNumRootObjects + uiNumChildObjects + 1);
  m_IndexToGameObjectHandle.PushBack(ezGameObjectHandle());

  m_IndexToComponentHandle.Reserve(uiNumComponents + 1);
  m_IndexToComponentHandle.PushBack(ezComponentHandle());

  for (ezUInt32 i = 0; i < uiNumRootObjects; ++i)
  {
    auto pObject = ReadGameObject(true);
    m_IndexToGameObjectHandle.PushBack(pObject->GetHandle());
  }

  for (ezUInt32 i = 0; i < uiNumChildObjects; ++i)
  {
    auto pObject = ReadGameObject(false);
    m_IndexToGameObjectHandle.PushBack(pObject->GetHandle());
  }

  for (ezUInt32 i = 0; i < uiNumComponentTypes; ++i)
  {
    ReadComponentsOfType();
  }

  FulfillComponentHandleRequets();
}

ezGameObjectHandle ezWorldReader::ReadHandle()
{
  ezUInt32 idx = 0;
  *m_pStream >> idx;

  return m_IndexToGameObjectHandle[idx];
}

void ezWorldReader::ReadHandle(ezComponentHandle* out_hComponent)
{
  ezUInt32 idx = 0;
  *m_pStream >> idx;

  CompRequest r;
  r.m_pWriteToComponent = out_hComponent;
  r.m_uiComponentIndex = idx;

  m_ComponentHandleRequests.PushBack(r);
}

ezGameObject* ezWorldReader::ReadGameObject(bool bRoot)
{
  ezGameObjectDesc desc;
  ezStringBuilder sName;

  desc.m_Flags = ezObjectFlags::None;
  desc.m_hParent = ReadHandle();

  *m_pStream >> sName;
  *m_pStream >> desc.m_LocalPosition;
  *m_pStream >> desc.m_LocalRotation;
  *m_pStream >> desc.m_LocalScaling;

  if (bRoot)
  {
    ezTransform tRoot(m_vRootPosition, m_qRootRotation, m_vRootScale);
    ezTransform tChild(desc.m_LocalPosition, desc.m_LocalRotation, desc.m_LocalScaling);

    ezTransform tNew;
    tNew.SetGlobalTransform(tRoot, tChild);

    tNew.Decompose(desc.m_LocalPosition, desc.m_LocalRotation, desc.m_LocalScaling);
  }

  bool bActive = true;
  *m_pStream >> bActive;

  bool bDynamic = true;
  *m_pStream >> bDynamic;

  desc.m_Flags.AddOrRemove(ezObjectFlags::Active, bActive);
  desc.m_Flags.AddOrRemove(ezObjectFlags::Dynamic, bDynamic);

  desc.m_sName.Assign(sName.GetData());

  // desc.m_Flags ..

  ezGameObject* pObject;
  m_pWorld->CreateObject(desc, pObject);

  return pObject;
}

void ezWorldReader::ReadComponentsOfType()
{
  ezStreamReader& s = *m_pStream;

  ezStringBuilder sRttiName;
  ezUInt32 uiRttiVersion = 0;
  ezUInt32 uiNumComponents = 0;

  s >> sRttiName;
  s >> uiRttiVersion;
  s >> uiNumComponents;

  /// \todo rtti version stuff

  const ezRTTI* pRtti = ezRTTI::FindTypeByName(sRttiName);

  /// \todo Skip unknown types
  EZ_ASSERT_DEV(pRtti != nullptr, "Unknown rtti type '%s'", sRttiName.GetData());

  auto* pManager = m_pWorld->GetComponentManager(pRtti);
  EZ_ASSERT_DEV(pManager != nullptr, "Cannot create components of type '%s'", sRttiName.GetData());

  for (ezUInt32 i = 0; i < uiNumComponents; ++i)
  {
    /// \todo redirect to memory stream, record data size

    const ezGameObjectHandle hOwner = ReadHandle();

    bool bActive = true;
    *m_pStream >> bActive;

    bool bDynamic = true;
    *m_pStream >> bDynamic;

    auto hComponent = pManager->CreateComponent();
    m_IndexToComponentHandle.PushBack(hComponent);

    ezComponent* pComponent = nullptr;
    pManager->TryGetComponent(hComponent, pComponent);

    pComponent->SetActive(bActive);
    /// \todo currently everything is always dynamic

    pComponent->DeserializeComponent(*this, uiRttiVersion);

    ezGameObject* pParentObject = nullptr;
    m_pWorld->TryGetObject(hOwner, pParentObject);

    pParentObject->AddComponent(pComponent);
  }
}

void ezWorldReader::FulfillComponentHandleRequets()
{
  for (const auto& req : m_ComponentHandleRequests)
  {
    *req.m_pWriteToComponent = m_IndexToComponentHandle[req.m_uiComponentIndex];
  }
}



EZ_STATICLINK_FILE(Core, Core_WorldSerializer_Implementation_WorldReader);

