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
#include <Shaders/Terrain/Generation/VoxelBakeConstants.h>
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

  m_hVoxelBakeShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Terrain/Generation/VoxelBakeCS.ezShader");
  m_hVoxelMeshClearShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Terrain/Generation/VoxelMeshClearCS.ezShader");
  m_hVoxelSurfaceNetsPass1Shader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Terrain/Generation/VoxelSurfaceNetsPass1CS.ezShader");
  m_hVoxelSurfaceNetsPass2Shader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Terrain/Generation/VoxelSurfaceNetsPass2CS.ezShader");
  m_hVoxelBlurDistShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Terrain/Generation/VoxelBlurDistCS.ezShader");
  m_hVoxelCleanupShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Terrain/Generation/VoxelCleanupCS.ezShader");
  m_hVoxelFillCompactCopyArgsShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Terrain/Generation/VoxelFillCompactCopyArgsCS.ezShader");
  m_hVoxelCompactCopyShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Terrain/Generation/VoxelCompactCopyCS.ezShader");
  m_hVoxelBakeConstants = ezRenderContext::CreateConstantBufferStorage<VoxelBakeConstants>();
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

  ezRenderContext::DeleteConstantBufferStorage(m_hVoxelBakeConstants);
  DestroyVoxelVolumes();
  DestroySharedVoxelScratch();

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

  for (ezUInt32 idx : m_QueuedVoxelVolumesToDelete)
  {
    DestroyVoxelTerrain(idx);
  }
  m_QueuedVoxelVolumesToDelete.Clear();
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
  {
    for (const auto& vol : m_VoxelVolumes)
    {
      if (vol.m_bInUse && vol.m_bDirty)
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

  for (ezUInt32 i = 0; i < m_VoxelVolumes.GetCount(); ++i)
  {
    ezTerrainData_Voxel& vol = m_VoxelVolumes[i];
    if (!vol.m_bInUse)
      continue;

    bool bShouldBake = vol.m_bDirty;
    ezUInt64 uiNewHash = vol.m_uiBrushOverlapHash;
    if (m_bBrushesDirty)
    {
      uiNewHash = ComputeVoxelBrushOverlapHash(vol);

      if (uiNewHash != vol.m_uiBrushOverlapHash)
        bShouldBake = true;
    }

    if (bShouldBake)
    {
      vol.m_uiBrushOverlapHash = uiNewHash;
      UpdateVoxels(i, *m_pRenderGraph);
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
    if (!brush.m_bInUse || !brush.m_bAffectHeightfields)
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
    if (!brush.m_bInUse || !brush.m_bAffectHeightfields)
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

//////////////////////////////////////////////////////////////////////////
// Voxel Terrain
//////////////////////////////////////////////////////////////////////////

EZ_DEFINE_AS_POD_TYPE(VoxelGpuVertex);

ezUInt32 ezTerrainSystem::CreateVoxelTerrain(ezUInt32 uiResolution, float fVoxelSize)
{
  EZ_PROFILE_SCOPE("CreateVoxelTerrain");

  ezUInt32 uiIndex = ezInvalidIndex;
  for (ezUInt32 i = 0; i < m_VoxelVolumes.GetCount(); ++i)
  {
    if (!m_VoxelVolumes[i].m_bInUse)
    {
      uiIndex = i;
      break;
    }
  }
  if (uiIndex == ezInvalidIndex)
  {
    uiIndex = m_VoxelVolumes.GetCount();
    m_VoxelVolumes.ExpandAndGetRef();
  }

  constexpr ezUInt32 c_uiVoxelBorderVoxels = 4u; // border voxels on each side of the inner volume
  ezTerrainData_Voxel& vol = m_VoxelVolumes[uiIndex];
  vol.m_uiResolution = uiResolution;
  // X: add border on both sides, then align to multiple of 8 (required for voxel packing — 8 per uint).
  // Y/Z: add border on both sides, no alignment needed.
  const ezUInt32 uiBufX = ((uiResolution + 2u * c_uiVoxelBorderVoxels + 7u) & ~7u);
  vol.m_uiPackPitch = uiBufX / 8u;
  vol.m_fVoxelSize = fVoxelSize;
  vol.m_bInUse = true;
  vol.m_bDirty = true;
  vol.m_uiBrushOverlapHash = 0;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // Only the final render buffers are held per piece; all bake scratch lives in the shared set on the system.
  // Cells = (N+1)³ where N = uiResolution; indices worst-case = cells * 18.
  const ezUInt32 uiCells = vol.m_uiResolution + 1u;
  const ezUInt32 uiMaxCells = uiCells * uiCells * uiCells;
  const ezUInt32 uiMaxIndices = uiMaxCells * 18u;

  // Final render buffers — worst-case sized so rendering can begin on the first bake without a count readback;
  // the GPU-driven compact-copy only writes the used entries and DrawArgs controls the draw count.
  {
    ezGALBufferCreationDescription bufDesc;
    bufDesc.m_uiStructSize = sizeof(VoxelGpuVertex);
    bufDesc.m_uiTotalSize = uiMaxCells * sizeof(VoxelGpuVertex);
    bufDesc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::UnorderedAccess | ezGALBufferUsageFlags::ShaderResource;
    bufDesc.m_ResourceAccess.m_bImmutable = false;
    vol.m_hFinalVertices = pDevice->CreateBuffer(bufDesc);
  }
  {
    ezGALBufferCreationDescription bufDesc;
    bufDesc.m_uiStructSize = sizeof(ezUInt32);
    bufDesc.m_uiTotalSize = uiMaxIndices * sizeof(ezUInt32);
    bufDesc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::UnorderedAccess | ezGALBufferUsageFlags::ShaderResource;
    bufDesc.m_ResourceAccess.m_bImmutable = false;
    vol.m_hFinalIndices = pDevice->CreateBuffer(bufDesc);
  }
  {
    // DrawArgs: 4 uints {IndexCount, 1, 0, 0}. ByteAddressBuffer + DrawIndirect.
    ezGALBufferCreationDescription bufDesc;
    bufDesc.m_uiStructSize = sizeof(ezUInt32);
    bufDesc.m_uiTotalSize = 4u * sizeof(ezUInt32);
    bufDesc.m_BufferFlags = ezGALBufferUsageFlags::ByteAddressBuffer | ezGALBufferUsageFlags::UnorderedAccess | ezGALBufferUsageFlags::DrawIndirect;
    bufDesc.m_ResourceAccess.m_bImmutable = false;
    const ezUInt32 zero[4] = {0, 1, 0, 0};
    vol.m_hFinalDrawArgs = pDevice->CreateBuffer(bufDesc, ezMakeArrayPtr(zero).ToByteArray());
  }

  return uiIndex;
}

void ezTerrainSystem::RemoveVoxelTerrain(ezUInt32 uiIndex)
{
  EZ_LOCK(m_Mutex);
  m_QueuedVoxelVolumesToDelete.PushBack(uiIndex);
}

ezTerrainData_Voxel& ezTerrainSystem::ModifyVoxelTerrain(ezUInt32 uiIndex)
{
  auto& vol = m_VoxelVolumes[uiIndex];
  vol.m_bDirty = true;
  return vol;
}

void ezTerrainSystem::DestroyVoxelTerrain(ezUInt32& uiIdx)
{
  if (uiIdx >= m_VoxelVolumes.GetCount())
    return;

  auto& data = m_VoxelVolumes[uiIdx];
  uiIdx = ezInvalidIndex;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  if (!data.m_hFinalVertices.IsInvalidated())
  {
    pDevice->DestroyBuffer(data.m_hFinalVertices);
    data.m_hFinalVertices.Invalidate();
  }
  if (!data.m_hFinalIndices.IsInvalidated())
  {
    pDevice->DestroyBuffer(data.m_hFinalIndices);
    data.m_hFinalIndices.Invalidate();
  }
  if (!data.m_hFinalDrawArgs.IsInvalidated())
  {
    pDevice->DestroyBuffer(data.m_hFinalDrawArgs);
    data.m_hFinalDrawArgs.Invalidate();
  }

  data.m_bInUse = false;
}

void ezTerrainSystem::EnsureSharedVoxelScratch(ezUInt32 uiPackPitch, ezUInt32 uiBufYZ, ezUInt32 uiResolution)
{
  if (uiPackPitch <= m_uiSharedVoxelPackPitch && uiBufYZ <= m_uiSharedVoxelBufYZ && uiResolution <= m_uiSharedVoxelResolution && !m_hSharedVoxels.IsInvalidated())
    return;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  DestroySharedVoxelScratch();

  // Grow to the maximum of the requested and previous dimensions so a smaller later bake never shrinks it.
  uiPackPitch = ezMath::Max(uiPackPitch, m_uiSharedVoxelPackPitch);
  uiBufYZ = ezMath::Max(uiBufYZ, m_uiSharedVoxelBufYZ);
  uiResolution = ezMath::Max(uiResolution, m_uiSharedVoxelResolution);

  const ezUInt32 uiBufX = uiPackPitch * 8u;
  const ezUInt32 uiPackedCount = uiPackPitch * uiBufYZ * uiBufYZ;
  const ezUInt32 uiCells = uiResolution + 1u;
  const ezUInt32 uiMaxCells = uiCells * uiCells * uiCells;
  const ezUInt32 uiMaxIndices = uiMaxCells * 18u;

  auto CreateBuf = [pDevice](ezUInt32 uiStructSize, ezUInt32 uiCount) -> ezGALBufferHandle
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = uiStructSize;
    desc.m_uiTotalSize = uiStructSize * uiCount;
    desc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource | ezGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceAccess.m_bImmutable = false;
    return pDevice->CreateBuffer(desc);
  };

  m_hSharedVoxels = CreateBuf(sizeof(ezUInt32), uiPackedCount);
  m_hSharedVoxelDist = CreateBuf(sizeof(float), uiBufX * uiBufYZ * uiBufYZ);
  m_hSharedVoxelDistScratch = CreateBuf(sizeof(float), uiBufX * uiBufYZ * uiBufYZ);
  m_hSharedMeshRemap = CreateBuf(sizeof(ezUInt32), uiMaxCells);
  m_hSharedMeshCompactVertices = CreateBuf(sizeof(VoxelGpuVertex), uiMaxCells);
  m_hSharedMeshIndices = CreateBuf(sizeof(ezUInt32), uiMaxIndices);
  m_hSharedMeshCounts = CreateBuf(sizeof(VoxelMeshCounts), 1);

  // DispatchIndirect args for VoxelCompactCopyCS: 3 uints {GroupsX, 1, 1}. Re-filled each bake.
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(ezUInt32);
    desc.m_uiTotalSize = 3u * sizeof(ezUInt32);
    desc.m_BufferFlags = ezGALBufferUsageFlags::ByteAddressBuffer | ezGALBufferUsageFlags::UnorderedAccess | ezGALBufferUsageFlags::DrawIndirect;
    desc.m_ResourceAccess.m_bImmutable = false;
    const ezUInt32 one[3] = {1, 1, 1};
    m_hSharedCompactCopyDispatchArgs = pDevice->CreateBuffer(desc, ezMakeArrayPtr(one).ToByteArray());
  }

  m_uiSharedVoxelPackPitch = uiPackPitch;
  m_uiSharedVoxelBufYZ = uiBufYZ;
  m_uiSharedVoxelResolution = uiResolution;
}

void ezTerrainSystem::DestroyVoxelVolumes()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  for (auto& vol : m_VoxelVolumes)
  {
    if (!vol.m_hFinalVertices.IsInvalidated())
      pDevice->DestroyBuffer(vol.m_hFinalVertices);
    if (!vol.m_hFinalIndices.IsInvalidated())
      pDevice->DestroyBuffer(vol.m_hFinalIndices);
    if (!vol.m_hFinalDrawArgs.IsInvalidated())
      pDevice->DestroyBuffer(vol.m_hFinalDrawArgs);
  }
  m_VoxelVolumes.Clear();
}

void ezTerrainSystem::DestroySharedVoxelScratch()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  ezGALBufferHandle* handles[] = {&m_hSharedVoxels, &m_hSharedVoxelDist, &m_hSharedVoxelDistScratch, &m_hSharedMeshRemap, &m_hSharedMeshCompactVertices, &m_hSharedMeshIndices, &m_hSharedMeshCounts, &m_hSharedCompactCopyDispatchArgs};
  for (ezGALBufferHandle* pHandle : handles)
  {
    if (!pHandle->IsInvalidated())
    {
      pDevice->DestroyBuffer(*pHandle);
      pHandle->Invalidate();
    }
  }

  m_uiSharedVoxelPackPitch = 0;
  m_uiSharedVoxelBufYZ = 0;
  m_uiSharedVoxelResolution = 0;
}

