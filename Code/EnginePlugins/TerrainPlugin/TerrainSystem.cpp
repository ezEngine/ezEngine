#include <TerrainPlugin/TerrainPluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Math/Float16.h>
#include <GameEngine/Utils/ImageDataResource.h>
#include <RendererCore/RenderContext/BindGroupBuilder.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphContext.h>
#include <RendererCore/RenderGraph/RenderGraphManager.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/ReadbackHelper.h>
#include <RendererFoundation/Utils/ResourceStateTracker.h>
#include <Shaders/Terrain/Generation/HeightfieldBakeConstants.h>
#include <TerrainPlugin/Components/TerrainBrushBaseComponent.h>
#include <TerrainPlugin/TerrainSystem.h>
#include <Texture/Image/ImageUtils.h>

static void ExecuteGraphSync(ezRenderGraph& ref_graph, ezGALDevice* pDevice)
{
  EZ_VERIFY(ref_graph.Compile().Succeeded(), "Terrain sync render graph compilation failed");

  pDevice->BeginFrame();

  ezGALResourceStateTracker tracker(pDevice);
  ref_graph.ComputeBarriers(tracker);

  auto* pEncoder = pDevice->BeginCommands("TerrainSync");
  ezRenderGraphContext ctx(pEncoder, pDevice, ezRenderContext::GetDefaultInstance());
  ref_graph.Execute(ctx);

  ezHybridArray<ezGALBufferBarrier, 4> bufBarriers;
  tracker.RevertBufferState([&](const ezGALBufferBarrier& b)
    { bufBarriers.PushBack(b); });
  if (!bufBarriers.IsEmpty())
    pEncoder->BufferBarrier(bufBarriers);

  pDevice->EndCommands(pEncoder);

  pDevice->EndFrame();
}

// clang-format off
EZ_IMPLEMENT_WORLD_MODULE(ezTerrainSystem);

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTerrainSystem, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTerrainResolution, 1)
  EZ_ENUM_CONSTANT(ezTerrainResolution::Res32),
  EZ_ENUM_CONSTANT(ezTerrainResolution::Res64),
  EZ_ENUM_CONSTANT(ezTerrainResolution::Res128),
  EZ_ENUM_CONSTANT(ezTerrainResolution::Res256),
  EZ_ENUM_CONSTANT(ezTerrainResolution::Res512),
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTerrainPatchColliderMode, 1)
  EZ_ENUM_CONSTANT(ezTerrainPatchColliderMode::None),
  EZ_ENUM_CONSTANT(ezTerrainPatchColliderMode::FullResolution),
  EZ_ENUM_CONSTANT(ezTerrainPatchColliderMode::HalfResolution),
  EZ_ENUM_CONSTANT(ezTerrainPatchColliderMode::QuarterResolution),
  EZ_ENUM_CONSTANT(ezTerrainPatchColliderMode::EighthResolution),
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

ezDeque<ezTerrainSystem*> ezTerrainSystem::s_TerrainSystems;

ezTerrainSystem::ezTerrainSystem(ezWorld* pWorld)
  : SUPER(pWorld)
{
}

ezTerrainSystem::~ezTerrainSystem() = default;

void ezTerrainSystem::Initialize()
{
  SUPER::Initialize();

  if (s_TerrainSystems.IsEmpty())
  {
    ezRenderWorld::GetRenderEvent().AddEventHandler(OnRenderEvent);
  }

  s_TerrainSystems.PushBack(this);

  m_pRenderGraph = ezRenderGraphManager::CreateRenderGraph("TerrainBake", ezRenderGraphPhase::PreRender);

  m_hTerrainBakeStep1Shader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Terrain/Generation/HeightfieldTerrainBakeStep1CS.ezShader");
  m_hTerrainBakeStep2Shader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Terrain/Generation/HeightfieldTerrainBakeStep2CS.ezShader");
  m_hTerrainBakeStep3Shader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Terrain/Generation/HeightfieldTerrainBakeStep3CS.ezShader");
  m_hHeightfieldBakeConstants = ezRenderContext::CreateConstantBufferStorage<HeightfieldBakeConstants>();
}

void ezTerrainSystem::Deinitialize()
{
  FrameCleanup();

  s_TerrainSystems.RemoveAndSwap(this);

  if (s_TerrainSystems.IsEmpty())
  {
    ezRenderWorld::GetRenderEvent().RemoveEventHandler(OnRenderEvent);
  }

  m_pRenderGraph = nullptr;

  ezRenderContext::DeleteConstantBufferStorage(m_hHeightfieldBakeConstants);
  DestroyHeightfields();
  DestroySharedHeightfieldScratch();

  SUPER::Deinitialize();
}

void ezTerrainSystem::FrameCleanup()
{
  EZ_LOCK(m_Mutex);

  for (ezUInt32 idx : m_QueuedHeightfieldsToDelete)
  {
    DestroyHeightfieldTerrain(idx);
  }
  m_QueuedHeightfieldsToDelete.Clear();
}

