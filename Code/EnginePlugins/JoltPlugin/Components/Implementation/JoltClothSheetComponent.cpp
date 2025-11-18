#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/SoftBody/SoftBodyCreationSettings.h>
#include <Jolt/Physics/SoftBody/SoftBodyMotionProperties.h>
#include <JoltPlugin/Components/JoltClothSheetComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/CustomMeshComponent.h>
#include <RendererCore/Meshes/DynamicMeshBufferResource.h>
#include <RendererCore/Pipeline/RenderDataManager.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezJoltClothSheetFlags, 1)
  EZ_ENUM_CONSTANT(ezJoltClothSheetFlags::FixedCornerTopLeft),
  EZ_ENUM_CONSTANT(ezJoltClothSheetFlags::FixedCornerTopRight),
  EZ_ENUM_CONSTANT(ezJoltClothSheetFlags::FixedCornerBottomRight),
  EZ_ENUM_CONSTANT(ezJoltClothSheetFlags::FixedCornerBottomLeft),
  EZ_ENUM_CONSTANT(ezJoltClothSheetFlags::FixedEdgeTop),
  EZ_ENUM_CONSTANT(ezJoltClothSheetFlags::FixedEdgeRight),
  EZ_ENUM_CONSTANT(ezJoltClothSheetFlags::FixedEdgeBottom),
  EZ_ENUM_CONSTANT(ezJoltClothSheetFlags::FixedEdgeLeft),
EZ_END_STATIC_REFLECTED_BITFLAGS;

EZ_BEGIN_COMPONENT_TYPE(ezJoltClothSheetComponent, 4, ezComponentMode::Static)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new ezDefaultValueAttribute(ezVec2(0.5f, 0.5f))),
      EZ_ACCESSOR_PROPERTY("Segments", GetSegments, SetSegments)->AddAttributes(new ezDefaultValueAttribute(ezVec2U32(16, 16)), new ezClampValueAttribute(ezVec2U32(2, 2), ezVec2U32(64, 64))),
      EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
      EZ_MEMBER_PROPERTY("WindInfluence", m_fWindInfluence)->AddAttributes(new ezDefaultValueAttribute(0.3f), new ezClampValueAttribute(0.0f, 10.0f)),
      EZ_MEMBER_PROPERTY("GravityFactor", m_fGravityFactor)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
      EZ_MEMBER_PROPERTY("Damping", m_fDamping)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
      EZ_MEMBER_PROPERTY("Thickness", m_fThickness)->AddAttributes(new ezDefaultValueAttribute(0.05f), new ezClampValueAttribute(0.0f, 0.5f)),
      EZ_BITFLAGS_ACCESSOR_PROPERTY("Flags", ezJoltClothSheetFlags, GetFlags, SetFlags),
      EZ_RESOURCE_MEMBER_PROPERTY("Material", m_hMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material")),
      EZ_MEMBER_PROPERTY("TextureScale", m_vTextureScale)->AddAttributes(new ezDefaultValueAttribute(ezVec2(1.0f))),
      EZ_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new ezDefaultValueAttribute(ezColor::White)),
    }
    EZ_END_PROPERTIES;
    EZ_BEGIN_ATTRIBUTES
    {
      new ezCategoryAttribute("Physics/Jolt/Effects"),
    }
    EZ_END_ATTRIBUTES;
    EZ_BEGIN_MESSAGEHANDLERS
    {
      EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
    }
    EZ_END_MESSAGEHANDLERS;
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezJoltClothSheetComponent::ezJoltClothSheetComponent() = default;
ezJoltClothSheetComponent::~ezJoltClothSheetComponent() = default;

void ezJoltClothSheetComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_vSize;
  s << m_vNumVertices;
  s << m_uiCollisionLayer;
  s << m_fWindInfluence;
  s << m_fGravityFactor;
  s << m_fDamping;
  s << m_Flags;
  s << m_hMaterial;
  s << m_vTextureScale;
  s << m_Color;
  s << m_fThickness;
}

void ezJoltClothSheetComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  EZ_ASSERT_DEBUG(uiVersion >= 4, "Outdated version, please re-transform asset.");
  if (uiVersion < 4)
    return;

  auto& s = inout_stream.GetStream();

  s >> m_vSize;
  s >> m_vNumVertices;
  s >> m_uiCollisionLayer;
  s >> m_fWindInfluence;
  s >> m_fGravityFactor;
  s >> m_fDamping;
  s >> m_Flags;
  s >> m_hMaterial;
  s >> m_vTextureScale;
  s >> m_Color;
  s >> m_fThickness;
}

void ezJoltClothSheetComponent::OnActivated()
{
  SUPER::OnActivated();

  SetupCloth();
}

void ezJoltClothSheetComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_uiObjectFilterID == ezInvalidIndex)
  {
    // only create a new filter ID, if none has been passed in manually

    ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
    m_uiObjectFilterID = pModule->CreateObjectFilterID();
  }

  SetupCloth();

  m_BodyGlobalTransform = GetOwner()->GetGlobalTransform();
}

void ezJoltClothSheetComponent::OnDeactivated()
{
  ezRenderDataManager* pRenderDataManager = GetWorld()->GetModule<ezRenderDataManager>();
  pRenderDataManager->DeleteInstanceData(m_InstanceDataOffset);

  RemoveBody();

  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();
  pModule->DeallocateUserData(m_uiUserDataIndex);

  pModule->DeleteObjectFilterID(m_uiObjectFilterID);

  SUPER::OnDeactivated();
}

ezResult ezJoltClothSheetComponent::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  if (m_BSphere.IsValid())
  {
    ref_bounds.ExpandToInclude(ezBoundingBoxSphere::MakeFromSphere(m_BSphere));
  }
  else
  {
    ezBoundingBox box = ezBoundingBox::MakeInvalid();
    box.ExpandToInclude(ezVec3::MakeZero());
    box.ExpandToInclude(ezVec3(0, 0, -0.1f));
    box.ExpandToInclude(ezVec3(m_vSize.x, m_vSize.y, 0.1f));

    ref_bounds.ExpandToInclude(ezBoundingBoxSphere::MakeFromBox(box));
  }

  return EZ_SUCCESS;
}

void ezJoltClothSheetComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!m_hDynamicMeshBuffer.IsValid())
    return;

  // Force dynamic instance data buffer since the render data is not cached, so we would trash the static instance data buffer every frame.
  const bool bDynamic = true;
  const ezTransform globalTransform = IsActiveAndSimulating() ? m_BodyGlobalTransform : GetOwner()->GetGlobalTransform();
  auto hInstanceDataBuffer = msg.m_pRenderDataManager->GetOrCreateInstanceDataAndFill(*this, bDynamic, globalTransform, m_InstanceDataOffset, GetUniqueIdForRendering(), m_Color);

  ezCustomMeshRenderData* pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<ezCustomMeshRenderData>(GetOwner());
  {
    pRenderData->m_uiNumInstances = 1;
    pRenderData->m_DataOffsets.m_uiInstance = m_InstanceDataOffset.m_uiOffset;
    pRenderData->m_hInstanceDataBuffer = hInstanceDataBuffer;
    pRenderData->m_fSortingDepthOffset = 0.0f;

    pRenderData->m_hMaterial = m_hMaterial;
    pRenderData->m_hDynamicMeshBuffer = m_hDynamicMeshBuffer;
    pRenderData->m_uiFirstPrimitive = 0;
    pRenderData->m_uiNumPrimitives = (m_vNumVertices.x - 1) * (m_vNumVertices.y - 1) * 2;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    pRenderData->m_FallbackGlobalBBox = GetOwner()->GetGlobalBounds().GetBox();
#endif

    pRenderData->FillSortingKey();
  }

  ezRenderData::Category category = m_RenderDataCategory;
  if (!category.IsValid())
  {
    category = ezDefaultRenderDataCategories::LitOpaque; // use as default fallback

    if (m_hMaterial.IsValid())
    {
      ezResourceLock<ezMaterialResource> pMaterial(m_hMaterial, ezResourceAcquireMode::AllowLoadingFallback);
      category = pMaterial->GetRenderDataCategory();

      if (pMaterial.GetAcquireResult() != ezResourceAcquireResult::LoadingFallback)
      {
        // if this is the final result, cache it
        m_RenderDataCategory = category;
      }
    }
  }

  msg.AddRenderData(pRenderData, category, ezRenderData::Caching::Never);
}

