#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Interfaces/WindWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Physics/ClothSheetComponent.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/CustomMeshComponent.h>
#include <RendererCore/Meshes/DynamicMeshBufferResource.h>
#include <RendererCore/Pipeline/RenderDataManager.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezClothSheetFlags, 1)
  EZ_ENUM_CONSTANT(ezClothSheetFlags::FixedCornerTopLeft),
  EZ_ENUM_CONSTANT(ezClothSheetFlags::FixedCornerTopRight),
  EZ_ENUM_CONSTANT(ezClothSheetFlags::FixedCornerBottomRight),
  EZ_ENUM_CONSTANT(ezClothSheetFlags::FixedCornerBottomLeft),
  EZ_ENUM_CONSTANT(ezClothSheetFlags::FixedEdgeTop),
  EZ_ENUM_CONSTANT(ezClothSheetFlags::FixedEdgeRight),
  EZ_ENUM_CONSTANT(ezClothSheetFlags::FixedEdgeBottom),
  EZ_ENUM_CONSTANT(ezClothSheetFlags::FixedEdgeLeft),
EZ_END_STATIC_REFLECTED_BITFLAGS;

EZ_BEGIN_COMPONENT_TYPE(ezClothSheetComponent, 1, ezComponentMode::Static)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new ezDefaultValueAttribute(ezVec2(0.5f, 0.5f))),
      EZ_ACCESSOR_PROPERTY("Slack", GetSlack, SetSlack)->AddAttributes(new ezDefaultValueAttribute(ezVec2(0.0f, 0.0f))),
      EZ_ACCESSOR_PROPERTY("Segments", GetSegments, SetSegments)->AddAttributes(new ezDefaultValueAttribute(ezVec2U32(7, 7)), new ezClampValueAttribute(ezVec2U32(1, 1), ezVec2U32(31, 31))),
      EZ_MEMBER_PROPERTY("Damping", m_fDamping)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
      EZ_MEMBER_PROPERTY("WindInfluence", m_fWindInfluence)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 10.0f)),
      EZ_BITFLAGS_ACCESSOR_PROPERTY("Flags", ezClothSheetFlags, GetFlags, SetFlags),
      EZ_RESOURCE_MEMBER_PROPERTY("Material", m_hMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material")),
      EZ_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new ezDefaultValueAttribute(ezColor::White)),
    }
    EZ_END_PROPERTIES;
    EZ_BEGIN_ATTRIBUTES
    {
      new ezCategoryAttribute("Effects"),
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

ezClothSheetComponent::ezClothSheetComponent() = default;
ezClothSheetComponent::~ezClothSheetComponent() = default;

void ezClothSheetComponent::SetSize(ezVec2 vVal)
{
  m_vSize = vVal;
  SetupCloth();
}

void ezClothSheetComponent::SetSlack(ezVec2 vVal)
{
  m_vSlack = vVal;
  SetupCloth();
}

void ezClothSheetComponent::SetSegments(ezVec2U32 vVal)
{
  m_vSegments = vVal;
  SetupCloth();
}

void ezClothSheetComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_vSize;
  s << m_vSegments;
  s << m_vSlack;
  s << m_fWindInfluence;
  s << m_fDamping;
  s << m_Flags;
  s << m_hMaterial;
  s << m_Color;
}

void ezClothSheetComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_vSize;
  s >> m_vSegments;
  s >> m_vSlack;
  s >> m_fWindInfluence;
  s >> m_fDamping;
  s >> m_Flags;
  s >> m_hMaterial;
  s >> m_Color;
}

void ezClothSheetComponent::OnActivated()
{
  SUPER::OnActivated();

  SetupCloth();
}

void ezClothSheetComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  SetupCloth();
}

void ezClothSheetComponent::OnDeactivated()
{
  ezRenderDataManager* pRenderDataManager = GetWorld()->GetModule<ezRenderDataManager>();
  pRenderDataManager->DeleteInstanceData(m_InstanceDataOffset);

  m_Simulator.m_Nodes.Clear();

  SUPER::OnDeactivated();
}

ezResult ezClothSheetComponent::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  if (m_Bbox.IsValid())
  {
    ref_bounds.ExpandToInclude(ezBoundingBoxSphere::MakeFromBox(m_Bbox));
  }
  else
  {
    ezBoundingBox box = ezBoundingBox::MakeInvalid();
    box.ExpandToInclude(ezVec3::MakeZero());
    box.ExpandToInclude(ezVec3(m_vSize.x, 0, -0.1f));
    box.ExpandToInclude(ezVec3(0, m_vSize.y, +0.1f));
    box.ExpandToInclude(ezVec3(m_vSize.x, m_vSize.y, 0));

    ref_bounds.ExpandToInclude(ezBoundingBoxSphere::MakeFromBox(box));
  }

  return EZ_SUCCESS;
}

void ezClothSheetComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!m_hDynamicMeshBuffer.IsValid())
    return;

  const bool bDynamic = GetOwner()->IsDynamic();
  auto hInstanceDataBuffer = msg.m_pRenderDataManager->GetOrCreateInstanceDataAndFill(*this, bDynamic, GetOwner()->GetGlobalTransform(), m_InstanceDataOffset, GetUniqueIdForRendering(), m_Color);

  ezCustomMeshRenderData* pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<ezCustomMeshRenderData>(GetOwner());
  {
    pRenderData->m_uiNumInstances = 1;
    pRenderData->m_DataOffsets.m_uiInstance = m_InstanceDataOffset.m_uiOffset;
    pRenderData->m_hInstanceDataBuffer = hInstanceDataBuffer;
    pRenderData->m_fSortingDepthOffset = 0.0f;

    pRenderData->m_hMaterial = m_hMaterial;
    pRenderData->m_hDynamicMeshBuffer = m_hDynamicMeshBuffer;
    pRenderData->m_uiFirstPrimitive = 0;
    pRenderData->m_uiNumPrimitives = m_vSegments.x * m_vSegments.y * 2;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    pRenderData->m_FallbackGlobalBBox = GetOwner()->GetGlobalBounds().GetBox();
#endif

    pRenderData->FillSortingKey();
  }

  ezRenderData::Category category = ezDefaultRenderDataCategories::LitOpaque;
  bool bDontCacheYet = true;

  if (m_hMaterial.IsValid())
  {
    ezResourceLock<ezMaterialResource> pMaterial(m_hMaterial, ezResourceAcquireMode::AllowLoadingFallback);
    category = pMaterial->GetRenderDataCategory();
    bDontCacheYet = pMaterial.GetAcquireResult() == ezResourceAcquireResult::LoadingFallback;
  }

  msg.AddRenderData(pRenderData, category, bDontCacheYet ? ezRenderData::Caching::Never : ezRenderData::Caching::IfStatic);
}

void ezClothSheetComponent::SetFlags(ezBitflags<ezClothSheetFlags> flags)
{
  m_Flags = flags;
  SetupCloth();
}

void ezClothSheetComponent::Update()
{
  if (m_Simulator.m_Nodes.IsEmpty() || GetOwner()->GetVisibilityState() == ezVisibilityState::Invisible)
    return;

  {
    ezVec3 acc = -GetOwner()->GetLinearVelocity();

    if (const ezPhysicsWorldModuleInterface* pModule = GetWorld()->GetModuleReadOnly<ezPhysicsWorldModuleInterface>())
    {
      acc += pModule->GetGravity();
    }
    else
    {
      acc += ezVec3(0, 0, -9.81f);
    }

    if (m_fWindInfluence > 0.0f)
    {
      if (const ezWindWorldModuleInterface* pWind = GetWorld()->GetModuleReadOnly<ezWindWorldModuleInterface>())
      {
        ezVec3 ropeDir(0, 0, 1);

        // take the position of the center cloth node to sample the wind
        const ezVec3 vSampleWindPos = GetOwner()->GetGlobalTransform().TransformPosition(ezSimdConversion::ToVec3(m_Simulator.m_Nodes[m_Simulator.m_uiWidth * (m_Simulator.m_uiHeight / 2) + m_Simulator.m_uiWidth / 2].m_vPosition));

        const ezVec3 vWind = pWind->GetWindAt(vSampleWindPos) * m_fWindInfluence;

        acc += vWind;
        acc += pWind->ComputeWindFlutter(vWind, ropeDir, 0.5f, GetOwner()->GetStableRandomSeed());
      }
    }

    // rotate the acceleration vector into the local simulation space
    acc = GetOwner()->GetGlobalRotation().GetInverse() * acc;

    if (m_Simulator.m_vAcceleration != acc)
    {
      m_Simulator.m_vAcceleration = acc;
      m_uiSleepCounter = 0;
    }
  }

  if (m_uiSleepCounter <= 10)
  {
    m_Simulator.m_fDampingFactor = ezMath::Lerp(1.0f, 0.97f, m_fDamping);

    m_Simulator.SimulateCloth(GetWorld()->GetClock().GetTimeDiff());

    auto prevBbox = m_Bbox;
    m_Bbox.ExpandToInclude(ezSimdConversion::ToVec3(m_Simulator.m_Nodes[0].m_vPosition));
    m_Bbox.ExpandToInclude(ezSimdConversion::ToVec3(m_Simulator.m_Nodes[m_Simulator.m_uiWidth - 1].m_vPosition));
    m_Bbox.ExpandToInclude(ezSimdConversion::ToVec3(m_Simulator.m_Nodes[((m_Simulator.m_uiHeight - 1) * m_Simulator.m_uiWidth)].m_vPosition));
    m_Bbox.ExpandToInclude(ezSimdConversion::ToVec3(m_Simulator.m_Nodes.PeekBack().m_vPosition));

    if (prevBbox != m_Bbox)
    {
      SetUserFlag(0, true); // flag 0 => requires local bounds update

      // can't call this here in the async phase
      // TriggerLocalBoundsUpdate();
    }

    UpdateClothMesh();

    ++m_uiCheckEquilibriumCounter;
    if (m_uiCheckEquilibriumCounter > 64)
    {
      m_uiCheckEquilibriumCounter = 0;

      if (m_Simulator.HasEquilibrium(0.01f))
      {
        ++m_uiSleepCounter;
      }
      else
      {
        m_uiSleepCounter = 0;
      }
    }
  }
}