ezGALBufferHandle ezTerrainSystem::GetVoxelVolumeGpuMeshVertexBuffer(ezUInt32 uiIndex) const
{
  if (uiIndex < m_VoxelVolumes.GetCount())
  {
    const auto& vol = m_VoxelVolumes[uiIndex];
    return vol.m_hFinalVertices;
  }
  return {};
}

ezGALBufferHandle ezTerrainSystem::GetVoxelVolumeGpuMeshDrawArgsBuffer(ezUInt32 uiIndex) const
{
  if (uiIndex < m_VoxelVolumes.GetCount())
  {
    const auto& vol = m_VoxelVolumes[uiIndex];
    return vol.m_hFinalDrawArgs;
  }
  return {};
}

ezGALBufferHandle ezTerrainSystem::GetVoxelVolumeGpuMeshIndexBuffer(ezUInt32 uiIndex) const
{
  if (uiIndex < m_VoxelVolumes.GetCount())
  {
    const auto& vol = m_VoxelVolumes[uiIndex];
    return vol.m_hFinalIndices;
  }
  return {};
}

ezUInt64 ezTerrainSystem::GetVoxelBrushOverlapHash(ezUInt32 uiIndex) const
{
  if (uiIndex >= m_VoxelVolumes.GetCount())
    return 0;
  return m_VoxelVolumes[uiIndex].m_uiBrushOverlapHash;
}