void ezJoltClothSheetComponent::SetSize(ezVec2 vVal)
{
  m_vSize = vVal;
  SetupCloth();
}

void ezJoltClothSheetComponent::SetSegments(ezVec2U32 vVal)
{
  m_vNumVertices = vVal;
  SetupCloth();
}

void ezJoltClothSheetComponent::SetFlags(ezBitflags<ezJoltClothSheetFlags> flags)
{
  m_Flags = flags;
  SetupCloth();
}

void ezJoltClothSheetComponent::UpdatePreAsync()
{
  if (GetOwner()->GetVisibilityState(60) == ezVisibilityState::Direct)
  {
    // only apply wind to directly visible pieces of cloth
    ApplyWind();
  }
}

void ezJoltClothSheetComponent::UpdatePostAsync()
{
  const ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();
  auto* pSystem = pModule->GetJoltSystem();
  const JPH::BodyLockInterface* pLi = &pSystem->GetBodyLockInterface();

  JPH::BodyID bodyId(m_uiJoltBodyID);

  if (bodyId.IsInvalid())
    return;

  // Get the body
  JPH::BodyLockRead lock(*pLi, bodyId);
  if (!lock.SucceededAndIsInBroadPhase())
    return;

  const JPH::Body& body = lock.GetBody();

  {
    ezBoundingSphere prevBounds = m_BSphere;

    // TODO: should rather iterate over all active (soft) bodies, than to check this here
    if (!body.IsActive())
      return;

    const JPH::AABox box = body.GetWorldSpaceBounds();

    const ezTransform t = GetOwner()->GetGlobalTransform().GetInverse();

    m_BSphere.m_vCenter = t.TransformPosition(ezJoltConversionUtils::ToVec3(box.GetCenter()));

    const ezVec3 ext = ezJoltConversionUtils::ToVec3(box.GetExtent());
    m_BSphere.m_fRadius = ezMath::Max(ext.x, ext.y, ext.z);

    if (prevBounds != m_BSphere)
    {
      TriggerLocalBoundsUpdate();
    }
  }

  // Don't update mesh and transform when invisible
  if (GetOwner()->GetVisibilityState() == ezVisibilityState::Invisible)
    return;

  {
    const JPH::SoftBodyMotionProperties* pMotion = static_cast<const JPH::SoftBodyMotionProperties*>(body.GetMotionProperties());
    const JPH::Array<JPH::SoftBodyMotionProperties::Vertex>& particles = pMotion->GetVertices();

    ezResourceLock<ezDynamicMeshBufferResource> pDynamicMeshBuffer(m_hDynamicMeshBuffer, ezResourceAcquireMode::BlockTillLoaded);
    auto positions = pDynamicMeshBuffer->AccessPositionData();

    const ezVec2U32 vNumVertices = m_vNumVertices;

    ezUInt32 vidx = 0;
    for (ezUInt32 y = 0; y < vNumVertices.y; ++y)
    {
      for (ezUInt32 x = 0; x < vNumVertices.x; ++x, ++vidx)
      {
        positions[vidx] = ezJoltConversionUtils::ToVec3(particles[vidx].mPosition);
      }
    }

    ezDynamicMeshBufferResource::CalculateGridNormalAndTangents(pDynamicMeshBuffer.GetPointerNonConst(), vNumVertices);
  }

  {
    m_BodyGlobalTransform = GetOwner()->GetGlobalTransform();

    const auto& transformed_shape = body.GetTransformedShape();
    JPH::RMat44 matrix = transformed_shape.GetCenterOfMassTransform();

    m_BodyGlobalTransform.m_vPosition = ezJoltConversionUtils::ToVec3(matrix.GetTranslation());
    m_BodyGlobalTransform.m_qRotation = ezJoltConversionUtils::ToQuat(matrix.GetRotation().GetQuaternion());
  }
}


