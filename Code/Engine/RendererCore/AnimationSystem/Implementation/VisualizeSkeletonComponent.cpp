#include <PCH.h>
#include <RendererCore/AnimationSystem/VisualizeSkeletonComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <Core/Graphics/Geometry.h>
#include <RendererCore/AnimationSystem/SkeletonBuilder.h>

ezVisualizeSkeletonComponentManager::ezVisualizeSkeletonComponentManager(ezWorld* pWorld) : SUPER(pWorld)
{
}

EZ_BEGIN_COMPONENT_TYPE(ezVisualizeSkeletonComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Skeleton", GetSkeletonFile, SetSkeletonFile)->AddAttributes(new ezAssetBrowserAttribute("Skeleton")),
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Animation"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualizeSkeletonComponent::ezVisualizeSkeletonComponent() = default;
ezVisualizeSkeletonComponent::~ezVisualizeSkeletonComponent() = default;

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
  //CreateRenderMesh();

  if (m_hMesh.IsValid())
  {
    ezResourceLock<ezMeshResource> pMesh(m_hMesh);
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

ezMeshRenderData* ezVisualizeSkeletonComponent::CreateRenderData(ezUInt32 uiBatchId) const
{
  return ezCreateRenderDataForThisFrame<ezMeshRenderData>(GetOwner(), uiBatchId);
}

static ezMat4 ComputeBoneMatrix(const ezSkeleton& skeleton, const ezSkeleton::Bone& bone)
{
  if (bone.IsRootBone())
    return bone.GetBoneTransform();

  ezMat4 parentMat = ComputeBoneMatrix(skeleton, skeleton.GetBone(bone.GetParentIndex()));

  return parentMat * bone.GetBoneTransform();
}

void ezVisualizeSkeletonComponent::CreateRenderMesh()
{
  if (!m_hSkeleton.IsValid())
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::NoFallback);

  if (pSkeleton->IsMissingResource())
    return;

  ezStringBuilder sVisMeshName = pSkeleton->GetResourceID();
  sVisMeshName.AppendFormat("_{0}_VisSkeletonMesh", pSkeleton->GetCurrentResourceChangeCounter()); // the change counter allows to react to resource updates

  m_hMesh = ezResourceManager::GetExistingResource<ezMeshResource>(sVisMeshName);

  if (m_hMesh.IsValid())
  {
    TriggerLocalBoundsUpdate();
    return;
  }

  

  const ezSkeleton* pSkeletonData = &pSkeleton->GetDescriptor().m_Skeleton;

  if (pSkeletonData->GetBoneCount() == 0)
    return;

  ezMeshResourceDescriptor md;
  auto& buffer = md.MeshBufferDesc();

  // create a realistic humanoid dummy skeleton
  //{
  //  ezMat4 m;
  //  ezSkeletonBuilder builder;

  //  m.SetTranslationMatrix(ezVec3(0, 0, 0.0f));
  //  const ezUInt32 root = builder.AddBone("Root", m);

  //  m.SetTranslationMatrix(ezVec3(0, 0, 1.0f));
  //  const ezUInt32 body = builder.AddBone("Body", m, root);

  //  m.SetTranslationMatrix(ezVec3(0, 0, 0.6f));
  //  const ezUInt32 head = builder.AddBone("Head", m, body);

  //  m.SetTranslationMatrix(ezVec3(0.2f, 0, 0));
  //  const ezUInt32 nose = builder.AddBone("Nose", m, head);

  //  m.SetTranslationMatrix(ezVec3(0, 0.5f, 0));
  //  const ezUInt32 rarm = builder.AddBone("RArm", m, body);

  //  m.SetTranslationMatrix(ezVec3(0, 0.3f, 0.1f));
  //  const ezUInt32 rarm2 = builder.AddBone("RArm2", m, rarm);

  //  m.SetTranslationMatrix(ezVec3(0, -0.4f, -0.1f));
  //  const ezUInt32 larm = builder.AddBone("LArm", m, body);

  //  m.SetTranslationMatrix(ezVec3(0, -0.1f, -0.2f));
  //  const ezUInt32 larm2 = builder.AddBone("LArm2", m, larm);

  //  pSkeletonData = builder.CreateSkeletonInstance();
  //}


  {
    ezGeometry geo;
    CreateSkeletonGeometry(pSkeletonData, geo);
    const ezUInt32 uiSubmeshTris1 = geo.CalculateTriangleCount();
    CreateHitBoxGeometry(&pSkeleton->GetDescriptor(), geo);
    const ezUInt32 uiSubmeshTris2 = geo.CalculateTriangleCount() - uiSubmeshTris1;

    buffer.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    buffer.AddStream(ezGALVertexAttributeSemantic::BoneWeights0, ezGALResourceFormat::XYZWFloat);
    buffer.AddStream(ezGALVertexAttributeSemantic::BoneIndices0, ezGALResourceFormat::RGBAUShort);

    // this will move the custom index into the first bone index
    buffer.AllocateStreamsFromGeometry(geo);

    md.AddSubMesh(uiSubmeshTris1, 0, 0);
    md.AddSubMesh(uiSubmeshTris2, uiSubmeshTris1, 1);
  }

  md.ComputeBounds();
  md.SetMaterial(0, "Materials/Common/SkeletonVisualization.ezMaterial");
  md.SetMaterial(1, "Materials/Common/HitBoxVisualization.ezMaterial");

  m_hMesh = ezResourceManager::CreateResource<ezMeshResource>(sVisMeshName, md, "Skeleton Visualization");

  TriggerLocalBoundsUpdate();
}

void ezVisualizeSkeletonComponent::CreateSkeletonGeometry(const ezSkeleton* pSkeletonData, ezGeometry& geo)
{
  //const ezAnimationPose* pPose = pSkeletonData->GetBindSpacePoseInObjectSpace();
  //const ezUInt32 uiNumBones = pPose->GetBoneTransformCount();
  const ezUInt32 uiNumBones = pSkeletonData->GetBoneCount();

  for (ezUInt32 b = 0; b < uiNumBones; ++b)
  {
    const auto& bone = pSkeletonData->GetBone(b);

    const ezMat4 mBone = ComputeBoneMatrix(*pSkeletonData, bone);
    //const ezMat4 mBone = pPose->GetBoneTransform(b);

    geo.AddSphere(0.03f, 10, 10, ezColor::RebeccaPurple, mBone, b);

    if (!bone.IsRootBone())
    {
      const ezMat4 mParentBone = ComputeBoneMatrix(*pSkeletonData, pSkeletonData->GetBone(bone.GetParentIndex()));
      //const ezMat4 mParentBone = pPose->GetBoneTransform(bone.GetParentIndex());

      const ezVec3 vTargetPos = mBone.GetTranslationVector();
      const ezVec3 vSourcePos = mParentBone.GetTranslationVector();

      ezVec3 vBoneDir = vTargetPos - vSourcePos;
      const float fBoneLen = vBoneDir.GetLengthAndNormalize();

      ezMat4 mScale;
      mScale.SetScalingMatrix(ezVec3(1, 1, fBoneLen));

      ezQuat qRot;
      qRot.SetShortestRotation(ezVec3(0, 0, 1), vBoneDir);

      ezMat4 mTransform;
      mTransform = qRot.GetAsMat4() * mScale;
      mTransform.SetTranslationVector(vSourcePos);

      geo.AddCone(0.02f, 1.0f, false, 4, ezColor::CornflowerBlue /* The Original! */, mTransform, b);
    }
  }
}

void ezVisualizeSkeletonComponent::CreateHitBoxGeometry(const ezSkeletonResourceDescriptor* pDescriptor, ezGeometry &geo)
{
  for (const auto& boneGeo : pDescriptor->m_Geometry)
  {
    const auto& bone = pDescriptor->m_Skeleton.GetBone(boneGeo.m_uiAttachedToBone);
    const ezMat4 boneTransform = bone.GetBoneTransform();

    switch (boneGeo.m_Type)
    {
    case ezSkeletonBoneGeometryType::Box:
      {
        geo.AddBox(boneGeo.m_Transform.m_vScale, ezColor::White, boneTransform, boneGeo.m_uiAttachedToBone);
      }
      break;

    case ezSkeletonBoneGeometryType::Capsule:
      {
        geo.AddCapsule(boneGeo.m_Transform.m_vScale.z, boneGeo.m_Transform.m_vScale.x, 16, 4, ezColor::White, boneTransform, boneGeo.m_uiAttachedToBone);
      }
      break;

    case ezSkeletonBoneGeometryType::Sphere:
      {
        geo.AddGeodesicSphere(boneGeo.m_Transform.m_vScale.z, 1, ezColor::White, boneTransform, boneGeo.m_uiAttachedToBone);
      }
      break;
    }
  }
}

void ezVisualizeSkeletonComponent::OnExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  const ezUInt32 uiMeshIDHash = m_hMesh.GetResourceIDHash();

  ezResourceLock<ezMeshResource> pMesh(m_hMesh);
  const ezDynamicArray<ezMeshResourceDescriptor::SubMesh>& parts = pMesh->GetSubMeshes();

  for (ezUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    const ezUInt32 uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;
    ezMaterialResourceHandle hMaterial;

    // todo use a built in material ?
    hMaterial = pMesh->GetMaterials()[uiMaterialIndex];

    const ezUInt32 uiMaterialIDHash = hMaterial.IsValid() ? hMaterial.GetResourceIDHash() : 0;
    const ezUInt32 uiFlipWinding = GetOwner()->GetGlobalTransformSimd().ContainsNegativeScale() ? 1 : 0;

    // Generate batch id from mesh, material and part index.
    ezUInt32 data[] = { uiMeshIDHash, uiMaterialIDHash, uiPartIndex, uiFlipWinding };
    ezUInt32 uiBatchId = ezHashing::xxHash32(data, sizeof(data));

    ezMeshRenderData* pRenderData = CreateRenderData(uiBatchId);
    {
      pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
      pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
      pRenderData->m_hMesh = m_hMesh;
      pRenderData->m_hMaterial = hMaterial;
      pRenderData->m_uiPartIndex = uiPartIndex;
      pRenderData->m_uiUniqueID = GetUniqueIdForRendering(uiMaterialIndex);
    }

    // Determine render data category.
    ezRenderData::Category category;
    if (msg.m_OverrideCategory != ezInvalidIndex)
    {
      category = msg.m_OverrideCategory;
    }
    else
    {
      category = ezDefaultRenderDataCategories::LitOpaque;
    }

    // Sort by material and then by mesh
    ezUInt32 uiSortingKey = (uiMaterialIDHash << 16) | (uiMeshIDHash & 0xFFFE) | uiFlipWinding;
    msg.m_pExtractedRenderData->AddRenderData(pRenderData, category, uiSortingKey);
  }
}