void ezTerrainSystem::OnRenderEvent(const ezRenderWorldRenderEvent& e)
{
  if (e.m_Type != ezRenderWorldRenderEvent::Type::BeginRender)
    return;

  for (auto* pTerrain : s_TerrainSystems)
  {
    pTerrain->UpdateTerrain();
  }
}

void ezTerrainSystem::UpdateTerrain()
{
  FrameCleanup();

  bool bAnyDirty = m_bBrushesDirty;

  if (!bAnyDirty)
  {
    for (const auto& patch : m_Heightfields)
    {
      if (patch.m_bInUse && patch.m_bDirty)
      {
        bAnyDirty = true;
        break;
      }
    }
  }

  if (!bAnyDirty)
    return;

  EZ_PROFILE_SCOPE("UpdateTerrain");

  m_pRenderGraph->Reset();

  for (ezUInt32 i = 0; i < m_Heightfields.GetCount(); ++i)
  {
    ezTerrainData_Heightfield& patch = m_Heightfields[i];

    if (!patch.m_bInUse)
      continue;

    bool bShouldBake = patch.m_bDirty;

    if (m_bBrushesDirty)
    {
      const ezUInt64 uiNewHash = ComputeHeightfieldBrushOverlapHash(patch);
      if (uiNewHash != patch.m_uiBrushOverlapHash)
        bShouldBake = true;

      patch.m_uiBrushOverlapHash = uiNewHash;
    }

    if (bShouldBake)
    {
      UpdateHeightfield(i, *m_pRenderGraph);
    }
  }

  ezRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);

  m_bBrushesDirty = false;
}

//////////////////////////////////////////////////////////////////////////
// Brushes
//////////////////////////////////////////////////////////////////////////

ezUInt32 ezTerrainSystem::CreateBrushData()
{
  for (ezUInt32 idx = 0; idx < m_Brushes.GetCount(); ++idx)
  {
    if (!m_Brushes[idx].m_bInUse)
    {
      m_Brushes[idx].m_bInUse = true;
      return idx;
    }
  }

  m_Brushes.ExpandAndGetRef().m_bInUse = true;
  return m_Brushes.GetCount() - 1;
}

void ezTerrainSystem::RemoveBrushData(ezUInt32& ref_uiIdx)
{
  if (ref_uiIdx == ezInvalidIndex)
    return;

  EZ_ASSERT_DEV(ref_uiIdx < m_Brushes.GetCount(), "Invalid brush index");

  m_bBrushesDirty = true;
  m_Brushes[ref_uiIdx].m_bInUse = false;
  ref_uiIdx = ezInvalidIndex;
}

const ezTerrainData_Brush& ezTerrainSystem::ReadBrushData(ezUInt32 uiIdx) const
{
  return m_Brushes[uiIdx];
}

ezTerrainData_Brush& ezTerrainSystem::ModifyBrushData(ezUInt32 uiIdx)
{
  m_bBrushesDirty = true;
  return m_Brushes[uiIdx];
}

ezGALBufferHandle ezTerrainSystem::CreateBrushBuffer(ezDynamicArray<TerrainBrushData>& brushes, ezGALDevice* pDevice) const
{
  // Always allocate at least one element so the binding is always valid.
  // BrushCount=0 prevents the shader from indexing into it.
  if (brushes.IsEmpty())
    brushes.ExpandAndGetRef();

  ezGALBufferCreationDescription brushDesc;
  brushDesc.m_uiStructSize = sizeof(TerrainBrushData);
  brushDesc.m_uiTotalSize = brushes.GetCount() * sizeof(TerrainBrushData);
  brushDesc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource;

  return pDevice->CreateBuffer(brushDesc, ezArrayPtr<const ezUInt8>(reinterpret_cast<const ezUInt8*>(brushes.GetData()), brushes.GetCount() * sizeof(TerrainBrushData)));
}

//////////////////////////////////////////////////////////////////////////
// Heightfield Terrain
//////////////////////////////////////////////////////////////////////////