void ezJoltClothSheetComponent::ApplyWind()
{
  if (m_fWindInfluence <= 0.0f)
    return;

  if (!m_BSphere.IsValid())
    return;

  if (const ezWindWorldModuleInterface* pWind = GetWorld()->GetModuleReadOnly<ezWindWorldModuleInterface>())
  {
    const ezVec3 vSamplePos = GetOwner()->GetGlobalTransform().TransformPosition(m_BSphere.m_vCenter);

    const ezVec3 vWind = pWind->GetWindAt(vSamplePos) * m_fWindInfluence;

    if (!vWind.IsZero())
    {
      ezVec3 windForce = vWind;
      windForce += pWind->ComputeWindFlutter(vWind, vWind.GetOrthogonalVector(), 5.0f, GetOwner()->GetStableRandomSeed());

      JPH::Vec3 windVel = ezJoltConversionUtils::ToVec3(windForce);

      ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();
      auto* pSystem = pModule->GetJoltSystem();
      const JPH::BodyLockInterface* pLi = &pSystem->GetBodyLockInterface();

      JPH::BodyID bodyId(m_uiJoltBodyID);

      if (bodyId.IsInvalid())
        return;

      // Get write access to the body
      JPH::BodyLockWrite lock(*pLi, bodyId);
      if (!lock.SucceededAndIsInBroadPhase())
        return;

      JPH::Body& body = lock.GetBody();
      JPH::SoftBodyMotionProperties* pMotion = static_cast<JPH::SoftBodyMotionProperties*>(body.GetMotionProperties());

      if (!body.IsActive())
      {
        pSystem->GetBodyInterfaceNoLock().ActivateBody(bodyId);
      }

      JPH::Array<JPH::SoftBodyMotionProperties::Vertex>& particles = pMotion->GetVertices();

      // randomize which vertices get the wind velocity applied,
      // both to save performance and also to introduce a nice ripple effect
      const ezUInt32 uiStart = GetWorld()->GetRandomNumberGenerator().UIntInRange(ezMath::Min<ezUInt32>(16u, (ezUInt32)particles.size()));
      const ezUInt32 uiStep = GetWorld()->GetRandomNumberGenerator().IntMinMax(16, 16 + 32);

      for (ezUInt32 i = uiStart; i < particles.size(); i += uiStep)
      {
        if (particles[i].mInvMass > 0)
        {
          particles[i].mVelocity = windVel;
        }
      }
    }
  }
}

