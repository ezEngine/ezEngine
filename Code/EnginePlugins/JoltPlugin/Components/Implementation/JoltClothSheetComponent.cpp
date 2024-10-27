#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Interfaces/WindWorldModule.h>
#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Body/BodyLockInterface.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/SoftBody/SoftBodyCreationSettings.h>
#include <Jolt/Physics/SoftBody/SoftBodySharedSettings.h>
#include <JoltPlugin/Components/JoltClothSheetComponent.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <Physics/SoftBody/SoftBodyMotionProperties.h>
#include <RendererCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/DynamicMeshBufferResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/InstanceDataProvider.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Device/Device.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezJoltClothSheetRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezJoltClothSheetRenderer, 1, ezRTTIDefaultAllocator<ezJoltClothSheetRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

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

EZ_BEGIN_COMPONENT_TYPE(ezJoltClothSheetComponent, 2, ezComponentMode::Static)
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

void ezJoltClothSheetComponent::SetSize(ezVec2 vVal)
{
  m_vSize = vVal;
  SetupCloth();
}

void ezJoltClothSheetComponent::SetSegments(ezVec2U32 vVal)
{
  m_vSegments = vVal;
  SetupCloth();
}

void ezJoltClothSheetComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_vSize;
  s << m_vSegments;
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
  auto& s = inout_stream.GetStream();

  s >> m_vSize;
  s >> m_vSegments;
  s >> m_uiCollisionLayer;
  s >> m_fWindInfluence;
  s >> m_fGravityFactor;
  s >> m_fDamping;
  s >> m_Flags;
  s >> m_hMaterial;
  s >> m_vTextureScale;
  s >> m_Color;

  if (uiVersion >= 2)
  {
    s >> m_fThickness;
  }
}

void ezJoltClothSheetComponent::OnActivated()
{
  SUPER::OnActivated();
}

void ezJoltClothSheetComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  SetupCloth();
}