ezUInt32 ezTerrainSystem::CreateHeightfieldTerrain(ezUInt32 uiCellsPerSide)
{
  EZ_PROFILE_SCOPE("CreateHeightfieldTerrain");

  // Find a free slot first
  ezUInt32 uiIndex = ezInvalidIndex;
  for (ezUInt32 i = 0; i < m_Heightfields.GetCount(); ++i)
  {
    if (!m_Heightfields[i].m_bInUse)
    {
      uiIndex = i;
      break;
    }
  }

  if (uiIndex == ezInvalidIndex)
  {
    uiIndex = m_Heightfields.GetCount();
    m_Heightfields.ExpandAndGetRef();
  }

  ezTerrainData_Heightfield& patch = m_Heightfields[uiIndex];
  patch.m_uiCellsPerSide = uiCellsPerSide;
  patch.m_bInUse = true;
  patch.m_bDirty = true;

  // Stored grid = render vertices (CellsPerSide+1) plus 4 extra rings on each side.
  // Extra rings let normals CS use correct central-differences at patch edges,
  // and provide enough padding for Jolt's required multiple-of-4 vertex count.
  const ezUInt32 uiStoredSize = uiCellsPerSide + 9;
  // Create persistent GPU buffer (UAV for CS, SRV in VS)
  ezGALBufferCreationDescription bufDesc;
  bufDesc.m_uiStructSize = sizeof(float);
  bufDesc.m_uiTotalSize = uiStoredSize * uiStoredSize * sizeof(float);
  bufDesc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource | ezGALBufferUsageFlags::UnorderedAccess;
  bufDesc.m_ResourceAccess.m_bImmutable = false;

  patch.m_hBakedHeights = ezGALDevice::GetDefaultDevice()->CreateBuffer(bufDesc);

  // Create persistent normal buffer (uint per vertex: XY as packed 16-bit floats)
  ezGALBufferCreationDescription normDesc;
  normDesc.m_uiStructSize = sizeof(ezUInt32);
  normDesc.m_uiTotalSize = uiStoredSize * uiStoredSize * sizeof(ezUInt32);
  normDesc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource | ezGALBufferUsageFlags::UnorderedAccess;
  normDesc.m_ResourceAccess.m_bImmutable = false;

  patch.m_hBakedNormals = ezGALDevice::GetDefaultDevice()->CreateBuffer(normDesc);

  // Per-cell top-3 material indices (uint per cell), written by Step3 and bound as SRV in the VS.
  ezGALBufferCreationDescription cellMatDesc;
  cellMatDesc.m_uiStructSize = sizeof(ezUInt32);
  cellMatDesc.m_uiTotalSize = uiCellsPerSide * uiCellsPerSide * sizeof(ezUInt32);
  cellMatDesc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource | ezGALBufferUsageFlags::UnorderedAccess;
  cellMatDesc.m_ResourceAccess.m_bImmutable = false;

  patch.m_hCellMaterials = ezGALDevice::GetDefaultDevice()->CreateBuffer(cellMatDesc);

  // Per-cell-corner f16 blend weights (uint per corner), written by Step3 and bound as SRV in the VS.
  // Layout: (cellIndex * 4 + cornerSlot), cornerSlot = TL:0, TR:1, BL:2, BR:3.
  // Each corner slot is unique — no inter-thread write conflicts in Step3.
  ezGALBufferCreationDescription weightDesc;
  weightDesc.m_uiStructSize = sizeof(ezUInt32);
  weightDesc.m_uiTotalSize = uiCellsPerSide * uiCellsPerSide * 4 * sizeof(ezUInt32);
  weightDesc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource | ezGALBufferUsageFlags::UnorderedAccess;
  weightDesc.m_ResourceAccess.m_bImmutable = false;

  patch.m_hVertexWeights = ezGALDevice::GetDefaultDevice()->CreateBuffer(weightDesc);

  return uiIndex;
}

void ezTerrainSystem::RemoveHeightfieldTerrain(ezUInt32& ref_uiPatchIndex)
{
  EZ_LOCK(m_Mutex);
  m_QueuedHeightfieldsToDelete.PushBack(ref_uiPatchIndex);
}

void ezTerrainSystem::DestroyHeightfieldTerrain(ezUInt32& uiIdx)
{
  if (uiIdx >= m_Heightfields.GetCount())
    return;

  auto& data = m_Heightfields[uiIdx];
  uiIdx = ezInvalidIndex;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  pDevice->DestroyBuffer(data.m_hBakedHeights);
  pDevice->DestroyBuffer(data.m_hBakedNormals);
  pDevice->DestroyBuffer(data.m_hCellMaterials);
  pDevice->DestroyBuffer(data.m_hVertexWeights);

  data.m_bInUse = false;
}

ezTerrainData_Heightfield& ezTerrainSystem::ModifyHeightfieldTerrain(ezUInt32 uiIdx)
{
  auto& data = m_Heightfields[uiIdx];
  data.m_bDirty = true;
  return data;
}

ezGALBufferHandle ezTerrainSystem::GetHeightfieldHeightBuffer(ezUInt32 uiPatchIndex) const
{
  if (uiPatchIndex < m_Heightfields.GetCount() && m_Heightfields[uiPatchIndex].m_bInUse)
    return m_Heightfields[uiPatchIndex].m_hBakedHeights;

  return ezGALBufferHandle();
}

ezGALBufferHandle ezTerrainSystem::GetHeightfieldNormalBuffer(ezUInt32 uiPatchIndex) const
{
  if (uiPatchIndex < m_Heightfields.GetCount() && m_Heightfields[uiPatchIndex].m_bInUse)
    return m_Heightfields[uiPatchIndex].m_hBakedNormals;

  return ezGALBufferHandle();
}

ezGALBufferHandle ezTerrainSystem::GetHeightfieldCellMaterialBuffer(ezUInt32 uiPatchIndex) const
{
  if (uiPatchIndex < m_Heightfields.GetCount() && m_Heightfields[uiPatchIndex].m_bInUse)
    return m_Heightfields[uiPatchIndex].m_hCellMaterials;

  return ezGALBufferHandle();
}