static JPH::Ref<JPH::SoftBodySharedSettings> CreateCloth(ezVec2U32 vNumVertices, ezVec2 vSpacing, ezBitflags<ezJoltClothSheetFlags> flags, float fPerVertexMass)
{
  // Create settings
  JPH::SoftBodySharedSettings* settings = new JPH::SoftBodySharedSettings;

  const float fInvVtxMass = 1.0f / fPerVertexMass;

  for (ezUInt32 y = 0; y < vNumVertices.y; ++y)
  {
    for (ezUInt32 x = 0; x < vNumVertices.x; ++x)
    {
      JPH::SoftBodySharedSettings::Vertex v;
      v.mPosition = JPH::Float3(x * vSpacing.x, y * vSpacing.y, 0.0f);
      v.mInvMass = fInvVtxMass;
      settings->mVertices.push_back(v);
    }
  }

  // Function to get the vertex index of a point on the cloth
  auto GetIdx = [vNumVertices](ezUInt32 x, ezUInt32 y) -> ezUInt32
  {
    return x + y * vNumVertices.x;
  };

  if (flags.IsAnyFlagSet())
  {
    if (flags.IsSet(ezJoltClothSheetFlags::FixedCornerTopLeft))
    {
      settings->mVertices[GetIdx(0, 0)].mInvMass = 0.0f;
    }

    if (flags.IsSet(ezJoltClothSheetFlags::FixedCornerTopRight))
    {
      settings->mVertices[GetIdx(vNumVertices.x - 1, 0)].mInvMass = 0.0f;
    }

    if (flags.IsSet(ezJoltClothSheetFlags::FixedCornerBottomLeft))
    {
      settings->mVertices[GetIdx(0, vNumVertices.y - 1)].mInvMass = 0.0f;
    }

    if (flags.IsSet(ezJoltClothSheetFlags::FixedCornerBottomRight))
    {
      settings->mVertices[GetIdx(vNumVertices.x - 1, vNumVertices.y - 1)].mInvMass = 0.0f;
    }

    if (flags.IsSet(ezJoltClothSheetFlags::FixedEdgeTop))
    {
      for (ezUInt32 x = 0; x < vNumVertices.x; ++x)
      {
        settings->mVertices[GetIdx(x, 0)].mInvMass = 0.0f;
      }
    }

    if (flags.IsSet(ezJoltClothSheetFlags::FixedEdgeBottom))
    {
      for (ezUInt32 x = 0; x < vNumVertices.x; ++x)
      {
        settings->mVertices[GetIdx(x, vNumVertices.y - 1)].mInvMass = 0.0f;
      }
    }

    if (flags.IsSet(ezJoltClothSheetFlags::FixedEdgeLeft))
    {
      for (ezUInt32 y = 0; y < vNumVertices.y; ++y)
      {
        settings->mVertices[GetIdx(0, y)].mInvMass = 0.0f;
      }
    }

    if (flags.IsSet(ezJoltClothSheetFlags::FixedEdgeRight))
    {
      for (ezUInt32 y = 0; y < vNumVertices.y; ++y)
      {
        settings->mVertices[GetIdx(vNumVertices.x - 1, y)].mInvMass = 0.0f;
      }
    }
  }

  // Create edges
  for (ezUInt32 y = 0; y < vNumVertices.y; ++y)
  {
    for (ezUInt32 x = 0; x < vNumVertices.x; ++x)
    {
      JPH::SoftBodySharedSettings::Edge e;
      e.mCompliance = 0.00001f;
      e.mVertex[0] = GetIdx(x, y);
      if (x < vNumVertices.x - 1)
      {
        e.mVertex[1] = GetIdx(x + 1, y);
        settings->mEdgeConstraints.push_back(e);
      }
      if (y < vNumVertices.y - 1)
      {
        e.mVertex[1] = GetIdx(x, y + 1);
        settings->mEdgeConstraints.push_back(e);
      }
      if (x < vNumVertices.x - 1 && y < vNumVertices.y - 1)
      {
        e.mVertex[1] = GetIdx(x + 1, y + 1);
        settings->mEdgeConstraints.push_back(e);

        e.mVertex[0] = GetIdx(x + 1, y);
        e.mVertex[1] = GetIdx(x, y + 1);
        settings->mEdgeConstraints.push_back(e);
      }
    }
  }

  settings->CalculateEdgeLengths();

  // Create faces
  for (ezUInt32 y = 0; y < vNumVertices.y - 1; ++y)
  {
    for (ezUInt32 x = 0; x < vNumVertices.x - 1; ++x)
    {
      JPH::SoftBodySharedSettings::Face f;
      f.mVertex[0] = GetIdx(x, y);
      f.mVertex[1] = GetIdx(x, y + 1);
      f.mVertex[2] = GetIdx(x + 1, y + 1);
      settings->AddFace(f);

      f.mVertex[1] = GetIdx(x + 1, y + 1);
      f.mVertex[2] = GetIdx(x + 1, y);
      settings->AddFace(f);
    }
  }

  settings->Optimize();

  return settings;
}