static JPH::Ref<JPH::SoftBodySharedSettings> CreateCloth(ezVec2U32 vSegments, ezVec2 vSpacing, ezBitflags<ezJoltClothSheetFlags> flags)
{
  // Create settings
  JPH::SoftBodySharedSettings* settings = new JPH::SoftBodySharedSettings;

  for (ezUInt32 y = 0; y < vSegments.y; ++y)
  {
    for (ezUInt32 x = 0; x < vSegments.x; ++x)
    {
      JPH::SoftBodySharedSettings::Vertex v;
      v.mPosition = JPH::Float3(x * vSpacing.x, y * vSpacing.y, 0.0f);
      settings->mVertices.push_back(v);
    }
  }

  // Function to get the vertex index of a point on the cloth
  auto GetIdx = [vSegments](ezUInt32 x, ezUInt32 y) -> ezUInt32
  {
    return x + y * vSegments.x;
  };

  if (flags.IsAnyFlagSet())
  {
    if (flags.IsSet(ezJoltClothSheetFlags::FixedCornerTopLeft))
    {
      settings->mVertices[GetIdx(0, 0)].mInvMass = 0.0f;
    }

    if (flags.IsSet(ezJoltClothSheetFlags::FixedCornerTopRight))
    {
      settings->mVertices[GetIdx(vSegments.x - 1, 0)].mInvMass = 0.0f;
    }

    if (flags.IsSet(ezJoltClothSheetFlags::FixedCornerBottomLeft))
    {
      settings->mVertices[GetIdx(0, vSegments.y - 1)].mInvMass = 0.0f;
    }

    if (flags.IsSet(ezJoltClothSheetFlags::FixedCornerBottomRight))
    {
      settings->mVertices[GetIdx(vSegments.x - 1, vSegments.y - 1)].mInvMass = 0.0f;
    }

    if (flags.IsSet(ezJoltClothSheetFlags::FixedEdgeTop))
    {
      for (ezUInt32 x = 0; x < vSegments.x; ++x)
      {
        settings->mVertices[GetIdx(x, 0)].mInvMass = 0.0f;
      }
    }

    if (flags.IsSet(ezJoltClothSheetFlags::FixedEdgeBottom))
    {
      for (ezUInt32 x = 0; x < vSegments.x; ++x)
      {
        settings->mVertices[GetIdx(x, vSegments.y - 1)].mInvMass = 0.0f;
      }
    }

    if (flags.IsSet(ezJoltClothSheetFlags::FixedEdgeLeft))
    {
      for (ezUInt32 y = 0; y < vSegments.y; ++y)
      {
        settings->mVertices[GetIdx(0, y)].mInvMass = 0.0f;
      }
    }

    if (flags.IsSet(ezJoltClothSheetFlags::FixedEdgeRight))
    {
      for (ezUInt32 y = 0; y < vSegments.y; ++y)
      {
        settings->mVertices[GetIdx(vSegments.x - 1, y)].mInvMass = 0.0f;
      }
    }
  }

  // Create edges
  for (ezUInt32 y = 0; y < vSegments.y; ++y)
  {
    for (ezUInt32 x = 0; x < vSegments.x; ++x)
    {
      JPH::SoftBodySharedSettings::Edge e;
      e.mCompliance = 0.00001f;
      e.mVertex[0] = GetIdx(x, y);
      if (x < vSegments.x - 1)
      {
        e.mVertex[1] = GetIdx(x + 1, y);
        settings->mEdgeConstraints.push_back(e);
      }
      if (y < vSegments.y - 1)
      {
        e.mVertex[1] = GetIdx(x, y + 1);
        settings->mEdgeConstraints.push_back(e);
      }
      if (x < vSegments.x - 1 && y < vSegments.y - 1)
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
  for (ezUInt32 y = 0; y < vSegments.y - 1; ++y)
  {
    for (ezUInt32 x = 0; x < vSegments.x - 1; ++x)
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

    JPH::Ref<JPH::SoftBodySharedSettings> settings = CreateCloth(m_vSegments, m_vSize.CompDiv(ezVec2(static_cast<float>(m_vSegments.x - 1), static_cast<float>(m_vSegments.y - 1))), m_Flags);

    settings->mVertexRadius = m_fThickness;

    ezTransform t = GetOwner()->GetGlobalTransform();

    JPH::SoftBodyCreationSettings cloth(settings, ezJoltConversionUtils::ToVec3(t.m_vPosition), ezJoltConversionUtils::ToQuat(t.m_qRotation), ezJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, ezJoltBroadphaseLayer::Cloth));

    cloth.mPressure = 0.0f;
    cloth.mLinearDamping = m_fDamping;
    cloth.mGravityFactor = m_fGravityFactor;
    // cloth.mUserData = TODO ?

    ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
    auto* pSystem = pModule->GetJoltSystem();
    auto* pBodies = &pSystem->GetBodyInterface();

    auto pBody = pBodies->CreateSoftBody(cloth);

    m_uiJoltBodyID = pBody->GetID().GetIndexAndSequenceNumber();

    pModule->QueueBodyToAdd(pBody, true);
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

void ezJoltClothSheetComponent::UpdateBodyBounds()
{
  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();
  auto* pSystem = pModule->GetJoltSystem();
  const JPH::BodyLockInterface* pLi = &pSystem->GetBodyLockInterface();

  JPH::BodyID bodyId(m_uiJoltBodyID);

  if (bodyId.IsInvalid())
    return;

  // Get write access to the body
  JPH::BodyLockRead lock(*pLi, bodyId);
  if (!lock.SucceededAndIsInBroadPhase())
    return;

  ezBoundingSphere prevBounds = m_BSphere;

  const JPH::Body& body = lock.GetBody();

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
    SetUserFlag(0, true);
  }
}

void ezJoltClothSheetComponent::OnDeactivated()
{
  RemoveBody();

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
  auto pRenderData = ezCreateRenderDataForThisFrame<ezJoltClothSheetRenderData>(GetOwner());
  pRenderData->m_uiUniqueID = GetUniqueIdForRendering();
  pRenderData->m_Color = m_Color;
  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_uiBatchId = ezHashingUtils::StringHashTo32(m_hMaterial.GetResourceIDHash());
  pRenderData->m_uiSortingKey = pRenderData->m_uiBatchId;
  pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
  pRenderData->m_hMaterial = m_hMaterial;
  pRenderData->m_vTextureScale = m_vTextureScale;

  if (!IsActiveAndSimulating())
  {
    pRenderData->m_uiVerticesX = 2;
    pRenderData->m_uiVerticesY = 2;

    pRenderData->m_Positions = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezVec3, 4);
    pRenderData->m_Indices = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezUInt16, 6);

    pRenderData->m_Positions[0] = ezVec3(0, 0, 0);
    pRenderData->m_Positions[1] = ezVec3(m_vSize.x, 0, 0);
    pRenderData->m_Positions[2] = ezVec3(0, m_vSize.y, 0);
    pRenderData->m_Positions[3] = ezVec3(m_vSize.x, m_vSize.y, 0);

    pRenderData->m_Indices[0] = 0;
    pRenderData->m_Indices[1] = 1;
    pRenderData->m_Indices[2] = 2;

    pRenderData->m_Indices[3] = 1;
    pRenderData->m_Indices[4] = 3;
    pRenderData->m_Indices[5] = 2;
  }
  else
  {
    pRenderData->m_uiVerticesX = m_vSegments.x;
    pRenderData->m_uiVerticesY = m_vSegments.y;

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

    pRenderData->m_Positions = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezVec3, pRenderData->m_uiVerticesX * pRenderData->m_uiVerticesY);
    pRenderData->m_Indices = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezUInt16, (pRenderData->m_uiVerticesX - 1) * (pRenderData->m_uiVerticesY - 1) * 2 * 3);

    const JPH::Body& body = lock.GetBody();
    const JPH::SoftBodyMotionProperties* pMotion = static_cast<const JPH::SoftBodyMotionProperties*>(body.GetMotionProperties());

    const auto& transformed_shape = body.GetTransformedShape();

    // Vec3 scale = transformed_shape.GetShapeScale();
    // RMat44 matrix = transformed_shape.GetCenterOfMassTransform().PreScaled(scale);

    pRenderData->m_GlobalTransform.m_vPosition = ezJoltConversionUtils::ToVec3(transformed_shape.GetCenterOfMassTransform().GetTranslation());
    pRenderData->m_GlobalTransform.m_qRotation = ezJoltConversionUtils::ToQuat(transformed_shape.GetCenterOfMassTransform().GetRotation().GetQuaternion());

    const JPH::Array<JPH::SoftBodyMotionProperties::Vertex>& particles = pMotion->GetVertices();

    // copy over the vertex positions
    {
      ezUInt32 vidx = 0;
      for (ezUInt32 y = 0; y < pRenderData->m_uiVerticesY; ++y)
      {
        for (ezUInt32 x = 0; x < pRenderData->m_uiVerticesX; ++x, ++vidx)
        {
          pRenderData->m_Positions[vidx] = ezJoltConversionUtils::ToVec3(particles[vidx].mPosition);
        }
      }
    }

    // create the triangle indices
    {
      ezUInt32 tidx = 0;
      ezUInt16 vidx = 0;
      for (ezUInt16 y = 0; y < pRenderData->m_uiVerticesY - 1; ++y)
      {
        for (ezUInt16 x = 0; x < pRenderData->m_uiVerticesX - 1; ++x, ++vidx)
        {
          pRenderData->m_Indices[tidx++] = vidx;
          pRenderData->m_Indices[tidx++] = vidx + 1;
          pRenderData->m_Indices[tidx++] = vidx + pRenderData->m_uiVerticesX;

          pRenderData->m_Indices[tidx++] = vidx + 1;
          pRenderData->m_Indices[tidx++] = vidx + pRenderData->m_uiVerticesX + 1;
          pRenderData->m_Indices[tidx++] = vidx + pRenderData->m_uiVerticesX;
        }

        ++vidx;
      }
    }
  }

  ezRenderData::Category category = m_RenderDataCategory;

  if (!category.IsValid())
  {
    category = ezDefaultRenderDataCategories::LitOpaque; // use as default fallback

    if (m_hMaterial.IsValid())
    {
      ezResourceLock<ezMaterialResource> pMaterial(m_hMaterial, ezResourceAcquireMode::AllowLoadingFallback);

      if (pMaterial.GetAcquireResult() != ezResourceAcquireResult::LoadingFallback)
      {
        // if this is the final result, cache it
        m_RenderDataCategory = pMaterial->GetRenderDataCategory();
      }

      category = pMaterial->GetRenderDataCategory();
    }
  }

  msg.AddRenderData(pRenderData, category, ezRenderData::Caching::Never);
}