ezGALBufferHandle ezTerrainSystem::GetHeightfieldMaterialVertexWeightBuffer(ezUInt32 uiPatchIndex) const
{
  if (uiPatchIndex < m_Heightfields.GetCount() && m_Heightfields[uiPatchIndex].m_bInUse)
    return m_Heightfields[uiPatchIndex].m_hVertexWeights;

  return ezGALBufferHandle();
}

ezUInt32 ezTerrainSystem::GetHeightfieldCellsPerSide(ezUInt32 uiPatchIndex) const
{
  return (uiPatchIndex < m_Heightfields.GetCount()) ? m_Heightfields[uiPatchIndex].m_uiCellsPerSide : 128;
}

ezUInt64 ezTerrainSystem::GetHeightfieldBrushOverlapHash(ezUInt32 uiPatchIndex) const
{
  if (uiPatchIndex >= m_Heightfields.GetCount() || !m_Heightfields[uiPatchIndex].m_bInUse)
    return 0;
  return ComputeHeightfieldBrushOverlapHash(m_Heightfields[uiPatchIndex]);
}

ezUInt64 ezTerrainSystem::ComputeHeightfieldBrushOverlapHash(const ezTerrainData_Heightfield& heightfield) const
{
  const ezTransform invTrans = heightfield.m_GlobalTransform.GetInverse();
  const float fSize = (float)heightfield.m_uiCellsPerSide * heightfield.m_fGridSpacing;

  ezHashStreamWriter64 writer;

  for (ezUInt32 i = 0; i < m_Brushes.GetCount(); ++i)
  {
    const auto& brush = m_Brushes[i];
    if (!brush.m_bInUse)
      continue;

    if (!brush.m_Tags.IsEmpty() && !brush.m_Tags.IsAnySet(heightfield.m_Tags))
      continue;

    const ezVec3 vLocalCenter = invTrans * brush.m_vPosition;
    const float fConservativeRadius = ezMath::Sqrt(brush.m_vHalfExtents.x * brush.m_vHalfExtents.x + brush.m_vHalfExtents.y * brush.m_vHalfExtents.y) + brush.m_fInnerRadius + brush.m_fOuterRadius;

    const float fClampedX = ezMath::Clamp(vLocalCenter.x, 0.0f, fSize);
    const float fClampedY = ezMath::Clamp(vLocalCenter.y, 0.0f, fSize);
    const float fDx = vLocalCenter.x - fClampedX;
    const float fDy = vLocalCenter.y - fClampedY;

    if (fDx * fDx + fDy * fDy > fConservativeRadius * fConservativeRadius)
      continue;

    writer << i;
    writer << brush.m_vPosition.x << brush.m_vPosition.y << brush.m_vPosition.z;
    writer << brush.m_qRotation.x << brush.m_qRotation.y << brush.m_qRotation.z << brush.m_qRotation.w;
    writer << brush.m_vHalfExtents.x << brush.m_vHalfExtents.y;
    writer << brush.m_fHalfExtentZ << brush.m_fHalfExtentYTop;
    writer << brush.m_fInnerRadius << brush.m_fOuterRadius << brush.m_fFalloff;
    writer << brush.m_ModifyMode.GetValue();
    writer << brush.m_uiMaterialIndex << brush.m_fMaterialStrength;
    writer << brush.m_fNoiseStrength;
    writer << brush.m_fNoiseFrequency;
    writer << brush.m_iPriority;
    brush.m_Tags.Save(writer);
  }

  writer << heightfield.m_uiDefaultMaterialIndex;
  heightfield.m_Tags.Save(writer);

  return writer.GetHashValue();
}