void ezClothSheetComponent::UpdateClothMesh()
{
  ezResourceLock<ezDynamicMeshBufferResource> pDynamicMeshBuffer(m_hDynamicMeshBuffer, ezResourceAcquireMode::BlockTillLoaded);

  auto nodes = m_Simulator.m_Nodes.GetArrayPtr();
  auto positions = pDynamicMeshBuffer->AccessPositionData();

  const ezVec2U32 vNumVertices = m_vSegments + ezVec2U32(1);

  ezUInt32 vidx = 0;
  for (ezUInt32 y = 0; y < vNumVertices.y; ++y)
  {
    for (ezUInt32 x = 0; x < vNumVertices.x; ++x, ++vidx)
    {
      positions[vidx] = ezSimdConversion::ToVec3(nodes[vidx].m_vPosition);
    }
  }

  ezDynamicMeshBufferResource::CalculateGridNormalAndTangents(pDynamicMeshBuffer.GetPointerNonConst(), vNumVertices);
}

void ezClothSheetComponent::SetupCloth()
{
  m_Bbox = ezBoundingBox::MakeInvalid();

  if (IsActiveAndSimulating())
  {
    m_uiSleepCounter = 0;

    m_Simulator.m_uiWidth = static_cast<ezUInt8>(m_vSegments.x + 1);
    m_Simulator.m_uiHeight = static_cast<ezUInt8>(m_vSegments.y + 1);
    m_Simulator.m_vAcceleration.Set(0, 0, -10);
    m_Simulator.m_vSegmentLength = m_vSize.CompMul(ezVec2(1.0f) + m_vSlack);
    m_Simulator.m_vSegmentLength.x /= (float)m_vSegments.x;
    m_Simulator.m_vSegmentLength.y /= (float)m_vSegments.y;
    m_Simulator.m_Nodes.Clear();
    m_Simulator.m_Nodes.SetCount(m_Simulator.m_uiWidth * m_Simulator.m_uiHeight);

    const ezVec3 dirX = ezVec3(1, 0, 0);
    const ezVec3 dirY = ezVec3(0, 1, 0);

    ezVec2 dist = m_vSize;
    dist.x /= (float)m_vSegments.x;
    dist.y /= (float)m_vSegments.y;

    for (ezUInt32 y = 0; y < m_Simulator.m_uiHeight; ++y)
    {
      for (ezUInt32 x = 0; x < m_Simulator.m_uiWidth; ++x)
      {
        const ezUInt32 idx = (y * m_Simulator.m_uiWidth) + x;

        m_Simulator.m_Nodes[idx].m_vPosition = ezSimdConversion::ToVec3(x * dist.x * dirX + y * dist.y * dirY);
        m_Simulator.m_Nodes[idx].m_vPreviousPosition = m_Simulator.m_Nodes[idx].m_vPosition;
      }
    }

    if (m_Flags.IsSet(ezClothSheetFlags::FixedCornerTopLeft))
      m_Simulator.m_Nodes[0].m_bFixed = true;

    if (m_Flags.IsSet(ezClothSheetFlags::FixedCornerTopRight))
      m_Simulator.m_Nodes[m_Simulator.m_uiWidth - 1].m_bFixed = true;

    if (m_Flags.IsSet(ezClothSheetFlags::FixedCornerBottomRight))
      m_Simulator.m_Nodes[m_Simulator.m_uiWidth * m_Simulator.m_uiHeight - 1].m_bFixed = true;

    if (m_Flags.IsSet(ezClothSheetFlags::FixedCornerBottomLeft))
      m_Simulator.m_Nodes[m_Simulator.m_uiWidth * (m_Simulator.m_uiHeight - 1)].m_bFixed = true;

    if (m_Flags.IsSet(ezClothSheetFlags::FixedEdgeTop))
    {
      for (ezUInt32 x = 0; x < m_Simulator.m_uiWidth; ++x)
      {
        const ezUInt32 idx = (0 * m_Simulator.m_uiWidth) + x;

        m_Simulator.m_Nodes[idx].m_bFixed = true;
      }
    }

    if (m_Flags.IsSet(ezClothSheetFlags::FixedEdgeRight))
    {
      for (ezUInt32 y = 0; y < m_Simulator.m_uiHeight; ++y)
      {
        const ezUInt32 idx = (y * m_Simulator.m_uiWidth) + (m_Simulator.m_uiWidth - 1);

        m_Simulator.m_Nodes[idx].m_bFixed = true;
      }
    }

    if (m_Flags.IsSet(ezClothSheetFlags::FixedEdgeBottom))
    {
      for (ezUInt32 x = 0; x < m_Simulator.m_uiWidth; ++x)
      {
        const ezUInt32 idx = ((m_Simulator.m_uiHeight - 1) * m_Simulator.m_uiWidth) + x;

        m_Simulator.m_Nodes[idx].m_bFixed = true;
      }
    }

    if (m_Flags.IsSet(ezClothSheetFlags::FixedEdgeLeft))
    {
      for (ezUInt32 y = 0; y < m_Simulator.m_uiHeight; ++y)
      {
        const ezUInt32 idx = (y * m_Simulator.m_uiWidth) + 0;

        m_Simulator.m_Nodes[idx].m_bFixed = true;
      }
    }
  }

  if (IsActiveAndInitialized() && m_vSize.x > 0 && m_vSize.y > 0 && m_vSegments.x > 0 && m_vSegments.y > 0)
  {
    ezStringBuilder sResourceName;
    sResourceName.SetFormat("ClothSheet_{}_{}x{}_{}x{}", ezArgP(this), m_vSize.x, m_vSize.y, m_vSegments.x, m_vSegments.y);

    m_hDynamicMeshBuffer = ezResourceManager::GetExistingResource<ezDynamicMeshBufferResource>(sResourceName);

    if (!m_hDynamicMeshBuffer.IsValid())
    {
      ezDynamicMeshBufferResourceDescriptor desc;
      desc.m_uiMaxVertices = (m_vSegments.x + 1) * (m_vSegments.y + 1);
      desc.m_IndexType = ezGALIndexType::UShort;
      desc.m_uiMaxPrimitives = m_vSegments.x * m_vSegments.y * 2;

      m_hDynamicMeshBuffer = ezResourceManager::GetOrCreateResource<ezDynamicMeshBufferResource>(sResourceName, std::move(desc));
    }

    ezResourceLock<ezDynamicMeshBufferResource> pDynamicMeshBuffer(m_hDynamicMeshBuffer, ezResourceAcquireMode::BlockTillLoaded);
    ezDynamicMeshBufferResource::CreateGridXY(pDynamicMeshBuffer.GetPointerNonConst(), m_vSize, m_vSegments + ezVec2U32(1));
  }

  TriggerLocalBoundsUpdate();
}

//////////////////////////////////////////////////////////////////////////

ezClothSheetComponentManager::ezClothSheetComponentManager(ezWorld* pWorld)
  : ezComponentManager(pWorld)
{
}

ezClothSheetComponentManager::~ezClothSheetComponentManager() = default;

void ezClothSheetComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezClothSheetComponentManager::Update, this);
    desc.m_Phase = ezWorldUpdatePhase::Async;
    desc.m_bOnlyUpdateWhenSimulating = true;
    desc.m_uiAsyncPhaseBatchSize = 2;

    this->RegisterUpdateFunction(desc);
  }

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezClothSheetComponentManager::UpdateBounds, this);
    desc.m_Phase = ezWorldUpdatePhase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }
}

void ezClothSheetComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized())
    {
      it->Update();
    }
  }
}

void ezClothSheetComponentManager::UpdateBounds(const ezWorldModule::UpdateContext& context)
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


EZ_STATICLINK_FILE(GameComponentsPlugin, GameComponentsPlugin_Physics_Implementation_ClothSheetComponent);