void ezJoltClothSheetComponent::SetFlags(ezBitflags<ezJoltClothSheetFlags> flags)
{
  m_Flags = flags;
  SetupCloth();
}

void ezJoltClothSheetComponent::Update()
{
  if (!IsActiveAndSimulating())
    return;

  // TODO: only do this every once in a while
  UpdateBodyBounds();

  if (GetOwner()->GetVisibilityState(60) == ezVisibilityState::Direct)
  {
    // only apply wind to directly visible pieces of cloth
    ApplyWind();
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

ezJoltClothSheetRenderer::ezJoltClothSheetRenderer()
{
  CreateVertexBuffer();
}

ezJoltClothSheetRenderer::~ezJoltClothSheetRenderer() = default;

void ezJoltClothSheetRenderer::GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(ezDefaultRenderDataCategories::LitOpaque);
  ref_categories.PushBack(ezDefaultRenderDataCategories::LitMasked);
  ref_categories.PushBack(ezDefaultRenderDataCategories::LitTransparent);
  ref_categories.PushBack(ezDefaultRenderDataCategories::Selection);
}

void ezJoltClothSheetRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(ezGetStaticRTTI<ezJoltClothSheetRenderData>());
}

void ezJoltClothSheetRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const
{
  const bool bNeedsNormals = (renderViewContext.m_pViewData->m_CameraUsageHint != ezCameraUsageHint::Shadow);


  ezRenderContext* pRenderContext = renderViewContext.m_pRenderContext;
  ezGALCommandEncoder* pGALCommandEncoder = pRenderContext->GetCommandEncoder();

  ezInstanceData* pInstanceData = pPass->GetPipeline()->GetFrameDataProvider<ezInstanceDataProvider>()->GetData(renderViewContext);
  pInstanceData->BindResources(pRenderContext);

  pRenderContext->SetShaderPermutationVariable("FLIP_WINDING", "FALSE");
  pRenderContext->SetShaderPermutationVariable("VERTEX_SKINNING", "FALSE");

  ezResourceLock<ezDynamicMeshBufferResource> pBuffer(m_hDynamicMeshBuffer, ezResourceAcquireMode::BlockTillLoaded);

  for (auto it = batch.GetIterator<ezJoltClothSheetRenderData>(0, batch.GetCount()); it.IsValid(); ++it)
  {
    const ezJoltClothSheetRenderData* pRenderData = it;

    EZ_ASSERT_DEV(pRenderData->m_uiVerticesX > 1 && pRenderData->m_uiVerticesY > 1, "Invalid cloth render data");

    pRenderContext->BindMaterial(pRenderData->m_hMaterial);

    ezUInt32 uiInstanceDataOffset = 0;
    ezArrayPtr<ezPerInstanceData> instanceData = pInstanceData->GetInstanceData(pRenderContext, 1, uiInstanceDataOffset);

    instanceData[0].ObjectToWorld = pRenderData->m_GlobalTransform;
    instanceData[0].ObjectToWorldNormal = instanceData[0].ObjectToWorld;
    instanceData[0].GameObjectID = pRenderData->m_uiUniqueID;
    instanceData[0].Color = pRenderData->m_Color;
    instanceData[0].CustomData.SetZero(); // unused

    pInstanceData->UpdateInstanceData(pRenderContext, 1);

    {
      auto pVertexData = pBuffer->AccessVertexData();
      auto pIndexData = pBuffer->AccessIndex16Data();

      const float fDivU = 1.0f / (pRenderData->m_uiVerticesX - 1);
      const float fDivY = 1.0f / (pRenderData->m_uiVerticesY - 1);

      const ezUInt16 width = pRenderData->m_uiVerticesX;

      if (bNeedsNormals)
      {
        const ezUInt16 widthM1 = width - 1;
        const ezUInt16 heightM1 = pRenderData->m_uiVerticesY - 1;

        ezUInt16 topIdx = 0;

        ezUInt32 vidx = 0;
        for (ezUInt16 y = 0; y < pRenderData->m_uiVerticesY; ++y)
        {
          ezUInt16 leftIdx = 0;
          const ezUInt16 bottomIdx = ezMath::Min<ezUInt16>(y + 1, heightM1);

          const ezUInt32 yOff = y * width;
          const ezUInt32 yOffTop = topIdx * width;
          const ezUInt32 yOffBottom = bottomIdx * width;

          for (ezUInt16 x = 0; x < width; ++x, ++vidx)
          {
            const ezUInt16 rightIdx = ezMath::Min<ezUInt16>(x + 1, widthM1);

            const ezVec3 leftPos = pRenderData->m_Positions[yOff + leftIdx];
            const ezVec3 rightPos = pRenderData->m_Positions[yOff + rightIdx];
            const ezVec3 topPos = pRenderData->m_Positions[yOffTop + x];
            const ezVec3 bottomPos = pRenderData->m_Positions[yOffBottom + x];

            const ezVec3 leftToRight = rightPos - leftPos;
            const ezVec3 bottomToTop = topPos - bottomPos;
            ezVec3 normal = -leftToRight.CrossRH(bottomToTop);
            normal.NormalizeIfNotZero(ezVec3(0, 0, 1)).IgnoreResult();

            ezVec3 tangent = leftToRight;
            tangent.NormalizeIfNotZero(ezVec3(1, 0, 0)).IgnoreResult();

            pVertexData[vidx].m_vPosition = pRenderData->m_Positions[vidx];
            pVertexData[vidx].m_vTexCoord = ezVec2(x * fDivU, y * fDivY).CompMul(pRenderData->m_vTextureScale);
            pVertexData[vidx].EncodeNormal(normal);
            pVertexData[vidx].EncodeTangent(tangent, 1.0f);

            leftIdx = x;
          }

          topIdx = y;
        }
      }
      else
      {
        ezUInt32 vidx = 0;
        for (ezUInt16 y = 0; y < pRenderData->m_uiVerticesY; ++y)
        {
          for (ezUInt16 x = 0; x < width; ++x, ++vidx)
          {
            pVertexData[vidx].m_vPosition = pRenderData->m_Positions[vidx];
            pVertexData[vidx].m_vTexCoord = ezVec2(x * fDivU, y * fDivY).CompMul(pRenderData->m_vTextureScale);
            pVertexData[vidx].EncodeNormal(ezVec3::MakeAxisZ());
            pVertexData[vidx].EncodeTangent(ezVec3::MakeAxisX(), 1.0f);
          }
        }
      }

      ezMemoryUtils::Copy<ezUInt16>(pIndexData.GetPtr(), pRenderData->m_Indices.GetPtr(), pRenderData->m_Indices.GetCount());
    }

    const ezUInt32 uiNumPrimitives = (pRenderData->m_uiVerticesX - 1) * (pRenderData->m_uiVerticesY - 1) * 2;

    pBuffer->UpdateGpuBuffer(pGALCommandEncoder, 0, pRenderData->m_uiVerticesX * pRenderData->m_uiVerticesY);

    // redo this after the primitive count has changed
    pRenderContext->BindMeshBuffer(m_hDynamicMeshBuffer);

    renderViewContext.m_pRenderContext->DrawMeshBuffer(uiNumPrimitives).IgnoreResult();
  }
}