ezUInt64 ezTerrainSystem::ComputeVoxelBrushOverlapHash(const ezTerrainData_Voxel& vol) const
{
  const ezTransform invTrans = vol.m_GlobalTransform.GetInverse();
  const float fSize = (float)vol.m_uiResolution * vol.m_fVoxelSize;

  ezHashStreamWriter64 writer;

  for (ezUInt32 i = 0; i < m_Brushes.GetCount(); ++i)
  {
    const auto& brush = m_Brushes[i];
    if (!brush.m_bInUse || !brush.m_bAffectVoxels)
      continue;

    if (!brush.m_Tags.IsEmpty() && !brush.m_Tags.IsAnySet(vol.m_Tags))
      continue;

    const ezVec3 vLocalCenter = invTrans * brush.m_vPosition;
    const float fConservativeRadius = ezMath::Sqrt(brush.m_vHalfExtents.x * brush.m_vHalfExtents.x + brush.m_vHalfExtents.y * brush.m_vHalfExtents.y + brush.m_fHalfExtentZ * brush.m_fHalfExtentZ) + brush.m_fInnerRadius + brush.m_fOuterRadius;

    const float fDx = vLocalCenter.x - ezMath::Clamp(vLocalCenter.x, 0.0f, fSize);
    const float fDy = vLocalCenter.y - ezMath::Clamp(vLocalCenter.y, 0.0f, fSize);
    // OnlyPaint2D projects from above with no Z extent — skip Z bounds check.
    const float fDz = (brush.m_ModifyMode == ezTerrainModifyMode::OnlyPaint2D) ? 0.0f : (vLocalCenter.z - ezMath::Clamp(vLocalCenter.z, 0.0f, fSize));
    if (fDx * fDx + fDy * fDy + fDz * fDz > fConservativeRadius * fConservativeRadius)
      continue;

    writer << i;
    writer << brush.m_vPosition.x << brush.m_vPosition.y << brush.m_vPosition.z;
    writer << brush.m_qRotation.x << brush.m_qRotation.y << brush.m_qRotation.z << brush.m_qRotation.w;
    writer << brush.m_vHalfExtents.x << brush.m_vHalfExtents.y;
    writer << brush.m_fHalfExtentZ << brush.m_fHalfExtentYTop;
    writer << brush.m_fInnerRadius << brush.m_fOuterRadius << brush.m_fFalloff;
    writer << brush.m_ModifyMode.GetValue();
    writer << brush.m_uiMaterialIndex << brush.m_fMaterialStrength;
    writer << brush.m_fNoiseStrength << brush.m_fNoiseFrequency;
    writer << brush.m_iPriority;
    brush.m_Tags.Save(writer);
  }

  writer << vol.m_fFillHeight;
  vol.m_Tags.Save(writer);

  return writer.GetHashValue();
}

