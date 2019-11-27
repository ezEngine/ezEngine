#include <RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/SkeletonBuilder.h>
#include <RendererCore/AnimationSystem/VisualizeSkeletonComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Debug/DebugRendererContext.h>
#include <RendererCore/Pipeline/RenderData.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezVisualizeSkeletonComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Skeleton", GetSkeletonFile, SetSkeletonFile)->AddAttributes(new ezAssetBrowserAttribute("Skeleton")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Animation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualizeSkeletonComponentManager::ezVisualizeSkeletonComponentManager(ezWorld* pWorld)
  : SUPER(pWorld)
{
}

ezVisualizeSkeletonComponent::ezVisualizeSkeletonComponent() = default;
ezVisualizeSkeletonComponent::~ezVisualizeSkeletonComponent() = default;

void ezVisualizeSkeletonComponent::Render()
{
  if (!m_hSkeleton.IsValid())
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded);

  if (pSkeleton.GetAcquireResult() == ezResourceAcquireResult::MissingFallback)
    return;

  const ezSkeletonResourceDescriptor& desc = pSkeleton->GetDescriptor();
  const ezSkeleton& skeleton = desc.m_Skeleton;

  ezAnimationPose pose;
  pose.Configure(skeleton);
  pose.SetToBindPoseInLocalSpace(skeleton);
  pose.ConvertFromLocalSpaceToObjectSpace(skeleton);

  pose.VisualizePose(GetWorld(), skeleton, GetOwner()->GetGlobalTransform(), 0);

  ezQuat qRotCapsule;
  qRotCapsule.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90));

  for (const auto& geo : desc.m_Geometry)
  {
    const ezVec3 scale = geo.m_Transform.m_vScale;

    ezTransform geoPose;
    geoPose.SetFromMat4(pose.GetTransform(geo.m_uiAttachedToJoint));
    geoPose = geoPose * ezTransform(geo.m_Transform.m_vPosition, geo.m_Transform.m_qRotation); // remove scale

    switch (geo.m_Type)
    {
      case ezSkeletonJointGeometryType::Box:
      {
        ezBoundingBox box;
        box.SetCenterAndHalfExtents(ezVec3::ZeroVector(), scale);
        ezDebugRenderer::DrawLineBox(GetWorld(), box, ezColor::IndianRed, geoPose);
        break;
      }

      case ezSkeletonJointGeometryType::Capsule:
      {
        geoPose.m_qRotation = geoPose.m_qRotation * qRotCapsule;
        ezDebugRenderer::DrawLineCapsuleZ(GetWorld(), scale.x, scale.z, ezColor::IndianRed, geoPose);
        break;
      }

      case ezSkeletonJointGeometryType::Sphere:
      {
        ezBoundingSphere sphere(ezVec3::ZeroVector(), scale.z);
        ezDebugRenderer::DrawLineSphere(GetWorld(), sphere, ezColor::IndianRed, geoPose);
        break;
      }
    }
  }
}

void ezVisualizeSkeletonComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_hSkeleton;
}

void ezVisualizeSkeletonComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_hSkeleton;

  GetWorld()->GetOrCreateComponentManager<ezVisualizeSkeletonComponentManager>()->EnqueueUpdate(GetHandle());
}

void ezVisualizeSkeletonComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // TODO: check if there is a sibling mesh component and get the skeleton + pose data from that
}

ezResult ezVisualizeSkeletonComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  // have to assume this isn't thread safe
  // CreateRenderMesh();

  if (m_hMesh.IsValid())
  {
    ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::AllowLoadingFallback);
    bounds = pMesh->GetBounds();
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezVisualizeSkeletonComponent::SetSkeletonFile(const char* szFile)
{
  ezSkeletonResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezSkeletonResource>(szFile);
  }

  SetSkeleton(hResource);
}

const char* ezVisualizeSkeletonComponent::GetSkeletonFile() const
{
  if (!m_hSkeleton.IsValid())
    return "";

  return m_hSkeleton.GetResourceID();
}

void ezVisualizeSkeletonComponent::SetSkeleton(const ezSkeletonResourceHandle& hResource)
{
  if (m_hSkeleton != hResource)
  {
    m_hSkeleton = hResource;
    m_hMesh.Invalidate();

    GetWorld()->GetOrCreateComponentManager<ezVisualizeSkeletonComponentManager>()->EnqueueUpdate(GetHandle());
  }
}