void ezTerrainSystem::FindHeightfieldOverlappingBrushes(const ezTerrainData_Heightfield& heightfield, ezDynamicArray<TerrainBrushData>& brushes) const
{
  const ezTransform invTrans = heightfield.m_GlobalTransform.GetInverse();
  const float fSize = (float)heightfield.m_uiCellsPerSide * heightfield.m_fGridSpacing;

  brushes.Clear();
  brushes.Reserve(m_Brushes.GetCount());

  const ezMat3 mInvTerrainRot = heightfield.m_GlobalTransform.m_qRotation.GetAsMat3().GetTranspose();

  for (auto& brush : m_Brushes)
  {
    if (!brush.m_bInUse)
      continue;

    if (!brush.m_Tags.IsEmpty() && !brush.m_Tags.IsAnySet(heightfield.m_Tags))
      continue;

    if ((brush.m_ModifyMode == ezTerrainModifyMode::OnlyPaint2D || brush.m_ModifyMode == ezTerrainModifyMode::OnlyPaint3D) && brush.m_fMaterialStrength <= 0.0f)
      continue;

    // Skip brushes whose conservative footprint does not overlap the heightfield XY extent.
    const ezVec3 vLocalCenter = invTrans * brush.m_vPosition;
    const float fConservativeRadius = ezMath::Sqrt(brush.m_vHalfExtents.x * brush.m_vHalfExtents.x + brush.m_vHalfExtents.y * brush.m_vHalfExtents.y) + brush.m_fInnerRadius + brush.m_fOuterRadius;
    const float fClampedX = ezMath::Clamp(vLocalCenter.x, 0.0f, fSize);
    const float fClampedY = ezMath::Clamp(vLocalCenter.y, 0.0f, fSize);
    const float fDx = vLocalCenter.x - fClampedX;
    const float fDy = vLocalCenter.y - fClampedY;
    if (fDx * fDx + fDy * fDy > fConservativeRadius * fConservativeRadius)
      continue;

    TerrainBrushData& bd = brushes.ExpandAndGetRef();
    bd.Position = vLocalCenter;
    // bd.Position.z -= 0.5f; // keep the brush component slightly above the baked terrain so its icon remains visible
    bd.HalfExtentX = brush.m_vHalfExtents.x;
    bd.HalfExtentYBottom = brush.m_vHalfExtents.y;
    bd.HalfExtentYTop = brush.m_fHalfExtentYTop;
    bd.HalfExtentZ = brush.m_fHalfExtentZ;
    bd.InnerRadius = brush.m_fInnerRadius;
    bd.OuterRadius = ezMath::Max(brush.m_fOuterRadius, 0.0001f);
    bd.Falloff = brush.m_fFalloff;
    bd.ModifyMode = brush.m_ModifyMode.GetValue();
    bd.MaterialIndex = brush.m_uiMaterialIndex;
    bd.MaterialStrength = brush.m_fMaterialStrength;
    bd.NoiseStrength = brush.m_fNoiseStrength;
    bd.NoiseFrequency = ezMath::Max(0.0001f, brush.m_fNoiseFrequency);
    bd.CpuPriority = static_cast<float>(brush.m_iPriority);

    // Build the inverse rotation from terrain-local space into brush-local space.
    // mLocalBrushRot = invTerrainRot * brushWorldRot  (takes brush-local -> terrain-local)
    // mInvLocalBrushRot = transpose of above          (takes terrain-local -> brush-local)
    const ezMat3 mInvLocalBrushRot = (mInvTerrainRot * brush.m_qRotation.GetAsMat3()).GetTranspose();

    bd.InvRotRow0 = mInvLocalBrushRot.GetRow(0);
    bd.InvRotRow1 = mInvLocalBrushRot.GetRow(1);
    bd.InvRotRow2 = mInvLocalBrushRot.GetRow(2);
  }

  // Primary sort: priority ascending (higher priority = applied later = wins).
  // Within the same priority: Carve last, then Max < Min < Set, then by height.
  brushes.Sort([](const TerrainBrushData& a, const TerrainBrushData& b) -> bool
    {
      if (a.CpuPriority != b.CpuPriority)
        return a.CpuPriority < b.CpuPriority;
      const bool aCarve = a.ModifyMode == ezTerrainModifyMode::Carve;
      const bool bCarve = b.ModifyMode == ezTerrainModifyMode::Carve;
      if (aCarve != bCarve)
        return !aCarve;
      if (a.ModifyMode != b.ModifyMode)
        return a.ModifyMode < b.ModifyMode;
      if (a.ModifyMode == 1u)             // Min: descending height
        return a.Position.z > b.Position.z;
      return a.Position.z < b.Position.z; // Max / Set / Ignore: ascending height
    });
}

void ezTerrainSystem::EnsureSharedHeightfieldScratch(ezUInt32 uiStoredSize)
{
  if (uiStoredSize <= m_uiHeightfieldSharedMaskStoredSize && !m_hHeightfieldSharedMask.IsInvalidated())
    return;

  DestroySharedHeightfieldScratch();

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // Intermediate material mask (uint2 per vertex): written by Step1/2, read by Step3.
  ezGALBufferCreationDescription maskDesc;
  maskDesc.m_uiStructSize = sizeof(ezUInt64);
  maskDesc.m_uiTotalSize = uiStoredSize * uiStoredSize * sizeof(ezUInt64);
  maskDesc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource | ezGALBufferUsageFlags::UnorderedAccess;
  maskDesc.m_ResourceAccess.m_bImmutable = false;

  m_hHeightfieldSharedMask = pDevice->CreateBuffer(maskDesc);
  m_uiHeightfieldSharedMaskStoredSize = uiStoredSize;
}

void ezTerrainSystem::DestroyHeightfields()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  for (auto& patch : m_Heightfields)
  {
    pDevice->DestroyBuffer(patch.m_hBakedHeights);
    pDevice->DestroyBuffer(patch.m_hBakedNormals);
    pDevice->DestroyBuffer(patch.m_hCellMaterials);
    pDevice->DestroyBuffer(patch.m_hVertexWeights);
  }
  m_Heightfields.Clear();
}

void ezTerrainSystem::DestroySharedHeightfieldScratch()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  pDevice->DestroyBuffer(m_hHeightfieldSharedMask);

  m_uiHeightfieldSharedMaskStoredSize = 0;
}

