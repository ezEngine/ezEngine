#include <RendererCorePCH.h>

#include <Core/Utils/WorldGeoExtractionUtil.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Meshes/InstancedMeshComponent.h>
#include <RendererCore/Pipeline/InstanceDataProvider.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>



//////////////////////////////////////////////////////////////////////////

// clang-format off

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshInstanceData, 1, ezRTTIDefaultAllocator<ezMeshInstanceData>)
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
EZ_END_DYNAMIC_REFLECTED_TYPE

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
    EZ_MESSAGE_HANDLER(ezMsgExtractGeometry, OnExtractGeometry)
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE

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

ezInstancedMeshComponent::ezInstancedMeshComponent() = default;
ezInstancedMeshComponent::~ezInstancedMeshComponent() = default;

void ezInstancedMeshComponent::SerializeComponent(ezWorldWriter& stream) const {}

void ezInstancedMeshComponent::DeserializeComponent(ezWorldReader& stream) {}

void ezInstancedMeshComponent::Initialize()
{
  SUPER::Initialize();

  m_pExplicitInstanceData = EZ_DEFAULT_NEW(ezInstanceData);

  UpdateRenderInstanceData();

  if (!m_rawInstancedData.IsEmpty())
    m_bExplicitInstanceDataDirty = true;
}

void ezInstancedMeshComponent::OnActivated()
{
  SUPER::OnActivated();

  m_pExplicitInstanceData = EZ_DEFAULT_NEW(ezInstanceData);

  UpdateRenderInstanceData();

  if (!m_rawInstancedData.IsEmpty())
    m_bExplicitInstanceDataDirty = true;
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
    ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::AllowFallback);
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


ezUInt32 ezInstancedMeshComponent::GetExplicitInstanceDataCount() const
{
  return m_rawInstancedData.GetCount();
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
  m_bExplicitInstanceDataDirty = true;
  UpdateRenderInstanceData(); // TODO
}

void ezInstancedMeshComponent::Instances_Insert(ezUInt32 uiIndex, ezMeshInstanceData value)
{
  m_rawInstancedData.Insert(value, uiIndex);
  m_bExplicitInstanceDataDirty = true;
}

void ezInstancedMeshComponent::Instances_Remove(ezUInt32 uiIndex)
{
  m_rawInstancedData.RemoveAtAndCopy(uiIndex);
  m_bExplicitInstanceDataDirty = true;
}

void ezInstancedMeshComponent::UpdateRenderInstanceData()
{
  if (!m_pExplicitInstanceData)
    return;

  m_pExplicitInstanceData->Reset();

  ezUInt32 uiOffset = 0;
  auto instanceData = m_pExplicitInstanceData->GetInstanceData(m_rawInstancedData.GetCount(), uiOffset);

  const ezTransform ownerTransform = GetOwner()->GetGlobalTransform();

  for (ezUInt32 i = 0; i < m_rawInstancedData.GetCount(); ++i)
  {
    //const ezTransform globalTransform = m_rawInstancedData[i].m_transform * ownerTransform;
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

    instanceData[i].GameObjectID = 0; // TODO
    instanceData[i].BoundingSphereRadius = 2.0f; // TODO

    instanceData[i].Color = m_rawInstancedData[i].m_color;
  }
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_InstancedMeshComponent);