static ezTransform ComputeJointTransform(const ezSkeleton& skeleton, const ezSkeletonJoint& joint)
{
  if (joint.IsRootJoint())
    return joint.GetBindPoseLocalTransform();

  const ezTransform parentMat = ComputeJointTransform(skeleton, skeleton.GetJointByIndex(joint.GetParentIndex()));

  return parentMat * joint.GetBindPoseLocalTransform();
}

void ezVisualizeSkeletonComponent::CreateRenderMesh()
{
  if (!m_hSkeleton.IsValid())
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded);

  if (pSkeleton.GetAcquireResult() == ezResourceAcquireResult::MissingFallback)
    return;

  ezStringBuilder sVisMeshName = pSkeleton->GetResourceID();
  sVisMeshName.AppendFormat("_{0}_VisSkeletonMesh",
    pSkeleton->GetCurrentResourceChangeCounter()); // the change counter allows to react to resource updates

  m_hMesh = ezResourceManager::GetExistingResource<ezMeshResource>(sVisMeshName);

  if (m_hMesh.IsValid())
  {
    TriggerLocalBoundsUpdate();
    return;
  }



  const ezSkeleton* pSkeletonData = &pSkeleton->GetDescriptor().m_Skeleton;

  if (pSkeletonData->GetJointCount() == 0)
    return;

  ezMeshResourceDescriptor md;
  auto& buffer = md.MeshBufferDesc();

  {
    ezGeometry geo;
    CreateSkeletonGeometry(pSkeletonData, geo);
    const ezUInt32 uiSubmeshTris1 = geo.CalculateTriangleCount();
    CreateHitBoxGeometry(&pSkeleton->GetDescriptor(), geo);
    const ezUInt32 uiSubmeshTris2 = geo.CalculateTriangleCount() - uiSubmeshTris1;

    buffer.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    buffer.AddStream(ezGALVertexAttributeSemantic::BoneWeights0, ezGALResourceFormat::XYZWFloat);
    buffer.AddStream(ezGALVertexAttributeSemantic::BoneIndices0, ezGALResourceFormat::RGBAUShort);

    // this will move the custom index into the first joint index
    buffer.AllocateStreamsFromGeometry(geo);

    md.AddSubMesh(uiSubmeshTris1, 0, 0);
    md.AddSubMesh(uiSubmeshTris2, uiSubmeshTris1, 1);
  }

  md.ComputeBounds();
  md.SetMaterial(0, "Materials/Common/SkeletonVisualization.ezMaterial");
  md.SetMaterial(1, "Materials/Common/HitBoxVisualization.ezMaterial");

  m_hMesh = ezResourceManager::CreateResource<ezMeshResource>(sVisMeshName, std::move(md), "Skeleton Visualization");

  TriggerLocalBoundsUpdate();
}

void ezVisualizeSkeletonComponent::CreateSkeletonGeometry(const ezSkeleton* pSkeletonData, ezGeometry& geo)
{
  const ezUInt32 uiNumJoints = pSkeletonData->GetJointCount();

  for (ezUInt32 b = 0; b < uiNumJoints; ++b)
  {
    const auto& joint = pSkeletonData->GetJointByIndex(b);

    const ezTransform mJoint = ComputeJointTransform(*pSkeletonData, joint);

    geo.AddSphere(0.03f, 10, 10, ezColor::RebeccaPurple, mJoint.GetAsMat4(), b);

    if (!joint.IsRootJoint())
    {
      const ezTransform mParentJoint = ComputeJointTransform(*pSkeletonData, pSkeletonData->GetJointByIndex(joint.GetParentIndex()));

      const ezVec3 vTargetPos = mJoint.m_vPosition;
      const ezVec3 vSourcePos = mParentJoint.m_vPosition;

      ezVec3 vJointDir = vTargetPos - vSourcePos;
      const float fJointLen = vJointDir.GetLength();

      if (fJointLen <= 0.0f)
        continue;

      vJointDir /= fJointLen;

      ezMat4 mScale;
      mScale.SetScalingMatrix(ezVec3(1, 1, fJointLen));

      ezQuat qRot;
      qRot.SetShortestRotation(ezVec3(0, 0, 1), vJointDir);

      ezMat4 mTransform;
      mTransform = qRot.GetAsMat4() * mScale;
      mTransform.SetTranslationVector(vSourcePos);

      geo.AddCone(0.02f, 1.0f, false, 4, ezColor::CornflowerBlue /* The Original! */, mTransform, b);
    }
  }
}