void ezTerrainSystem::UpdateHeightfield(ezUInt32 uiIndex, ezRenderGraph& graph)
{
  EZ_PROFILE_SCOPE("UpdateHeightfield");

  auto& patch = m_Heightfields[uiIndex];
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  ezTempHybridArray<TerrainBrushData, 16> brushCPUData;
  FindHeightfieldOverlappingBrushes(patch, brushCPUData);
  const ezUInt32 uiNumBrushes = brushCPUData.GetCount();

  ezGALBufferHandle hBrushBuffer = CreateBrushBuffer(brushCPUData, pDevice);

  // Build source height data by sampling the height image (or zeros if no image is assigned).
  // Stored grid = (CellsPerSide+9)² — 4 border rings on each side beyond the render vertices.
  const ezUInt32 uiStoredSize = patch.m_uiCellsPerSide + 9;
  const ezUInt32 uiVertexCount = patch.m_uiCellsPerSide + 1; // vertices per side (quads+1)
  ezTempArray<float> srcHeights;
  srcHeights.SetCount(uiStoredSize * uiStoredSize, 0.0f);

  if (patch.m_hHeightImage.IsValid() && patch.m_fHeightScale > 0.0f)
  {
    ezResourceLock<ezImageDataResource> imgLock(patch.m_hHeightImage, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (imgLock.GetAcquireResult() == ezResourceAcquireResult::Final)
    {
      const ezImage& img = imgLock->GetDescriptor().m_Image;
      const ezColor* pPixels = img.GetPixelPointer<ezColor>();
      const ezUInt32 uiImgW = img.GetWidth();
      const ezUInt32 uiImgH = img.GetHeight();
      const float fInvVertexCount = (uiVertexCount > 1) ? 1.0f / static_cast<float>(uiVertexCount - 1) : 0.0f;

      for (ezUInt32 sRow = 0; sRow < uiStoredSize; ++sRow)
      {
        for (ezUInt32 sCol = 0; sCol < uiStoredSize; ++sCol)
        {
          const float u = patch.m_vImageOffset.x + static_cast<float>(static_cast<ezInt32>(sCol) - 4) * fInvVertexCount * patch.m_vImageSize.x;
          const float v = patch.m_vImageOffset.y + static_cast<float>(static_cast<ezInt32>(sRow) - 4) * fInvVertexCount * patch.m_vImageSize.y;
          srcHeights[sRow * uiStoredSize + sCol] = ezImageUtils::BilinearSample(pPixels, uiImgW, uiImgH, ezImageAddressMode::Clamp, ezVec2(u, v)).r * patch.m_fHeightScale;
        }
      }
    }
  }

  // Create source heights buffer with initial data (no separate UpdateBuffer needed).
  ezGALBufferCreationDescription srcDesc;
  srcDesc.m_uiStructSize = sizeof(float);
  srcDesc.m_uiTotalSize = srcHeights.GetCount() * sizeof(float);
  srcDesc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource;
  srcDesc.m_ResourceAccess.m_bImmutable = false;

  ezGALBufferHandle hSourceBuffer = pDevice->CreateBuffer(srcDesc, ezArrayPtr<const ezUInt8>(reinterpret_cast<const ezUInt8*>(srcHeights.GetData()), srcHeights.GetCount() * sizeof(float)));

  const ezUInt32 uiGroups = (uiStoredSize + 15) / 16;

  // The intermediate mask is shared bake scratch — grow it to fit this patch's stored grid.
  EnsureSharedHeightfieldScratch(uiStoredSize);

  // Import all buffers into the graph.
  auto hGraphSrc = graph.ImportBuffer(hSourceBuffer);
  auto hGraphBrush = graph.ImportBuffer(hBrushBuffer);
  auto hGraphBakedH = graph.ImportBuffer(patch.m_hBakedHeights);
  auto hGraphBakedM = graph.ImportBuffer(m_hHeightfieldSharedMask);
  auto hGraphBakedN = graph.ImportBuffer(patch.m_hBakedNormals);

  // Pass 1: bake heights from source + brushes into BakedHeights (UAV) and BakedMask (UAV).
  {
    HeightfieldBakeConstants c1;
    c1.GridSpacing = patch.m_fGridSpacing;
    c1.VertexIdxPitch = uiStoredSize;
    c1.BrushCount = uiNumBrushes;
    c1.PatchOrigin = patch.m_GlobalTransform.m_vPosition.GetAsVec2();

    auto pass = graph.AddComputePass("TerrainHFBakeStep1");
    pass.ReadBuffer(hGraphSrc);
    pass.ReadBuffer(hGraphBrush);
    pass.WriteBuffer(hGraphBakedH);
    pass.WriteBuffer(hGraphBakedM);
    pass.HasSideEffects();
    pass.SetExecuteCallback([this, c1, uiGroups, hGraphSrc, hGraphBrush, hGraphBakedH, hGraphBakedM](const ezRenderGraphContext& ctx)
      {
        auto* pRC = ctx.GetRenderContext();
        const bool bPrevAsync = pRC->GetAllowAsyncShaderLoading();
        pRC->SetAllowAsyncShaderLoading(false);
        EZ_SCOPE_EXIT(pRC->SetAllowAsyncShaderLoading(bPrevAsync));

        HeightfieldBakeConstants* constants = ezRenderContext::GetConstantBufferData<HeightfieldBakeConstants>(m_hHeightfieldBakeConstants);
        *constants = c1;
        pRC->BindShader(m_hTerrainBakeStep1Shader);
        ezBindGroupBuilder& bg = pRC->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);
        bg.BindBuffer("SourceHeights", ctx.ResolveBuffer(hGraphSrc));
        bg.BindBuffer("BakedHeights", ctx.ResolveBuffer(hGraphBakedH));
        bg.BindBuffer("BakedMask", ctx.ResolveBuffer(hGraphBakedM));
        bg.BindBuffer("Brushes", ctx.ResolveBuffer(hGraphBrush));
        bg.BindBuffer("HeightfieldBakeConstants", m_hHeightfieldBakeConstants);
        pRC->Dispatch(uiGroups, uiGroups, 1).AssertSuccess(); });
  }

  // Pass 2: derive normals from BakedHeights (SRV) into BakedNormals (UAV).
  // The graph inserts a UAV→SRV barrier on BakedHeights between pass 1 and pass 2.
  {
    HeightfieldBakeConstants c2;
    c2.GridSpacing = patch.m_fGridSpacing;
    c2.VertexIdxPitch = uiStoredSize;
    c2.DefaultMaterialIndex = patch.m_uiDefaultMaterialIndex;

    auto pass = graph.AddComputePass("TerrainHFBakeStep2");
    pass.ReadBuffer(hGraphBakedH);
    pass.WriteBuffer(hGraphBakedM); // RWStructuredBuffer: normalizes weights in-place, needs UAV state
    pass.WriteBuffer(hGraphBakedN);
    pass.HasSideEffects();
    pass.SetExecuteCallback([this, c2, uiGroups, hGraphBakedH, hGraphBakedN, hGraphBakedM](const ezRenderGraphContext& ctx)
      {
        auto* pRC = ctx.GetRenderContext();
        const bool bPrevAsync = pRC->GetAllowAsyncShaderLoading();
        pRC->SetAllowAsyncShaderLoading(false);
        EZ_SCOPE_EXIT(pRC->SetAllowAsyncShaderLoading(bPrevAsync));

        HeightfieldBakeConstants* constants = ezRenderContext::GetConstantBufferData<HeightfieldBakeConstants>(m_hHeightfieldBakeConstants);
        *constants = c2;
        pRC->BindShader(m_hTerrainBakeStep2Shader);
        ezBindGroupBuilder& bg = pRC->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);
        bg.BindBuffer("BakedHeights", ctx.ResolveBuffer(hGraphBakedH));
        bg.BindBuffer("BakedNormals", ctx.ResolveBuffer(hGraphBakedN));
        bg.BindBuffer("BakedMask", ctx.ResolveBuffer(hGraphBakedM));
        bg.BindBuffer("HeightfieldBakeConstants", m_hHeightfieldBakeConstants);
        pRC->Dispatch(uiGroups, uiGroups, 1).AssertSuccess(); });
  }

  // Pass 3: read finalized BakedMask (SRV) → write CellMaterials + VertexWeights (UAVs).
  // The graph inserts a UAV→SRV barrier on BakedMask between Pass2 and Pass3.
  {
    HeightfieldBakeConstants c3;
    c3.VertexIdxPitch = uiStoredSize;
    c3.CellsPerSide = patch.m_uiCellsPerSide;

    const ezUInt32 uiCellGroups = (patch.m_uiCellsPerSide + 15) / 16;

    auto hGraphCellMat = graph.ImportBuffer(patch.m_hCellMaterials);
    auto hGraphVtxW = graph.ImportBuffer(patch.m_hVertexWeights);

    auto pass = graph.AddComputePass("TerrainHFBakeStep3");
    pass.ReadBuffer(hGraphBakedM);
    pass.WriteBuffer(hGraphCellMat);
    pass.WriteBuffer(hGraphVtxW);
    pass.HasSideEffects();
    pass.SetExecuteCallback([this, c3, uiCellGroups, hGraphBakedM, hGraphCellMat, hGraphVtxW](const ezRenderGraphContext& ctx)
      {
        auto* pRC = ctx.GetRenderContext();
        const bool bPrevAsync = pRC->GetAllowAsyncShaderLoading();
        pRC->SetAllowAsyncShaderLoading(false);
        EZ_SCOPE_EXIT(pRC->SetAllowAsyncShaderLoading(bPrevAsync));

        HeightfieldBakeConstants* constants = ezRenderContext::GetConstantBufferData<HeightfieldBakeConstants>(m_hHeightfieldBakeConstants);
        *constants = c3;
        pRC->BindShader(m_hTerrainBakeStep3Shader);
        ezBindGroupBuilder& bg = pRC->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);
        bg.BindBuffer("BakedMask", ctx.ResolveBuffer(hGraphBakedM));
        bg.BindBuffer("CellMaterials", ctx.ResolveBuffer(hGraphCellMat));
        bg.BindBuffer("VertexWeights", ctx.ResolveBuffer(hGraphVtxW));
        bg.BindBuffer("HeightfieldBakeConstants", m_hHeightfieldBakeConstants);
        pRC->Dispatch(uiCellGroups, uiCellGroups, 1).AssertSuccess(); });
  }

  // Deferred deletion: GAL delays destruction by several frames, so the callbacks will still see valid data.
  pDevice->DestroyBuffer(hSourceBuffer);
  pDevice->DestroyBuffer(hBrushBuffer);

  patch.m_bDirty = false;
}