void ezTerrainSystem::FindVoxelOverlappingBrushes(const ezTerrainData_Voxel& vol, ezDynamicArray<TerrainBrushData>& brushes) const
{
  const ezTransform invTrans = vol.m_GlobalTransform.GetInverse();
  const float fSize = (float)vol.m_uiResolution * vol.m_fVoxelSize;

  brushes.Clear();
  brushes.Reserve(m_Brushes.GetCount());

  const ezMat3 mInvVolumeRot = vol.m_GlobalTransform.m_qRotation.GetAsMat3().GetTranspose();

  for (const auto& brush : m_Brushes)
  {
    if (!brush.m_bInUse || !brush.m_bAffectVoxels)
      continue;

    if (!brush.m_Tags.IsEmpty() && !brush.m_Tags.IsAnySet(vol.m_Tags))
      continue;

    if ((brush.m_ModifyMode == ezTerrainModifyMode::OnlyPaint2D || brush.m_ModifyMode == ezTerrainModifyMode::OnlyPaint3D) && brush.m_fMaterialStrength <= 0.0f)
      continue;

    const ezVec3 vLocalCenter = invTrans * brush.m_vPosition;
    const float fConservativeRadius = ezMath::Sqrt(brush.m_vHalfExtents.x * brush.m_vHalfExtents.x + brush.m_vHalfExtents.y * brush.m_vHalfExtents.y + brush.m_fHalfExtentZ * brush.m_fHalfExtentZ) + brush.m_fInnerRadius + brush.m_fOuterRadius;

    const float fDx = vLocalCenter.x - ezMath::Clamp(vLocalCenter.x, 0.0f, fSize);
    const float fDy = vLocalCenter.y - ezMath::Clamp(vLocalCenter.y, 0.0f, fSize);
    // OnlyPaint2D projects from above with no Z extent — skip Z bounds check.
    const float fDz = (brush.m_ModifyMode == ezTerrainModifyMode::OnlyPaint2D) ? 0.0f : (vLocalCenter.z - ezMath::Clamp(vLocalCenter.z, 0.0f, fSize));
    if (fDx * fDx + fDy * fDy + fDz * fDz > fConservativeRadius * fConservativeRadius)
      continue;

    TerrainBrushData& bd = brushes.ExpandAndGetRef();
    bd.Position = vLocalCenter;
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

    const ezMat3 mInvLocalBrushRot = (mInvVolumeRot * brush.m_qRotation.GetAsMat3()).GetTranspose();
    bd.InvRotRow0 = mInvLocalBrushRot.GetRow(0);
    bd.InvRotRow1 = mInvLocalBrushRot.GetRow(1);
    bd.InvRotRow2 = mInvLocalBrushRot.GetRow(2);
  }

  // Primary sort: priority ascending (higher priority = applied later = wins).
  // Within same priority: Carve last, then by mode number, then by height.
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
      if (a.ModifyMode == 1u)
        return a.Position.z > b.Position.z;
      return a.Position.z < b.Position.z; });
}