void ezVisualizeSkeletonComponent::CreateHitBoxGeometry(const ezSkeletonResourceDescriptor* pDescriptor, ezGeometry& geo)
{
  ezMat4 mRotCapsule;
  mRotCapsule.SetRotationMatrixY(ezAngle::Degree(90));

  for (const auto& jointGeo : pDescriptor->m_Geometry)
  {
    const auto& joint = pDescriptor->m_Skeleton.GetJointByIndex(jointGeo.m_uiAttachedToJoint);

    const ezTransform jointTransform = ComputeJointTransform(pDescriptor->m_Skeleton, joint);

    switch (jointGeo.m_Type)
    {
      case ezSkeletonJointGeometryType::Box:
      {
        geo.AddBox(jointGeo.m_Transform.m_vScale, ezColor::White, jointTransform.GetAsMat4(), jointGeo.m_uiAttachedToJoint);
      }
      break;

      case ezSkeletonJointGeometryType::Capsule:
      {
        geo.AddCapsule(jointGeo.m_Transform.m_vScale.z, jointGeo.m_Transform.m_vScale.x, 16, 4, ezColor::White,
          jointTransform.GetAsMat4() * mRotCapsule, jointGeo.m_uiAttachedToJoint);
      }
      break;

      case ezSkeletonJointGeometryType::Sphere:
      {
        geo.AddGeodesicSphere(jointGeo.m_Transform.m_vScale.z, 1, ezColor::White, jointTransform.GetAsMat4(), jointGeo.m_uiAttachedToJoint);
      }
      break;
    }
  }
}

void ezVisualizeSkeletonComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::AllowLoadingFallback);
  ezArrayPtr<const ezMeshResourceDescriptor::SubMesh> parts = pMesh->GetSubMeshes();

  for (ezUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    const ezUInt32 uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;
    ezMaterialResourceHandle hMaterial = pMesh->GetMaterials()[uiMaterialIndex];

    ezMeshRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezMeshRenderData>(GetOwner());
    {
      pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
      pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
      pRenderData->m_hMesh = m_hMesh;
      pRenderData->m_hMaterial = hMaterial;
      pRenderData->m_uiSubMeshIndex = uiPartIndex;
      pRenderData->m_uiUniqueID = GetUniqueIdForRendering(uiMaterialIndex);

      pRenderData->FillBatchIdAndSortingKey();
    }

    msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::LitOpaque, ezRenderData::Caching::Never);
  }
}

//////////////////////////////////////////////////////////////////////////

void ezVisualizeSkeletonComponentManager::Initialize()
{
  SUPER::Initialize();

  ezWorldModule::UpdateFunctionDesc desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezVisualizeSkeletonComponentManager::Update, this);
  desc.m_Phase = UpdateFunctionDesc::Phase::PreAsync;

  RegisterUpdateFunction(desc);

  ezResourceManager::GetResourceEvents().AddEventHandler(ezMakeDelegate(&ezVisualizeSkeletonComponentManager::ResourceEventHandler, this));
}

void ezVisualizeSkeletonComponentManager::Deinitialize()
{
  EZ_LOCK(m_Mutex);

  ezResourceManager::GetResourceEvents().RemoveEventHandler(ezMakeDelegate(&ezVisualizeSkeletonComponentManager::ResourceEventHandler, this));

  SUPER::Deinitialize();
}

void ezVisualizeSkeletonComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    it->Render();
  }

  // for (const auto& hComp : m_RequireUpdate)
  //{
  //  ezVisualizeSkeletonComponent* pComp = nullptr;
  //  if (!TryGetComponent(hComp, pComp))
  //    continue;

  //  pComp->CreateRenderMesh();
  //}

  m_RequireUpdate.Clear();
}

void ezVisualizeSkeletonComponentManager::EnqueueUpdate(ezComponentHandle hComponent)
{
  m_RequireUpdate.PushBack(hComponent);
}

void ezVisualizeSkeletonComponentManager::ResourceEventHandler(const ezResourceEvent& e)
{
  if ((e.m_Type == ezResourceEvent::Type::ResourceContentUnloading || e.m_Type == ezResourceEvent::Type::ResourceContentUpdated) &&
      e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<ezSkeletonResource>())
  {
    EZ_LOCK(m_Mutex);

    ezSkeletonResourceHandle hResource((ezSkeletonResource*)(e.m_pResource));

    for (auto it = m_Components.GetIterator(); it.IsValid(); ++it)
    {
      const ezVisualizeSkeletonComponent* pComponent = static_cast<ezVisualizeSkeletonComponent*>(it.Value());

      if (pComponent->GetSkeleton() == hResource)
      {
        m_RequireUpdate.PushBack(pComponent->GetHandle());
      }
    }
  }
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_VisualizeSkeletonComponent);