ezResult ezTerrainSystem::ReadbackHeightfieldData(ezUInt32 uiPatchIndex, ezDynamicArray<float>& out_heights, ezDynamicArray<ezUInt8>& out_dominantMat, ezTime timeout)
{
  if (uiPatchIndex >= m_Heightfields.GetCount())
    return EZ_FAILURE;

  auto& patch = m_Heightfields[uiPatchIndex];
  if (!patch.m_bInUse || patch.m_hBakedHeights.IsInvalidated())
    return EZ_FAILURE;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // The mask is shared scratch, so it is not persistent — always re-bake and read back within one sync graph.
  patch.m_bDirty = true;

  ezGALReadbackBufferHelper heightRB;
  ezGALReadbackBufferHelper maskRB;

  auto pGraph = ezRenderGraphManager::CreateRenderGraph("TerrainHFExportReadback", ezRenderGraphPhase::PreRender);
  pGraph->Reset();

  UpdateHeightfield(uiPatchIndex, *pGraph);

  // Re-import the same buffers (ImportBuffer dedupes by handle) to add the readback transfer pass.
  auto hGraphBakedH = pGraph->ImportBuffer(patch.m_hBakedHeights);
  auto hGraphBakedM = pGraph->ImportBuffer(m_hHeightfieldSharedMask);

  const ezGALBufferHandle hBH = patch.m_hBakedHeights;
  const ezGALBufferHandle hBM = m_hHeightfieldSharedMask;

  auto pass = pGraph->AddTransferPass("TerrainHFReadback");
  pass.ReadBuffer(hGraphBakedH, ezGALResourceState::CopySource);
  pass.ReadBuffer(hGraphBakedM, ezGALResourceState::CopySource);
  pass.HasSideEffects();
  pass.SetExecuteCallback([hBH, hBM, &heightRB, &maskRB](const ezRenderGraphContext& ctx)
    {
      heightRB.ReadbackBuffer(*ctx.GetCommandEncoder(), hBH);
      maskRB.ReadbackBuffer(*ctx.GetCommandEncoder(), hBM); });

  ExecuteGraphSync(*pGraph, pDevice);

  const ezTime tDeadline = ezTime::Now() + timeout;

  auto PollReadback = [&](ezGALReadbackBufferHelper& readback, const char* szName) -> ezResult
  {
    while (true)
    {
      const auto result = readback.GetReadbackResult(ezTime::MakeFromMilliseconds(2));
      if (result == ezGALAsyncResult::Expired)
      {
        ezLog::Error("ReadbackHeightfieldData: {} readback expired for patch {}.", szName, uiPatchIndex);
        return EZ_FAILURE;
      }
      if (result == ezGALAsyncResult::Ready)
        return EZ_SUCCESS;
      if (ezTime::Now() >= tDeadline)
      {
        ezLog::Error("ReadbackHeightfieldData: timed out waiting for {} of patch {}.", szName, uiPatchIndex);
        return EZ_FAILURE;
      }
    }
  };

  if (PollReadback(heightRB, "heights").Failed())
    return EZ_FAILURE;
  if (PollReadback(maskRB, "mask").Failed())
    return EZ_FAILURE;

  const ezUInt32 S = patch.m_uiCellsPerSide + 9;

  {
    ezArrayPtr<const ezUInt8> rawMemory;
    auto lock = heightRB.LockBuffer(rawMemory);
    if (!lock)
      return EZ_FAILURE;

    const float* pStoredHeights = reinterpret_cast<const float*>(rawMemory.GetPtr());
    out_heights.SetCountUninitialized(S * S);
    ezMemoryUtils::Copy(out_heights.GetData(), pStoredHeights, S * S);
  }

  {
    ezArrayPtr<const ezUInt8> rawMemory;
    auto lock = maskRB.LockBuffer(rawMemory);
    if (lock)
    {
      const ezUInt32* pStoredMask = reinterpret_cast<const ezUInt32*>(rawMemory.GetPtr());
      out_dominantMat.SetCountUninitialized(S * S);
      for (ezUInt32 i = 0; i < S * S; ++i)
        out_dominantMat[i] = static_cast<ezUInt8>(pStoredMask[i * 2] & 0xFFu);
    }
  }

  return EZ_SUCCESS;
}
