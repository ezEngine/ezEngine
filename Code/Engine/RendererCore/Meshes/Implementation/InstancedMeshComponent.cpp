#include <RendererCorePCH.h>

#include <Core/Utils/WorldGeoExtractionUtil.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Meshes/InstancedMeshComponent.h>
#include <RendererCore/Pipeline/InstanceDataProvider.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezMeshInstanceData, ezNoBase, 1, ezRTTIDefaultAllocator<ezMeshInstanceData>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("LocalPosition", GetLocalPosition, SetLocalPosition)->AddAttributes(new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("LocalRotation", GetLocalRotation, SetLocalRotation),
    EZ_ACCESSOR_PROPERTY("LocalScaling", GetLocalScaling, SetLocalScaling)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1.0f, 1.0f, 1.0f))),

    EZ_MEMBER_PROPERTY("Color", m_color)
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezTransformManipulatorAttribute("LocalPosition", "LocalRotation", "LocalScaling"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_STATIC_REFLECTED_TYPE
// clang-format on

void ezMeshInstanceData::SetLocalPosition(ezVec3 position)
{
  m_transform.m_vPosition = position;
}
ezVec3 ezMeshInstanceData::GetLocalPosition() const
{
  return m_transform.m_vPosition;
}

void ezMeshInstanceData::SetLocalRotation(ezQuat rotation)
{
  m_transform.m_qRotation = rotation;
}

ezQuat ezMeshInstanceData::GetLocalRotation() const
{
  return m_transform.m_qRotation;
}

void ezMeshInstanceData::SetLocalScaling(ezVec3 scaling)
{
  m_transform.m_vScale = scaling;
}

ezVec3 ezMeshInstanceData::GetLocalScaling() const
{
  return m_transform.m_vScale;
}

static const ezTypeVersion s_MeshInstanceDataVersion = 1;
ezResult ezMeshInstanceData::Serialize(ezStreamWriter& writer) const
{
  writer.WriteVersion(s_MeshInstanceDataVersion);

  writer << m_transform;
  writer << m_color;

  return EZ_SUCCESS;
}

ezResult ezMeshInstanceData::Deserialize(ezStreamReader& reader)
{
  /*auto version = */ reader.ReadVersion(s_MeshInstanceDataVersion);

  reader >> m_transform;
  reader >> m_color;

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezInstancedMeshRenderData, 1, ezRTTIDefaultAllocator<ezInstancedMeshRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezInstancedMeshRenderData::FillBatchIdAndSortingKey()
{
  FillBatchIdAndSortingKeyInternal(m_uiUniqueID);
}

//////////////////////////////////////////////////////////////////////////////////////

ezInstancedMeshComponentManager::ezInstancedMeshComponentManager(ezWorld* pWorld)
  : SUPER(pWorld)
{
}

void ezInstancedMeshComponentManager::EnqueueUpdate(const ezInstancedMeshComponent* pComponent) const
{
  ezUInt64 uiCurrentFrame = ezRenderWorld::GetFrameCounter();
  if (pComponent->m_uiEnqueuedFrame == uiCurrentFrame)
    return;

  EZ_LOCK(m_Mutex);
  if (pComponent->m_uiEnqueuedFrame == uiCurrentFrame)
    return;

  auto instanceData = pComponent->GetInstanceData();
  if (instanceData.IsEmpty())
    return;

  m_RequireUpdate.PushBack({pComponent->GetHandle(), instanceData});
  pComponent->m_uiEnqueuedFrame = uiCurrentFrame;
}

void ezInstancedMeshComponentManager::OnRenderBegin(ezUInt64 uiFrameCounter)
{
  EZ_LOCK(m_Mutex);

  if (m_RequireUpdate.IsEmpty())
    return;

  for (const auto& componentToUpdate : m_RequireUpdate)
  {
    ezInstancedMeshComponent* pComp = nullptr;
    if (!TryGetComponent(componentToUpdate.m_hComponent, pComp))
      continue;

    if (pComp->m_pExplicitInstanceData)
    {
      ezUInt32 uiOffset = 0;
      auto instanceData = pComp->m_pExplicitInstanceData->GetInstanceData(componentToUpdate.m_InstanceData.GetCount(), uiOffset);
      instanceData.CopyFrom(componentToUpdate.m_InstanceData);

      pComp->m_pExplicitInstanceData->UpdateInstanceData(ezRenderContext::GetDefaultInstance(), instanceData.GetCount());
    }
  }

  m_RequireUpdate.Clear();
}

void ezInstancedMeshComponentManager::Initialize()
{
  SUPER::Initialize();

  ezRenderWorld::s_BeginRenderEvent.AddEventHandler(ezMakeDelegate(&ezInstancedMeshComponentManager::OnRenderBegin, this));
}

void ezInstancedMeshComponentManager::Deinitialize()
{
  ezRenderWorld::s_BeginRenderEvent.RemoveEventHandler(ezMakeDelegate(&ezInstancedMeshComponentManager::OnRenderBegin, this));

  SUPER::Deinitialize();
}

//////////////////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezInstancedMeshComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Mesh", GetMeshFile, SetMeshFile)->AddAttributes(new ezAssetBrowserAttribute("Mesh")),
    EZ_ACCESSOR_PROPERTY("MainColor", GetColor, SetColor)->AddAttributes(new ezExposeColorAlphaAttribute()),
    EZ_ARRAY_ACCESSOR_PROPERTY("Materials", Materials_GetCount, Materials_GetValue, Materials_SetValue, Materials_Insert, Materials_Remove)->AddAttributes(new ezAssetBrowserAttribute("Material")),

    EZ_ARRAY_ACCESSOR_PROPERTY("InstanceData", Instances_GetCount, Instances_GetValue, Instances_SetValue, Instances_Insert, Instances_Remove),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractGeometry, OnExtractGeometry),
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezInstancedMeshComponent::ezInstancedMeshComponent() = default;
ezInstancedMeshComponent::~ezInstancedMeshComponent() = default;

void ezInstancedMeshComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  stream.GetStream().WriteArray(m_rawInstancedData);
}

void ezInstancedMeshComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);

  stream.GetStream().ReadArray(m_rawInstancedData);
}

void ezInstancedMeshComponent::OnActivated()
{
  SUPER::OnActivated();

  EZ_ASSERT_DEV(m_pExplicitInstanceData == nullptr, "Instance data must not be initialized at this point");
  m_pExplicitInstanceData = EZ_DEFAULT_NEW(ezInstanceData);
}

void ezInstancedMeshComponent::OnDeactivated()
{
  EZ_DEFAULT_DELETE(m_pExplicitInstanceData);
  m_pExplicitInstanceData = nullptr;

  SUPER::OnDeactivated();
}

void ezInstancedMeshComponent::OnExtractGeometry(ezMsgExtractGeometry& msg) {}

ezResult ezInstancedMeshComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  ezBoundingBoxSphere singleBounds;
  if (m_hMesh.IsValid())
  {
    ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::AllowLoadingFallback);
    singleBounds = pMesh->GetBounds();

    for (const auto& instance : m_rawInstancedData)
    {
      auto instanceBounds = singleBounds;
      instanceBounds.Transform(instance.m_transform.GetAsMat4());

      bounds.ExpandToInclude(instanceBounds);
    }

    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezInstancedMeshComponent::OnExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  SUPER::OnExtractRenderData(msg);

  static_cast<const ezInstancedMeshComponentManager*>(GetOwningManager())->EnqueueUpdate(this);
}

ezMeshRenderData* ezInstancedMeshComponent::CreateRenderData() const
{
  auto pRenderData = ezCreateRenderDataForThisFrame<ezInstancedMeshRenderData>(GetOwner());

  if (m_pExplicitInstanceData)
  {
    pRenderData->m_pExplicitInstanceData = m_pExplicitInstanceData;
    pRenderData->m_uiExplicitInstanceCount = m_rawInstancedData.GetCount();
  }

  return pRenderData;
}

ezUInt32 ezInstancedMeshComponent::Instances_GetCount() const
{
  return m_rawInstancedData.GetCount();
}

ezMeshInstanceData ezInstancedMeshComponent::Instances_GetValue(ezUInt32 uiIndex) const
{
  return m_rawInstancedData[uiIndex];
}

void ezInstancedMeshComponent::Instances_SetValue(ezUInt32 uiIndex, ezMeshInstanceData value)
{
  m_rawInstancedData[uiIndex] = value;

  TriggerLocalBoundsUpdate();
}

void ezInstancedMeshComponent::Instances_Insert(ezUInt32 uiIndex, ezMeshInstanceData value)
{
  m_rawInstancedData.Insert(value, uiIndex);

  TriggerLocalBoundsUpdate();
}

void ezInstancedMeshComponent::Instances_Remove(ezUInt32 uiIndex)
{
  m_rawInstancedData.RemoveAtAndCopy(uiIndex);

  TriggerLocalBoundsUpdate();
}

ezArrayPtr<ezPerInstanceData> ezInstancedMeshComponent::GetInstanceData() const
{
  if (!m_pExplicitInstanceData || m_rawInstancedData.IsEmpty())
    return ezArrayPtr<ezPerInstanceData>();

  auto instanceData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezPerInstanceData, m_rawInstancedData.GetCount());

  const ezTransform ownerTransform = GetOwner()->GetGlobalTransform();

  float fBoundingSphereRadius = 1.0f;

  if (m_hMesh.IsValid())
  {
    ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::AllowLoadingFallback);
    fBoundingSphereRadius = pMesh->GetBounds().GetSphere().m_fRadius;
  }

  for (ezUInt32 i = 0; i < m_rawInstancedData.GetCount(); ++i)
  {
    const ezTransform globalTransform = ownerTransform * m_rawInstancedData[i].m_transform;
    const ezMat4 objectToWorld = globalTransform.GetAsMat4();

    instanceData[i].ObjectToWorld = objectToWorld;

    if (m_rawInstancedData[i].m_transform.ContainsUniformScale())
    {
      instanceData[i].ObjectToWorldNormal = objectToWorld;
    }
    else
    {
      ezMat3 mInverse = objectToWorld.GetRotationalPart();
      mInverse.Invert(0.0f);
      // we explicitly ignore the return value here (success / failure)
      // because when we have a scale of 0 (which happens temporarily during editing) that would be annoying

      ezShaderTransform shaderT;
      shaderT = mInverse.GetTranspose();
      instanceData[i].ObjectToWorldNormal = shaderT;
    }

    instanceData[i].GameObjectID = GetUniqueIdForRendering();
    instanceData[i].BoundingSphereRadius = fBoundingSphereRadius * m_rawInstancedData[i].m_transform.GetMaxScale();

    instanceData[i].Color = m_Color * m_rawInstancedData[i].m_color;
  }

  return instanceData;
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_InstancedMeshComponent);