void ezTerrainSystem::UpdateVoxels(ezUInt32 uiIndex, ezRenderGraph& graph)
{
  EZ_PROFILE_SCOPE("UpdateVoxels");

  auto& vol = m_VoxelVolumes[uiIndex];
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  ezTempHybridArray<TerrainBrushData, 16> brushCPUData;
  FindVoxelOverlappingBrushes(vol, brushCPUData);

  const ezUInt32 uiNumBrushes = brushCPUData.GetCount();
  ezGALBufferHandle hBrushBuffer = CreateBrushBuffer(brushCPUData, pDevice);

  constexpr ezUInt32 c_uiVoxelBorderVoxels = 4u;

  const ezUInt32 uiResolution = vol.m_uiResolution;
  const ezUInt32 uiPackPitch = vol.m_uiPackPitch;
  const ezUInt32 uiBufYZ = uiResolution + 2u * c_uiVoxelBorderVoxels;
  const ezUInt32 uiGroupsX = (uiPackPitch + 3u) / 4u;
  const ezUInt32 uiGroupsYZ = (uiBufYZ + 3u) / 4u;

  // The bake scratch is shared across all volumes — grow it to fit this volume before importing.
  EnsureSharedVoxelScratch(uiPackPitch, uiBufYZ, uiResolution);

  // Import all buffers. Default states: UAV for SRV|UAV buffers, SRV for brush. Scratch is the shared set;
  // only the Final* buffers are per volume.
  auto hGraphBrush = graph.ImportBuffer(hBrushBuffer, ezGALResourceState::ShaderResource);
  auto hGraphBakedVoxels = graph.ImportBuffer(m_hSharedVoxels);
  auto hGraphBakedVoxelDist = graph.ImportBuffer(m_hSharedVoxelDist);
  auto hGraphBakedVoxelDistScratch = graph.ImportBuffer(m_hSharedVoxelDistScratch);
  auto hGraphGpuMeshRemap = graph.ImportBuffer(m_hSharedMeshRemap);
  auto hGraphGpuMeshCompactVerts = graph.ImportBuffer(m_hSharedMeshCompactVertices);
  auto hGraphGpuMeshIndices = graph.ImportBuffer(m_hSharedMeshIndices);
  auto hGraphGpuMeshCounts = graph.ImportBuffer(m_hSharedMeshCounts);
  auto hGraphCompactCopyDispatchArgs = graph.ImportBuffer(m_hSharedCompactCopyDispatchArgs);
  auto hGraphFinalVertices = graph.ImportBuffer(vol.m_hFinalVertices);
  auto hGraphFinalIndices = graph.ImportBuffer(vol.m_hFinalIndices);
  auto hGraphFinalDrawArgs = graph.ImportBuffer(vol.m_hFinalDrawArgs);

  // Bake pass: writes BakedVoxels (UAV) and BakedVoxelDist (UAV) from brushes.
  {
    VoxelBakeConstants c = {};
    c.GridSpacing = vol.m_fVoxelSize;
    c.VoxelResolution = ezVec3U32(uiResolution, uiResolution, uiResolution);
    c.BufferSize = ezVec3U32(uiPackPitch * 8u, uiBufYZ, uiBufYZ);
    c.NumBorderVoxels = c_uiVoxelBorderVoxels;
    c.BrushCount = uiNumBrushes;
    c.InitialSolid = vol.m_bInitialSolid ? 1 : 0;
    c.FillHeight = vol.m_fFillHeight;
    c.PatchOrigin = vol.m_GlobalTransform.m_vPosition;

    auto pass = graph.AddComputePass("VoxelBake");
    pass.ReadBuffer(hGraphBrush);
    pass.WriteBuffer(hGraphBakedVoxels);
    pass.WriteBuffer(hGraphBakedVoxelDist);
    pass.HasSideEffects();
    pass.SetExecuteCallback([this, c, uiGroupsX, uiGroupsYZ, hGraphBrush, hGraphBakedVoxels, hGraphBakedVoxelDist](const ezRenderGraphContext& ctx)
      {
        auto* pRC = ctx.GetRenderContext();
        const bool bPrev = pRC->GetAllowAsyncShaderLoading();
        pRC->SetAllowAsyncShaderLoading(false);
        EZ_SCOPE_EXIT(pRC->SetAllowAsyncShaderLoading(bPrev));

        VoxelBakeConstants* constants = ezRenderContext::GetConstantBufferData<VoxelBakeConstants>(m_hVoxelBakeConstants);
        *constants = c;
        pRC->BindShader(m_hVoxelBakeShader);
        ezBindGroupBuilder& bg = pRC->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);
        bg.BindBuffer("BakedVoxels",    ctx.ResolveBuffer(hGraphBakedVoxels));
        bg.BindBuffer("BakedVoxelDist", ctx.ResolveBuffer(hGraphBakedVoxelDist));
        bg.BindBuffer("Brushes",        ctx.ResolveBuffer(hGraphBrush));
        bg.BindBuffer("VoxelBakeConstants", m_hVoxelBakeConstants);
        pRC->Dispatch(uiGroupsX, uiGroupsYZ, uiGroupsYZ).AssertSuccess(); });
  }

  pDevice->DestroyBuffer(hBrushBuffer);
  vol.m_bDirty = false;

  // SDF blur: reads BakedVoxelDist (SRV), writes BakedVoxelDistScratch (UAV), reads BakedVoxels (SRV).
  {
    VoxelBakeConstants c = {};
    c.VoxelResolution = ezVec3U32(uiResolution, uiResolution, uiResolution);
    c.BufferSize = ezVec3U32(uiPackPitch * 8u, uiBufYZ, uiBufYZ);
    c.NumBorderVoxels = c_uiVoxelBorderVoxels;
    c.SmoothFactor = 0.3f;

    auto pass = graph.AddComputePass("VoxelBlurDist");
    pass.ReadBuffer(hGraphBakedVoxelDist);
    pass.ReadBuffer(hGraphBakedVoxels);
    pass.WriteBuffer(hGraphBakedVoxelDistScratch);
    pass.HasSideEffects();
    pass.SetExecuteCallback([this, c, uiGroupsX, uiGroupsYZ, hGraphBakedVoxelDist, hGraphBakedVoxelDistScratch, hGraphBakedVoxels](const ezRenderGraphContext& ctx)
      {
        auto* pRC = ctx.GetRenderContext();
        const bool bPrev = pRC->GetAllowAsyncShaderLoading();
        pRC->SetAllowAsyncShaderLoading(false);
        EZ_SCOPE_EXIT(pRC->SetAllowAsyncShaderLoading(bPrev));

        VoxelBakeConstants* constants = ezRenderContext::GetConstantBufferData<VoxelBakeConstants>(m_hVoxelBakeConstants);
        *constants = c;
        pRC->BindShader(m_hVoxelBlurDistShader);
        ezBindGroupBuilder& bg = pRC->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);
        bg.BindBuffer("BakedVoxelDistIn",  ctx.ResolveBuffer(hGraphBakedVoxelDist));
        bg.BindBuffer("BakedVoxelDistOut", ctx.ResolveBuffer(hGraphBakedVoxelDistScratch));
        bg.BindBuffer("BakedVoxels",       ctx.ResolveBuffer(hGraphBakedVoxels));
        bg.BindBuffer("VoxelBakeConstants", m_hVoxelBakeConstants);
        pRC->Dispatch(uiGroupsX, uiGroupsYZ, uiGroupsYZ).AssertSuccess(); });
  }

  // Topology cleanup iterations: ping-pong between the two dist buffers.
  // After blur, Scratch holds the smoothed result so that is the initial src.
  auto hGraphCleanupSrc = hGraphBakedVoxelDistScratch;
  auto hGraphCleanupDst = hGraphBakedVoxelDist;
  const ezUInt32 uiCleanupIterations = vol.m_uiCleanupIterations;

  for (ezUInt32 it = 0; it < uiCleanupIterations; ++it)
  {
    VoxelBakeConstants c = {};
    c.GridSpacing = vol.m_fVoxelSize;
    c.VoxelResolution = ezVec3U32(uiResolution, uiResolution, uiResolution);
    c.BufferSize = ezVec3U32(uiPackPitch * 8u, uiBufYZ, uiBufYZ);
    c.NumBorderVoxels = c_uiVoxelBorderVoxels;

    auto hSrc = hGraphCleanupSrc;
    auto hDst = hGraphCleanupDst;

    auto pass = graph.AddComputePass("VoxelCleanup");
    pass.ReadBuffer(hSrc);
    pass.WriteBuffer(hDst);
    pass.ReadBuffer(hGraphBakedVoxels);
    pass.HasSideEffects();
    pass.SetExecuteCallback([this, c, uiGroupsX, uiGroupsYZ, hSrc, hDst, hGraphBakedVoxels](const ezRenderGraphContext& ctx)
      {
        auto* pRC = ctx.GetRenderContext();
        const bool bPrev = pRC->GetAllowAsyncShaderLoading();
        pRC->SetAllowAsyncShaderLoading(false);
        EZ_SCOPE_EXIT(pRC->SetAllowAsyncShaderLoading(bPrev));

        VoxelBakeConstants* constants = ezRenderContext::GetConstantBufferData<VoxelBakeConstants>(m_hVoxelBakeConstants);
        *constants = c;
        pRC->BindShader(m_hVoxelCleanupShader);
        ezBindGroupBuilder& bg = pRC->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);
        bg.BindBuffer("BakedVoxelDistIn",  ctx.ResolveBuffer(hSrc));
        bg.BindBuffer("BakedVoxelDistOut", ctx.ResolveBuffer(hDst));
        bg.BindBuffer("BakedVoxels",       ctx.ResolveBuffer(hGraphBakedVoxels));
        bg.BindBuffer("VoxelBakeConstants", m_hVoxelBakeConstants);
        pRC->Dispatch(uiGroupsX, uiGroupsYZ, uiGroupsYZ).AssertSuccess(); });

    ezMath::Swap(hGraphCleanupSrc, hGraphCleanupDst);
  }

  // After cleanup, hGraphCleanupSrc holds the most recent output.
  const auto hGraphFinalVoxelDist = hGraphCleanupSrc;

  // Clear mesh counts before surface nets atomic-adds.
  {
    auto pass = graph.AddComputePass("VoxelMeshClear");
    pass.WriteBuffer(hGraphGpuMeshCounts);
    pass.HasSideEffects();
    pass.SetExecuteCallback([this, hGraphGpuMeshCounts](const ezRenderGraphContext& ctx)
      {
        auto* pRC = ctx.GetRenderContext();
        const bool bPrev = pRC->GetAllowAsyncShaderLoading();
        pRC->SetAllowAsyncShaderLoading(false);
        EZ_SCOPE_EXIT(pRC->SetAllowAsyncShaderLoading(bPrev));

        pRC->BindShader(m_hVoxelMeshClearShader);
        pRC->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL).BindBuffer("OutCounts", ctx.ResolveBuffer(hGraphGpuMeshCounts));
        pRC->Dispatch(1, 1, 1).AssertSuccess(); });
  }

  // Surface Nets Pass 1: scatter vertices into compact slots, record remapping.
  const ezUInt32 uiPass1Groups = (uiResolution + 1u + 3u) / 4u;
  {
    VoxelBakeConstants c = {};
    c.GridSpacing = vol.m_fVoxelSize;
    c.VoxelResolution = ezVec3U32(uiResolution, uiResolution, uiResolution);
    c.BufferSize = ezVec3U32(uiPackPitch * 8u, uiBufYZ, uiBufYZ);
    c.NumBorderVoxels = c_uiVoxelBorderVoxels;

    auto pass = graph.AddComputePass("VoxelSurfaceNetsPass1");
    pass.ReadBuffer(hGraphBakedVoxels);
    pass.ReadBuffer(hGraphFinalVoxelDist);
    pass.WriteBuffer(hGraphGpuMeshRemap);
    pass.WriteBuffer(hGraphGpuMeshCompactVerts);
    pass.WriteBuffer(hGraphGpuMeshCounts);
    pass.HasSideEffects();
    pass.SetExecuteCallback([this, c, uiPass1Groups, hGraphBakedVoxels, hGraphFinalVoxelDist, hGraphGpuMeshRemap, hGraphGpuMeshCompactVerts, hGraphGpuMeshCounts](const ezRenderGraphContext& ctx)
      {
        auto* pRC = ctx.GetRenderContext();
        const bool bPrev = pRC->GetAllowAsyncShaderLoading();
        pRC->SetAllowAsyncShaderLoading(false);
        EZ_SCOPE_EXIT(pRC->SetAllowAsyncShaderLoading(bPrev));

        VoxelBakeConstants* constants = ezRenderContext::GetConstantBufferData<VoxelBakeConstants>(m_hVoxelBakeConstants);
        *constants = c;
        pRC->BindShader(m_hVoxelSurfaceNetsPass1Shader);
        ezBindGroupBuilder& bg = pRC->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);
        bg.BindBuffer("BakedVoxels",        ctx.ResolveBuffer(hGraphBakedVoxels));
        bg.BindBuffer("BakedVoxelDist",     ctx.ResolveBuffer(hGraphFinalVoxelDist));
        bg.BindBuffer("OutRemap",           ctx.ResolveBuffer(hGraphGpuMeshRemap));
        bg.BindBuffer("OutCompactVertices", ctx.ResolveBuffer(hGraphGpuMeshCompactVerts));
        bg.BindBuffer("OutCounts",          ctx.ResolveBuffer(hGraphGpuMeshCounts));
        bg.BindBuffer("VoxelBakeConstants", m_hVoxelBakeConstants);
        pRC->Dispatch(uiPass1Groups, uiPass1Groups, uiPass1Groups).AssertSuccess(); });
  }

  // Surface Nets Pass 2: emit indices for each axis (3 separate passes).
  // Reading OutRemap (now InRemap as SRV) causes a UAV→SRV barrier, which serializes pass 1 writes.
  const ezUInt32 uiPass2Groups = (uiResolution + 1u + 3u) / 4u;
  for (ezUInt32 axis = 0; axis < 3; ++axis)
  {
    VoxelBakeConstants c = {};
    c.GridSpacing = vol.m_fVoxelSize;
    c.VoxelResolution = ezVec3U32(uiResolution, uiResolution, uiResolution);
    c.BufferSize = ezVec3U32(uiPackPitch * 8u, uiBufYZ, uiBufYZ);
    c.NumBorderVoxels = c_uiVoxelBorderVoxels;
    c.EdgeAxis = axis;

    auto pass = graph.AddComputePass("VoxelSurfaceNetsPass2");
    pass.ReadBuffer(hGraphFinalVoxelDist);
    pass.ReadBuffer(hGraphGpuMeshRemap);
    pass.WriteBuffer(hGraphGpuMeshIndices);
    pass.WriteBuffer(hGraphGpuMeshCounts);
    pass.HasSideEffects();
    pass.SetExecuteCallback([this, c, uiPass2Groups, hGraphBakedVoxels, hGraphFinalVoxelDist, hGraphGpuMeshRemap, hGraphGpuMeshIndices, hGraphGpuMeshCounts](const ezRenderGraphContext& ctx)
      {
        auto* pRC = ctx.GetRenderContext();
        const bool bPrev = pRC->GetAllowAsyncShaderLoading();
        pRC->SetAllowAsyncShaderLoading(false);
        EZ_SCOPE_EXIT(pRC->SetAllowAsyncShaderLoading(bPrev));

        VoxelBakeConstants* constants = ezRenderContext::GetConstantBufferData<VoxelBakeConstants>(m_hVoxelBakeConstants);
        *constants = c;
        pRC->BindShader(m_hVoxelSurfaceNetsPass2Shader);
        ezBindGroupBuilder& bg = pRC->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);
        bg.BindBuffer("BakedVoxels",    ctx.ResolveBuffer(hGraphBakedVoxels));
        bg.BindBuffer("BakedVoxelDist", ctx.ResolveBuffer(hGraphFinalVoxelDist));
        bg.BindBuffer("InRemap",        ctx.ResolveBuffer(hGraphGpuMeshRemap));
        bg.BindBuffer("OutIndices",     ctx.ResolveBuffer(hGraphGpuMeshIndices));
        bg.BindBuffer("OutCounts",      ctx.ResolveBuffer(hGraphGpuMeshCounts));
        bg.BindBuffer("VoxelBakeConstants", m_hVoxelBakeConstants);
        pRC->Dispatch(uiPass2Groups, uiPass2Groups, uiPass2Groups).AssertSuccess(); });
  }

  // Fill indirect dispatch args from GPU counts (written by surface nets passes above).
  {
    auto pass = graph.AddComputePass("VoxelFillCompactCopyArgs");
    pass.ReadBuffer(hGraphGpuMeshCounts);
    pass.WriteBuffer(hGraphCompactCopyDispatchArgs);
    pass.HasSideEffects();
    pass.SetExecuteCallback([this, hGraphGpuMeshCounts, hGraphCompactCopyDispatchArgs](const ezRenderGraphContext& ctx)
      {
        auto* pRC = ctx.GetRenderContext();
        const bool bPrev = pRC->GetAllowAsyncShaderLoading();
        pRC->SetAllowAsyncShaderLoading(false);
        EZ_SCOPE_EXIT(pRC->SetAllowAsyncShaderLoading(bPrev));

        pRC->BindShader(m_hVoxelFillCompactCopyArgsShader);
        ezBindGroupBuilder& bg = pRC->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);
        bg.BindBuffer("InCounts",        ctx.ResolveBuffer(hGraphGpuMeshCounts));
        bg.BindBuffer("OutDispatchArgs", ctx.ResolveBuffer(hGraphCompactCopyDispatchArgs));
        pRC->Dispatch(1, 1, 1).AssertSuccess(); });
  }

  // Compact-copy: GPU-driven DispatchIndirect copies only active entries to final render buffers.
  // The dispatch args buffer must be in DrawIndirect state for DispatchIndirect.
  {
    ezGALBufferHandle hDispatchArgs = m_hSharedCompactCopyDispatchArgs;

    auto pass = graph.AddComputePass("VoxelCompactCopy");
    pass.ReadBuffer(hGraphCompactCopyDispatchArgs, ezGALResourceState::DrawIndirect);
    pass.ReadBuffer(hGraphGpuMeshCompactVerts);
    pass.ReadBuffer(hGraphGpuMeshIndices);
    pass.ReadBuffer(hGraphGpuMeshCounts);
    pass.WriteBuffer(hGraphFinalVertices);
    pass.WriteBuffer(hGraphFinalIndices);
    pass.WriteBuffer(hGraphFinalDrawArgs);
    pass.HasSideEffects();
    pass.SetExecuteCallback([this, hDispatchArgs, hGraphGpuMeshCompactVerts, hGraphGpuMeshIndices, hGraphGpuMeshCounts, hGraphFinalVertices, hGraphFinalIndices, hGraphFinalDrawArgs](const ezRenderGraphContext& ctx)
      {
        auto* pRC = ctx.GetRenderContext();
        const bool bPrev = pRC->GetAllowAsyncShaderLoading();
        pRC->SetAllowAsyncShaderLoading(false);
        EZ_SCOPE_EXIT(pRC->SetAllowAsyncShaderLoading(bPrev));

        pRC->BindShader(m_hVoxelCompactCopyShader);
        ezBindGroupBuilder& bg = pRC->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);
        bg.BindBuffer("InCompactVertices", ctx.ResolveBuffer(hGraphGpuMeshCompactVerts));
        bg.BindBuffer("InIndices",         ctx.ResolveBuffer(hGraphGpuMeshIndices));
        bg.BindBuffer("InCounts",          ctx.ResolveBuffer(hGraphGpuMeshCounts));
        bg.BindBuffer("OutFinalVertices",  ctx.ResolveBuffer(hGraphFinalVertices));
        bg.BindBuffer("OutFinalIndices",   ctx.ResolveBuffer(hGraphFinalIndices));
        bg.BindBuffer("OutFinalDrawArgs",  ctx.ResolveBuffer(hGraphFinalDrawArgs));
        pRC->ApplyContextStates().AssertSuccess();
        ctx.GetCommandEncoder()->DispatchIndirect(hDispatchArgs, 0).AssertSuccess(); });
  }
}


