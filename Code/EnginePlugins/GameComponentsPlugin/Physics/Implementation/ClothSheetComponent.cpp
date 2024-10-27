#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Interfaces/WindWorldModule.h>
#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Physics/ClothSheetComponent.h>
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

/* TODO:
 * cache render category
 */

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezClothSheetRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezClothSheetRenderer, 1, ezRTTIDefaultAllocator<ezClothSheetRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

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

void ezClothSheetComponent::SetupCloth()
{
  m_Bbox = ezBoundingBox::MakeInvalid();

  if (IsActiveAndSimulating())
  {
    m_uiSleepCounter = 0;
    m_uiVisibleCounter = 5;

    m_Simulator.m_uiWidth = static_cast<ezUInt8>(m_vSegments.x + 1);
    m_Simulator.m_uiHeight = static_cast<ezUInt8>(m_vSegments.y + 1);
    m_Simulator.m_vAcceleration.Set(0, 0, -10);
    m_Simulator.m_vSegmentLength = m_vSize.CompMul(ezVec2(1.0f) + m_vSlack);
    m_Simulator.m_vSegmentLength.x /= (float)m_vSegments.x;
    m_Simulator.m_vSegmentLength.y /= (float)m_vSegments.y;
    m_Simulator.m_Nodes.Clear();
    m_Simulator.m_Nodes.SetCount(m_Simulator.m_uiWidth * m_Simulator.m_uiHeight);

    const ezVec3 pos = ezVec3(0);
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

        m_Simulator.m_Nodes[idx].m_vPosition = ezSimdConversion::ToVec3(pos + x * dist.x * dirX + y * dist.y * dirY);
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

  TriggerLocalBoundsUpdate();
}

void ezClothSheetComponent::OnDeactivated()
{
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
  auto pRenderData = ezCreateRenderDataForThisFrame<ezClothSheetRenderData>(GetOwner());
  pRenderData->m_uiUniqueID = GetUniqueIdForRendering();
  pRenderData->m_Color = m_Color;
  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_uiBatchId = ezHashingUtils::StringHashTo32(m_hMaterial.GetResourceIDHash());
  pRenderData->m_uiSortingKey = pRenderData->m_uiBatchId;
  pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
  pRenderData->m_hMaterial = m_hMaterial;


  if (m_Simulator.m_Nodes.IsEmpty())
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
    m_uiVisibleCounter = 3;

    pRenderData->m_uiVerticesX = m_Simulator.m_uiWidth;
    pRenderData->m_uiVerticesY = m_Simulator.m_uiHeight;

    pRenderData->m_Positions = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezVec3, pRenderData->m_uiVerticesX * pRenderData->m_uiVerticesY);
    pRenderData->m_Indices = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezUInt16, (pRenderData->m_uiVerticesX - 1) * (pRenderData->m_uiVerticesY - 1) * 2 * 3);

    {
      ezUInt32 vidx = 0;
      for (ezUInt32 y = 0; y < pRenderData->m_uiVerticesY; ++y)
      {
        for (ezUInt32 x = 0; x < pRenderData->m_uiVerticesX; ++x, ++vidx)
        {
          pRenderData->m_Positions[vidx] = ezSimdConversion::ToVec3(m_Simulator.m_Nodes[vidx].m_vPosition);
        }
      }
    }

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

  // TODO: render pass category (plus cache this)
  ezRenderData::Category category = ezDefaultRenderDataCategories::LitOpaque;

  if (m_hMaterial.IsValid())
  {
    ezResourceLock<ezMaterialResource> pMaterial(m_hMaterial, ezResourceAcquireMode::AllowLoadingFallback);
    category = pMaterial->GetRenderDataCategory();
  }

  msg.AddRenderData(pRenderData, category, ezRenderData::Caching::Never);
}

void ezClothSheetComponent::SetFlags(ezBitflags<ezClothSheetFlags> flags)
{
  m_Flags = flags;
  SetupCloth();
}

void ezClothSheetComponent::Update()
{
  if (m_Simulator.m_Nodes.IsEmpty() || m_uiVisibleCounter == 0)
    return;

  --m_uiVisibleCounter;

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

ezClothSheetRenderer::ezClothSheetRenderer()
{
  CreateVertexBuffer();
}

ezClothSheetRenderer::~ezClothSheetRenderer() = default;

void ezClothSheetRenderer::GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(ezDefaultRenderDataCategories::LitOpaque);
  ref_categories.PushBack(ezDefaultRenderDataCategories::LitMasked);
  ref_categories.PushBack(ezDefaultRenderDataCategories::LitTransparent);
  ref_categories.PushBack(ezDefaultRenderDataCategories::Selection);
}

void ezClothSheetRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(ezGetStaticRTTI<ezClothSheetRenderData>());
}

void ezClothSheetRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const
{
  const bool bNeedsNormals = (renderViewContext.m_pViewData->m_CameraUsageHint != ezCameraUsageHint::Shadow);


  ezRenderContext* pRenderContext = renderViewContext.m_pRenderContext;
  ezGALCommandEncoder* pGALCommandEncoder = pRenderContext->GetCommandEncoder();

  ezInstanceData* pInstanceData = pPass->GetPipeline()->GetFrameDataProvider<ezInstanceDataProvider>()->GetData(renderViewContext);
  pInstanceData->BindResources(pRenderContext);

  pRenderContext->SetShaderPermutationVariable("FLIP_WINDING", "FALSE");
  pRenderContext->SetShaderPermutationVariable("VERTEX_SKINNING", "FALSE");

  ezResourceLock<ezDynamicMeshBufferResource> pBuffer(m_hDynamicMeshBuffer, ezResourceAcquireMode::BlockTillLoaded);

  for (auto it = batch.GetIterator<ezClothSheetRenderData>(0, batch.GetCount()); it.IsValid(); ++it)
  {
    const ezClothSheetRenderData* pRenderData = it;

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
            pVertexData[vidx].m_vTexCoord = ezVec2(x * fDivU, y * fDivY);
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
            pVertexData[vidx].m_vTexCoord = ezVec2(x * fDivU, y * fDivY);
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

void ezClothSheetRenderer::CreateVertexBuffer()
{
  if (m_hDynamicMeshBuffer.IsValid())
    return;

  m_hDynamicMeshBuffer = ezResourceManager::GetExistingResource<ezDynamicMeshBufferResource>("ClothSheet");

  if (!m_hDynamicMeshBuffer.IsValid())
  {
    const ezUInt32 uiMaxVerts = 32;

    ezDynamicMeshBufferResourceDescriptor desc;
    desc.m_uiMaxVertices = uiMaxVerts * uiMaxVerts;
    desc.m_IndexType = ezGALIndexType::UShort;
    desc.m_uiMaxPrimitives = ezMath::Square(uiMaxVerts - 1) * 2;

    m_hDynamicMeshBuffer = ezResourceManager::GetOrCreateResource<ezDynamicMeshBufferResource>("ClothSheet", std::move(desc), "Cloth Sheet Buffer");
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
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
    desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::Async;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezClothSheetComponentManager::UpdateBounds, this);
    desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PostAsync;
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