//////////////////////////////////////////////////////////////////////////

void ezVisualizeSkeletonComponentManager::Initialize()
{
  SUPER::Initialize();

  ezWorldModule::UpdateFunctionDesc desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezVisualizeSkeletonComponentManager::Update, this);
  desc.m_Phase = UpdateFunctionDesc::Phase::PreAsync;

  RegisterUpdateFunction(desc);

  ezResourceManager::s_ResourceEvents.AddEventHandler(ezMakeDelegate(&ezVisualizeSkeletonComponentManager::ResourceEventHandler, this));
}

void ezVisualizeSkeletonComponentManager::Deinitialize()
{
  EZ_LOCK(m_Mutex);

  ezResourceManager::s_ResourceEvents.RemoveEventHandler(ezMakeDelegate(&ezVisualizeSkeletonComponentManager::ResourceEventHandler, this));

  SUPER::Deinitialize();
}

void ezVisualizeSkeletonComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  for (const auto& hComp : m_RequireUpdate)
  {
    ezVisualizeSkeletonComponent* pComp = nullptr;
    if (!TryGetComponent(hComp, pComp))
      continue;

    pComp->CreateRenderMesh();
  }

  m_RequireUpdate.Clear();
}

void ezVisualizeSkeletonComponentManager::EnqueueUpdate(ezComponentHandle hComponent)
{
  m_RequireUpdate.PushBack(hComponent);
}

void ezVisualizeSkeletonComponentManager::ResourceEventHandler(const ezResourceEvent& e)
{
  if ((e.m_EventType == ezResourceEventType::ResourceContentUnloading ||e.m_EventType == ezResourceEventType::ResourceContentUpdated) &&
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