ezResult ezTerrainSystem::ReadbackVoxelData(ezUInt32 uiIndex, ezTempArray<VoxelGpuVertex>& out_verts, ezDynamicArray<ezUInt32>& out_indices, ezUInt32& out_uiVertexCount, ezUInt32& out_uiPrimitiveCount, ezTime timeout)
{
  out_uiVertexCount = 0;
  out_uiPrimitiveCount = 0;

  if (uiIndex >= m_VoxelVolumes.GetCount())
    return EZ_FAILURE;

  auto& vol = m_VoxelVolumes[uiIndex];
  if (!vol.m_bInUse)
    return EZ_FAILURE;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // The mesh scratch is shared and not persistent — always re-bake and read back within one sync graph.
  vol.m_bDirty = true;

  ezGALReadbackBufferHelper countRB;
  ezGALReadbackBufferHelper vertRB;
  ezGALReadbackBufferHelper idxRB;

  auto pGraph = ezRenderGraphManager::CreateRenderGraph("VoxelExportReadback", ezRenderGraphPhase::PreRender);
  pGraph->Reset();

  UpdateVoxels(uiIndex, *pGraph);

  // Re-import the shared scratch (ImportBuffer dedupes by handle) to add the readback transfer pass.
  auto hGraphCounts = pGraph->ImportBuffer(m_hSharedMeshCounts);
  auto hGraphVerts = pGraph->ImportBuffer(m_hSharedMeshCompactVertices);
  auto hGraphIdx = pGraph->ImportBuffer(m_hSharedMeshIndices);

  const ezGALBufferHandle hCounts = m_hSharedMeshCounts;
  const ezGALBufferHandle hVerts = m_hSharedMeshCompactVertices;
  const ezGALBufferHandle hIdx = m_hSharedMeshIndices;

  auto pass = pGraph->AddTransferPass("VoxelReadback");
  pass.ReadBuffer(hGraphCounts, ezGALResourceState::CopySource);
  pass.ReadBuffer(hGraphVerts, ezGALResourceState::CopySource);
  pass.ReadBuffer(hGraphIdx, ezGALResourceState::CopySource);
  pass.HasSideEffects();
  pass.SetExecuteCallback([hCounts, hVerts, hIdx, &countRB, &vertRB, &idxRB](const ezRenderGraphContext& ctx)
    {
      countRB.ReadbackBuffer(*ctx.GetCommandEncoder(), hCounts);
      vertRB.ReadbackBuffer(*ctx.GetCommandEncoder(), hVerts);
      idxRB.ReadbackBuffer(*ctx.GetCommandEncoder(), hIdx); });

  ExecuteGraphSync(*pGraph, pDevice);

  const ezTime tDeadline = ezTime::Now() + timeout;

  auto PollReadback = [&](ezGALReadbackBufferHelper& readback, const char* szName) -> ezResult
  {
    while (true)
    {
      const auto result = readback.GetReadbackResult(ezTime::MakeFromMilliseconds(2));
      if (result == ezGALAsyncResult::Expired)
      {
        ezLog::Error("ReadbackVoxelData: {} readback expired for volume {}.", szName, uiIndex);
        return EZ_FAILURE;
      }
      if (result == ezGALAsyncResult::Ready)
        return EZ_SUCCESS;
      if (ezTime::Now() >= tDeadline)
      {
        ezLog::Error("ReadbackVoxelData: timed out waiting for {} of volume {}.", szName, uiIndex);
        return EZ_FAILURE;
      }
    }
  };

  if (PollReadback(countRB, "counts").Failed())
    return EZ_FAILURE;
  if (PollReadback(vertRB, "vertices").Failed())
    return EZ_FAILURE;
  if (PollReadback(idxRB, "indices").Failed())
    return EZ_FAILURE;

  // Pull the counts first; they bound the vertex/index copies below.
  {
    ezArrayPtr<const ezUInt8> rawMemory;
    auto lock = countRB.LockBuffer(rawMemory);
    if (!lock)
      return EZ_FAILURE;
    const VoxelMeshCounts* pCounts = reinterpret_cast<const VoxelMeshCounts*>(rawMemory.GetPtr());
    out_uiVertexCount = pCounts->VertexCount;
    out_uiPrimitiveCount = pCounts->PrimitiveCount;
  }

  {
    ezArrayPtr<const ezUInt8> rawMemory;
    auto lock = vertRB.LockBuffer(rawMemory);
    if (lock)
    {
      out_verts.SetCountUninitialized(out_uiVertexCount);
      ezMemoryUtils::Copy(out_verts.GetData(), reinterpret_cast<const VoxelGpuVertex*>(rawMemory.GetPtr()), out_uiVertexCount);
    }
  }

  {
    ezArrayPtr<const ezUInt8> rawMemory;
    auto lock = idxRB.LockBuffer(rawMemory);
    if (lock)
    {
      const ezUInt32 uiIndexCount = out_uiPrimitiveCount * 3;
      out_indices.SetCountUninitialized(uiIndexCount);
      ezMemoryUtils::Copy(out_indices.GetData(), reinterpret_cast<const ezUInt32*>(rawMemory.GetPtr()), uiIndexCount);
    }
  }

  return EZ_SUCCESS;
}