void ezJoltClothSheetRenderer::CreateVertexBuffer()
{
  if (m_hDynamicMeshBuffer.IsValid())
    return;

  m_hDynamicMeshBuffer = ezResourceManager::GetExistingResource<ezDynamicMeshBufferResource>("JoltClothSheet");

  if (!m_hDynamicMeshBuffer.IsValid())
  {
    const ezUInt32 uiMaxVerts = 64;

    ezDynamicMeshBufferResourceDescriptor desc;
    desc.m_uiMaxVertices = uiMaxVerts * uiMaxVerts;
    desc.m_IndexType = ezGALIndexType::UShort;
    desc.m_uiMaxPrimitives = ezMath::Square(uiMaxVerts - 1) * 2;

    m_hDynamicMeshBuffer = ezResourceManager::GetOrCreateResource<ezDynamicMeshBufferResource>("JoltClothSheet", std::move(desc), "Jolt Cloth Sheet Buffer");
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
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
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezJoltClothSheetComponentManager::Update, this);
    desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezJoltClothSheetComponentManager::UpdateBounds, this);
    desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }
}

void ezJoltClothSheetComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized())
    {
      it->Update();
    }
  }
}

void ezJoltClothSheetComponentManager::UpdateBounds(const ezWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized() && it->GetUserFlag(0))
    {
      it->TriggerLocalBoundsUpdate();

      // reset update bounds flag
      it->SetUserFlag(0, false);
    }
  }
}


EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltClothSheetComponent);