void ezJoltClothSheetComponent::SetupCloth()
{
  m_BSphere = ezBoundingSphere::MakeInvalid();

  if (IsActiveAndSimulating())
  {
    RemoveBody();

    float fPerVertexMass = 1.0f; // default value

    ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
    auto* pSystem = pModule->GetJoltSystem();
    auto* pBodies = &pSystem->GetBodyInterface();

    JPH::Ref<JPH::SoftBodySharedSettings> settings = CreateCloth(m_vNumVertices, m_vSize.CompDiv(ezVec2(static_cast<float>(m_vNumVertices.x - 1), static_cast<float>(m_vNumVertices.y - 1))), m_Flags, fPerVertexMass);

    settings->mVertexRadius = m_fThickness;

    ezTransform t = GetOwner()->GetGlobalTransform();

    ezJoltUserData* pUserData = nullptr;
    m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
    pUserData->Init(this);

    JPH::SoftBodyCreationSettings cloth(settings, ezJoltConversionUtils::ToVec3(t.m_vPosition), ezJoltConversionUtils::ToQuat(t.m_qRotation), ezJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, ezJoltBroadphaseLayer::Cloth));

    cloth.mPressure = 0.0f;
    cloth.mLinearDamping = m_fDamping;
    cloth.mGravityFactor = m_fGravityFactor;
    cloth.mUserData = reinterpret_cast<ezUInt64>(pUserData);
    cloth.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
    // cloth.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter()); // the group filter is only needed for objects constrained via joints

    auto pBody = pBodies->CreateSoftBody(cloth);

    m_uiJoltBodyID = pBody->GetID().GetIndexAndSequenceNumber();

    pModule->QueueBodyToAdd(pBody, true);
  }

  if (IsActiveAndInitialized() && m_vSize.x > 0 && m_vSize.y > 0 && m_vNumVertices.x > 1 && m_vNumVertices.y > 1)
  {
    ezStringBuilder sResourceName;
    sResourceName.SetFormat("JoltClothSheet_{}_{}x{}_{}x{}_{}x{}", ezArgP(this), m_vSize.x, m_vSize.y, m_vNumVertices.x, m_vNumVertices.y, m_vTextureScale.x, m_vTextureScale.y);

    m_hDynamicMeshBuffer = ezResourceManager::GetExistingResource<ezDynamicMeshBufferResource>(sResourceName);

    if (!m_hDynamicMeshBuffer.IsValid())
    {
      ezDynamicMeshBufferResourceDescriptor desc;
      desc.m_uiMaxVertices = m_vNumVertices.x * m_vNumVertices.y;
      desc.m_IndexType = ezGALIndexType::UShort;
      desc.m_uiMaxPrimitives = (m_vNumVertices.x - 1) * (m_vNumVertices.y - 1) * 2;

      m_hDynamicMeshBuffer = ezResourceManager::GetOrCreateResource<ezDynamicMeshBufferResource>(sResourceName, std::move(desc));
    }

    ezResourceLock<ezDynamicMeshBufferResource> pDynamicMeshBuffer(m_hDynamicMeshBuffer, ezResourceAcquireMode::BlockTillLoaded);
    ezDynamicMeshBufferResource::CreateGridXY(pDynamicMeshBuffer.GetPointerNonConst(), m_vSize, m_vNumVertices);
  }

  TriggerLocalBoundsUpdate();
}

void ezJoltClothSheetComponent::RemoveBody()
{
  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();

  JPH::BodyID bodyId(m_uiJoltBodyID);

  if (!bodyId.IsInvalid())
  {
    auto* pSystem = pModule->GetJoltSystem();
    auto* pBodies = &pSystem->GetBodyInterface();

    if (pBodies->IsAdded(bodyId))
    {
      pBodies->RemoveBody(bodyId);
    }

    pBodies->DestroyBody(bodyId);
    m_uiJoltBodyID = JPH::BodyID::cInvalidBodyID;
  }

  // TODO: currently not yet needed
  // pModule->DeallocateUserData(m_uiUserDataIndex);
  // pModule->DeleteObjectFilterID(m_uiObjectFilterID);
}

//////////////////////////////////////////////////////////////////////////

ezJoltClothSheetComponentManager::ezJoltClothSheetComponentManager(ezWorld* pWorld)
  : ezComponentManager(pWorld)
{
}

ezJoltClothSheetComponentManager::~ezJoltClothSheetComponentManager() = default;

void ezJoltClothSheetComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezJoltClothSheetComponentManager::UpdatePreAsync, this);
    desc.m_Phase = ezWorldUpdatePhase::PreAsync;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezJoltClothSheetComponentManager::UpdatePostAsync, this);
    desc.m_Phase = ezWorldUpdatePhase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }
}

void ezJoltClothSheetComponentManager::UpdatePreAsync(const ezWorldModule::UpdateContext& context)
{
  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();
  if (pModule == nullptr || pModule->GetJoltUpdateCounter() == m_uiLastJoltUpdateCounter)
  {
    // skip cloth updates, when there was no Jolt update yet
    return;
  }

  m_uiLastJoltUpdateCounter = pModule->GetJoltUpdateCounter();

  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized())
    {
      it->UpdatePreAsync();
    }
  }
}

void ezJoltClothSheetComponentManager::UpdatePostAsync(const ezWorldModule::UpdateContext& context)
{
  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();
  if (pModule == nullptr || pModule->GetJoltUpdateCounter() == m_uiLastJoltUpdateCounter)
  {
    // skip cloth updates, when there was no Jolt update yet
    return;
  }

  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized())
    {
      it->UpdatePostAsync();
    }
  }
}


EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltClothSheetComponent);
