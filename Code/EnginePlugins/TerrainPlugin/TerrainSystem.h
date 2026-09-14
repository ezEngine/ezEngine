#pragma once

#include <Core/World/WorldModule.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/TagSet.h>
#include <GameEngine/Utils/ImageDataResource.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Device/Device.h>
#include <TerrainPlugin/TerrainPluginDLL.h>

#include <Shaders/Terrain/Generation/TerrainBrushData.h>
#include <Shaders/Terrain/Generation/VoxelMeshConstants.h>

class ezRenderGraph;

struct EZ_TERRAINPLUGIN_DLL ezTerrainModifyMode
{
  using StorageType = ezUInt8;
  enum Enum : ezUInt8
  {
    Max = 0,         ///< Only raises terrain toward the brush height.
    Min = 1,         ///< Only lowers terrain toward the brush height.
    Set = 2,         ///< Forces terrain to the brush height regardless of current value.
    Carve = 3,
    Add = 4,
    OnlyPaint2D = 5, ///< No geometry change. Paints material using 2D SDF projection (2D brush style).
    OnlyPaint3D = 6, ///< No geometry change. Paints material using 3D SDF (3D brush style).
    Default = Max,
  };
};

struct EZ_TERRAINPLUGIN_DLL ezTerrainResolution
{
  using StorageType = ezUInt16;
  enum Enum : ezUInt16
  {
    Res32 = 32,
    Res64 = 64,
    Res128 = 128,
    Res256 = 256,
    Res512 = 512,
    Default = Res256,
  };
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_TERRAINPLUGIN_DLL, ezTerrainResolution);

/// Full-resolution quads spanned by one baked material cell along each axis.
/// Must match TERRAIN_MATERIAL_CELL_STEP in Shaders/Terrain/Generation/HeightfieldBakeConstants.h,
/// which documents it. Together they fix the size and layout of the material buffers.
inline constexpr ezUInt32 ezTerrainMaterialCellStep = 4;

/// Vertex sampling stride for the heightfield patch collision mesh.
/// The enumerator value equals the vertex stride (skip factor) used when sub-sampling full-resolution data.
struct EZ_TERRAINPLUGIN_DLL ezTerrainPatchColliderMode
{
  using StorageType = ezUInt8;

  enum Enum : ezUInt8
  {
    None = 0,              ///< No collision shape is generated.
    FullResolution = 1,    ///< Collision mesh matches the render resolution.
    HalfResolution = 2,    ///< Samples every second vertex — 1/4 the triangles of full resolution.
    QuarterResolution = 4, ///< Samples every fourth vertex — 1/16 the triangles of full resolution.
    EighthResolution = 8,  ///< Samples every eighth vertex — 1/64 the triangles of full resolution.

    Default = QuarterResolution,
  };
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_TERRAINPLUGIN_DLL, ezTerrainPatchColliderMode);

/// One node of the tessellated polyline that a spline brush follows.
struct ezTerrainData_SplineNode
{
  ezVec3 m_vPosition;      ///< World space.
  ezVec3 m_vUpDir;         ///< World-space brush Z axis at this node.
  float m_fArcLength = 0.0f; ///< Distance along the spline from its start.
};

/// CPU-side description of one terrain modification brush.
///
/// Components write to this each frame; the terrain system uploads the data to GPU before each bake.
///
/// A brush either is a box around m_vPosition, or, if m_SplineNodes has at least two entries, sweeps
/// its cross-section along the polyline through those nodes. In the latter case m_qRotation is ignored
/// and m_vHalfExtents.x extends the brush past both ends of an open spline.
struct ezTerrainData_Brush
{
  bool m_bInUse = false;
  bool m_bAffectHeightfields = true;
  bool m_bAffectVoxels = true;
  ezInt8 m_iPriority = 0;                      ///< Sort order. Higher = applied later = wins over lower-priority brushes. Equal priorities use mode-based ordering.
  ezUInt8 m_uiMaterialIndex = 0;               ///< Material (layer) index to paint: 0-3.
  ezEnum<ezTerrainModifyMode> m_ModifyMode;
  ezVec3 m_vPosition = ezVec3::MakeZero();
  ezQuat m_qRotation = ezQuat::MakeIdentity(); ///< World-space rotation of the brush.
  ezVec2 m_vHalfExtents = ezVec2::MakeZero();  ///< Half-size of the straight rectangular region along each brush axis.
  float m_fHalfExtentZ = 0.0f;                 ///< Half-size along the brush Z axis. Used only by Carve/Add mode for a 3D rounded-box volume.
  float m_fHalfExtentYTop = 0.0f;              ///< Upper Y half-size for asymmetric Carve/Add volumes (e.g. flat-floor tunnels). 0 = same as m_vHalfExtents.y (symmetric).
  float m_fInnerRadius = 0.0f;                 ///< Corner rounding radius of the inner (full-weight) region.
  float m_fOuterRadius = 5.0f;                 ///< Corner rounding radius; also the outer edge of the falloff zone.
  float m_fFalloff = 1.0f;                     ///< Exponent applied after smoothstep in the transition zone.
  float m_fMaterialStrength = 0.0f;            ///< Blend weight for material painting in [0, 1]. 0 = disabled (no material write).
  float m_fNoiseStrength = 0.0f;
  float m_fNoiseFrequency = 1.0f;
  ezTagSet m_Tags;                             ///< If non-empty, the brush only affects terrain objects that have at least one matching tag.
  ezDynamicArray<ezTerrainData_SplineNode> m_SplineNodes; ///< Polyline the brush follows. Fewer than two nodes = box brush.
  float m_fSplineLength = 0.0f;                ///< Arc length of the spline; equals the arc length of the last node.
  bool m_bSplineClosed = false;                ///< The last node connects back to the first, so the brush has no ends.
};

/// Hash over everything of one brush that influences the bake of one terrain object.
///
/// Terrain objects store one entry per brush that affects them, so that after brush modifications only
/// the modified brushes have to be evaluated again to decide whether a rebake is needed.
struct ezTerrainBrushContribution
{
  ezUInt32 m_uiBrushIndex = 0;
  ezUInt64 m_uiHash = 0; ///< Never 0, that value represents a brush that does not affect the object.
};

/// CPU-side state for one heightfield patch managed by ezTerrainSystem.
struct ezTerrainData_Heightfield
{
  bool m_bInUse = false;
  bool m_bDirty = true;
  ezUInt16 m_uiCellsPerSide = 128;            ///< Number of rendered quads per side (= resolution enum value).
  ezTransform m_GlobalTransform = ezTransform::MakeIdentity();
  ezGALBufferHandle m_hBakedHeights;          ///< Persistent, bound as UAV in heights CS and SRV in VS.
  ezGALBufferHandle m_hBakedNormals;          ///< Persistent, bound as UAV in normals CS and SRV in VS.
  ezGALBufferHandle m_hCellMaterials;         ///< Per-cell top-4 material indices (uint/cell), baked by Step3 CS over the stored grid including the border ring. SRV in VS.
  ezGALBufferHandle m_hVertexWeights;         ///< Per-cell-corner blend weights relative to that cell's top-4 (4 uint/cell), baked by Step3 CS over the same grid. SRV in VS.
  ezGALBufferHandle m_hCarveMask;             ///< One bit per stored-grid vertex, set when carved away. Written by Step2 CS, SRV in VS. Always full resolution.
  ezUInt8 m_uiDefaultMaterialIndex = 0;
  ezImageDataResourceHandle m_hHeightImage;   ///< Optional greyscale image used as baseline height source; sampled each bake.
  ezVec2 m_vImageOffset = ezVec2::MakeZero(); ///< UV offset into m_hHeightImage; selects the top-left corner of the sampled rect.
  ezVec2 m_vImageSize = ezVec2(1.0f);         ///< UV extent of the sampled rect within m_hHeightImage. Values < 1 select a sub-region.
  float m_fHeightScale = 0.0f;                ///< Multiplier applied to the [0, 1] greyscale sample to produce a world-space height.
  float m_fGridSpacing = 1.0f;
  ezTagSet m_Tags;                            ///< Identity tags for this heightfield; matched against brush include-tag filters.
  /// All brushes that affect this heightfield, sorted by brush index.
  ezDynamicArray<ezTerrainBrushContribution> m_BrushContributions;
  bool m_bBrushContributionsValid = false; ///< If false, m_BrushContributions is rebuilt from all brushes.
};

/// CPU-side state for one voxel terrain volume managed by ezTerrainSystem.
struct ezTerrainData_Voxel
{
  bool m_bInUse = false;
  bool m_bDirty = true;
  bool m_bInitialSolid = false;      ///< false = all cells start as air; true = all cells start as solid. Applied before brushes each bake.
  ezUInt8 m_uiCleanupIterations = 1; ///< Number of topology-cleanup iterations to run after baking (0–4). Higher values remove more spike voxels at the cost of slightly more GPU work.

  ezUInt16 m_uiResolution = 32;      ///< User-selected inner voxels per axis (no border, no alignment). Equals the triangulated cell count.
  ezUInt16 m_uiPackPitch = 5;        ///< Packed X row count = BufX / 8, where BufX = ((m_uiResolution + 2*border + 7) & ~7). Stride for BakedVoxels.
  float m_fVoxelSize = 1.0f;         ///< World-space size of one voxel.
  ezTransform m_GlobalTransform = ezTransform::MakeIdentity();

  /// Final render buffers — written from the shared compact scratch by VoxelCompactCopyCS each rebuild.
  /// Allocated at worst-case capacity in CreateVoxelTerrain; the copy is GPU-driven (DispatchIndirect)
  /// so only the used entries are written, and DrawArgs controls the actual draw count.
  ezGALBufferHandle m_hFinalVertices; ///< StructuredBuffer<VoxelGpuVertex> for rendering.
  ezGALBufferHandle m_hFinalIndices;  ///< StructuredBuffer<uint> for rendering.
  /// DrawArgs buffer: 4 uints {IndexCount, 1, 0, 0}. Written by VoxelCompactCopyCS thread 0.
  ezGALBufferHandle m_hFinalDrawArgs;

  float m_fFillHeight = 0.0f; ///< World-space Z height below which voxels start as solid (when m_bInitialSolid is true).
  ezTagSet m_Tags;            ///< Identity tags for this voxel volume; matched against brush include-tag filters.
  ezUInt64 m_uiBrushOverlapHash = 0;                                ///< Combined hash of m_BrushContributions and the volume properties that affect the bake. Updated when the volume is baked.
  ezDynamicArray<ezTerrainBrushContribution> m_BrushContributions; ///< All brushes that affect this volume, sorted by brush index.
  bool m_bBrushContributionsValid = false;                          ///< If false, m_BrushContributions is rebuilt from all brushes.
};

/// Manages GPU resources and compute shader dispatch for all terrain patches in a world.
///
/// Terrain components register here on activation and receive a slot index.
/// On each BeginRender event the system re-bakes all dirty patches before the main pipeline renders.
/// Baking is also triggered when the set of overlapping brushes changes for a given patch.
class EZ_TERRAINPLUGIN_DLL ezTerrainSystem : public ezWorldModule
{
  EZ_DECLARE_WORLD_MODULE();
  EZ_ADD_DYNAMIC_REFLECTION(ezTerrainSystem, ezWorldModule);

public:
  ezTerrainSystem(ezWorld* pWorld);
  ~ezTerrainSystem();

private:
  virtual void Initialize() override;
  virtual void Deinitialize() override;

  /// BeginRender handler; drives UpdateTerrain on every registered ezTerrainSystem instance.
  static void OnRenderEvent(const ezRenderWorldRenderEvent& e);

  /// Processes the deferred deletion queues.
  /// Called at the start of UpdateTerrain and from readback functions before they submit a sync graph.
  void FrameCleanup();

  ezMutex m_Mutex;
  ezSharedPtr<ezRenderGraph> m_pRenderGraph;

  static ezDeque<ezTerrainSystem*> s_TerrainSystems;

  //////////////////////////////////////////////////////////////////////////
  // Brushes
  //////////////////////////////////////////////////////////////////////////

public:
  /// Allocates a brush slot and returns its index. Reuses freed slots before growing the array.
  ezUInt32 CreateBrushData();

  /// Releases the brush slot and sets uiIdx to ezInvalidIndex. Terrain objects it affected rebake next frame.
  void RemoveBrushData(ezUInt32& ref_uiIdx);

  const ezTerrainData_Brush& ReadBrushData(ezUInt32 uiIdx) const;

  /// Returns a mutable reference and marks the brush as modified.
  ///
  /// Next frame only this brush is evaluated again, and only terrain objects whose result it changes rebake.
  /// Calling this without actually changing anything therefore doesn't cause rebakes.
  ezTerrainData_Brush& ModifyBrushData(ezUInt32 uiIdx);

private:
  /// Number of spline segments that share one bounding box in BrushCache::m_ChunkBounds.
  static constexpr ezUInt32 SplineChunkSegments = 16;

  /// Data derived from an ezTerrainData_Brush for culling. Rebuilt by UpdateBrushCaches after the brush was modified.
  struct BrushCache
  {
    bool m_bValid = false;
    bool m_bChanged = false;                     ///< The brush is in m_ChangedBrushes.
    ezBoundingBox m_NodeBounds;                  ///< World-space bounds of all spline nodes, not grown by the brush reach.
    ezDynamicArray<ezBoundingBox> m_ChunkBounds; ///< Same as m_NodeBounds, for every SplineChunkSegments segments.
  };

  /// The space and extent of one terrain object, against which brushes are culled.
  struct CullRegion
  {
    ezTransform m_InvTransform;  ///< World space to terrain object space.
    ezBoundingBox m_LocalBounds; ///< Extent including the border, used for spline brushes.
    float m_fSize = 0.0f;        ///< Extent without the border, used for box brushes.
    bool m_bVoxel = false;       ///< Box brushes include their Z extent in the culling radius only for voxels.
  };

  static CullRegion MakeCullRegion(const ezTerrainData_Heightfield& heightfield);
  static CullRegion MakeCullRegion(const ezTerrainData_Voxel& vol);

  /// Marks the brush cache as outdated and queues the brush for re-evaluation in UpdateTerrain.
  void MarkBrushChanged(ezUInt32 uiIdx);

  /// Rebuilds the caches of all changed brushes that are outdated. Must be called before any brush culling.
  void UpdateBrushCaches();

  /// Determines whether a brush can affect a terrain object, and for spline brushes which part of it.
  ///
  /// With bIgnoreZ the brush is only tested against the XY extent of the region.
  /// For spline brushes out_uiFirstNode and out_uiLastNode enclose all segments that come close enough, including
  /// those in between. For box brushes they are not written.
  bool FindBrushOverlap(ezUInt32 uiBrush, const CullRegion& region, bool bIgnoreZ, ezUInt32& out_uiFirstNode, ezUInt32& out_uiLastNode) const;

  /// Spline part of FindBrushOverlap. Rejects whole chunks of segments through the brush cache first.
  bool FindSplineNodeRange(ezUInt32 uiBrush, const CullRegion& region, bool bIgnoreZ, ezUInt32& out_uiFirstNode, ezUInt32& out_uiLastNode) const;

  /// Creates a transient GPU structured buffer from the provided brush array.
  /// Always allocates at least one element so shader bindings stay valid when the brush list is empty.
  ezGALBufferHandle CreateBrushBuffer(ezDynamicArray<TerrainBrushData>& brushes, ezGALDevice* pDevice) const;

  /// Same as CreateBrushBuffer, for the polyline nodes of spline brushes.
  ezGALBufferHandle CreateSplineNodeBuffer(ezDynamicArray<TerrainSplineNode>& nodes, ezGALDevice* pDevice) const;

  /// Uploads the node range found by FindSplineNodeRange into nodes and sets the spline fields of bd.
  /// Also overrides bd.Position with the center of the range, which makes the height-based bake order
  /// meaningful for spline brushes, and resets the brush rotation, which does not apply to spline brushes.
  static void SetupSplineBrush(const ezTerrainData_Brush& brush, const ezTransform& invTrans, ezUInt32 uiFirstNode, ezUInt32 uiLastNode, TerrainBrushData& bd, ezDynamicArray<TerrainSplineNode>& nodes);

  ezDeque<ezTerrainData_Brush> m_Brushes;
  ezDeque<BrushCache> m_BrushCaches;         ///< Parallel to m_Brushes.
  ezDynamicArray<ezUInt32> m_ChangedBrushes; ///< Brushes modified or removed since the last UpdateTerrain.

  //////////////////////////////////////////////////////////////////////////
  // Heightfields
  //////////////////////////////////////////////////////////////////////////

public:
  /// Allocates GPU buffers for a new heightfield patch of the given resolution and returns a slot index.
  ezUInt32 CreateHeightfieldTerrain(ezUInt32 uiCellsPerSide);

  /// Thread-safe: queues the patch for GPU buffer destruction at the start of the next frame.
  void RemoveHeightfieldTerrain(ezUInt32& ref_uiPatchIndex);

  /// Returns a mutable reference and marks the patch dirty so it rebakes next frame.
  ezTerrainData_Heightfield& ModifyHeightfieldTerrain(ezUInt32 uiIdx);

  /// Returns the baked GPU height buffer handle (SRV in VS, UAV in heights CS).
  ezGALBufferHandle GetHeightfieldHeightBuffer(ezUInt32 uiPatchIndex) const;

  /// Returns the baked GPU normal buffer handle (SRV in VS, UAV in normals CS).
  ezGALBufferHandle GetHeightfieldNormalBuffer(ezUInt32 uiPatchIndex) const;

  /// Returns the per-cell material index buffer (uint/cell) baked by Step3. Bound as SRV in VS.
  ezGALBufferHandle GetHeightfieldCellMaterialBuffer(ezUInt32 uiPatchIndex) const;

  /// Returns the per-vertex f16 weight buffer (uint/vertex) baked by Step3. Bound as SRV in VS.
  ezGALBufferHandle GetHeightfieldMaterialVertexWeightBuffer(ezUInt32 uiPatchIndex) const;

  /// Returns the per-vertex carve bitmask buffer (32 vertices per uint) written by Step2. SRV in VS.
  ezGALBufferHandle GetHeightfieldCarveMaskBuffer(ezUInt32 uiPatchIndex) const;


  /// Returns the number of quads per side (= resolution enum value) for the given patch.
  ezUInt32 GetHeightfieldCellsPerSide(ezUInt32 uiPatchIndex) const;

  /// Returns a hash over all brushes whose footprint overlaps this patch's XY extent.
  /// Changes when the set of relevant brushes changes. Returns 0 for invalid indices.
  /// Always computed from scratch, so it is valid even if the terrain was not updated since brushes changed.
  ezUInt64 GetHeightfieldBrushOverlapHash(ezUInt32 uiPatchIndex);

  /// Bakes the given heightfield patch and blocks while reading the result back to the CPU.
  ///
  /// out_heights receives the full stored height grid (CellsPerSide+9)² floats — 4 border rings on each
  /// side beyond the rendered vertices. out_dominantMat receives the dominant material index per stored cell,
  /// which is the material with the largest weight including the patch's implicit base material, or 0xFF for
  /// carved cells.
  /// Must be called from the main/render thread with GPU device access.
  ezResult ReadbackHeightfieldData(ezUInt32 uiPatchIndex, ezDynamicArray<float>& out_heights, ezDynamicArray<ezUInt8>& out_dominantMat, ezTime timeout = ezTime::MakeFromSeconds(5.0));

private:
  /// Immediately destroys the GPU buffers for the given slot and marks it free. Not thread-safe; call via RemoveHeightfieldTerrain.
  void DestroyHeightfieldTerrain(ezUInt32& uiIndex);

  /// Called each BeginRender. Computes brush overlap hashes, marks patches dirty when the brush set changes,
  /// then enqueues render graph passes for all dirty patches.
  void UpdateTerrain();

  /// Adds a 3-pass render graph sequence for one heightfield bake:
  /// Step1 bakes heights and the material mask, Step2 derives normals, Step3 resolves per-cell materials and vertex weights.
  void UpdateHeightfield(ezUInt32 uiIndex, ezRenderGraph& graph);

  /// (Re)creates the shared heightfield bake scratch when uiStoredSize exceeds the current capacity, else reuses it.
  /// Content is valid only within a single bake; one buffer set is shared across all heightfield patches.
  void EnsureSharedHeightfieldScratch(ezUInt32 uiStoredSize);

  void DestroyHeightfields();

  /// Destroys all heightfield scratch buffers.
  void DestroySharedHeightfieldScratch();

  /// Populates brushes with all active brushes whose footprint overlaps the heightfield's XY extent,
  /// sorted in bake order: priority ascending, Carve last within the same priority.
  void FindHeightfieldOverlappingBrushes(const ezTerrainData_Heightfield& heightfield, ezDynamicArray<TerrainBrushData>& brushes, ezDynamicArray<TerrainSplineNode>& splineNodes) const;

  /// Hash of the brush with index uiBrush for this heightfield, or 0 if it doesn't affect it.
  ezUInt64 ComputeHeightfieldBrushContribution(const ezTerrainData_Heightfield& heightfield, const CullRegion& region, ezUInt32 uiBrush) const;

  /// Evaluates all brushes (bAllBrushes) or only m_ChangedBrushes and updates heightfield.m_BrushContributions.
  /// Returns true if any contribution changed.
  bool UpdateHeightfieldBrushContributions(ezTerrainData_Heightfield& heightfield, bool bAllBrushes);

  /// Combines the given contributions with the heightfield properties that affect the bake.
  static ezUInt64 ComputeHeightfieldBrushOverlapHash(const ezTerrainData_Heightfield& heightfield, const ezDynamicArray<ezTerrainBrushContribution>& contributions);

  /// Shared heightfield bake scratch: intermediate material mask (uint2/vertex), written by Step1/2 and read
  /// by Step3 within one bake. Sized to the largest patch's stored grid; m_uiSharedMaskStoredSize tracks it.
  ezGALBufferHandle m_hHeightfieldSharedMask;
  ezUInt32 m_uiHeightfieldSharedMaskStoredSize = 0;

  ezHybridArray<ezUInt32, 16> m_QueuedHeightfieldsToDelete;
  ezDynamicArray<ezTerrainData_Heightfield> m_Heightfields;
  ezConstantBufferStorageHandle m_hHeightfieldBakeConstants;
  ezShaderResourceHandle m_hTerrainBakeStep1Shader;
  ezShaderResourceHandle m_hTerrainBakeStep2Shader;
  ezShaderResourceHandle m_hTerrainBakeStep3Shader;

  //////////////////////////////////////////////////////////////////////////
  // Voxel Volumes
  //////////////////////////////////////////////////////////////////////////

public:
  /// Allocates GPU buffers and readback helpers for a new voxel terrain piece. Returns a slot index.
  ezUInt32 CreateVoxelTerrain(ezUInt32 uiResolution, float fVoxelSize);
  /// Thread-safe: queues the volume for GPU buffer destruction at the start of the next frame.
  void RemoveVoxelTerrain(ezUInt32 uiIndex);

  /// Returns a mutable reference and marks the volume dirty so it rebakes next frame.
  ezTerrainData_Voxel& ModifyVoxelTerrain(ezUInt32 uiIndex);

  /// Returns the GPU vertex buffer (StructuredBuffer SRV) for the given voxel volume.
  ezGALBufferHandle GetVoxelVolumeGpuMeshVertexBuffer(ezUInt32 uiIndex) const;

  /// Returns the GPU index buffer (StructuredBuffer SRV) for the given voxel volume.
  ezGALBufferHandle GetVoxelVolumeGpuMeshIndexBuffer(ezUInt32 uiIndex) const;

  /// Returns the indirect draw arguments buffer for the given voxel volume.
  ezGALBufferHandle GetVoxelVolumeGpuMeshDrawArgsBuffer(ezUInt32 uiIndex) const;

  /// Returns the brush overlap hash stored on the voxel volume (updated each bake).
  /// Returns 0 for invalid indices. Used by export modifiers to detect stale baked files.
  ezUInt64 GetVoxelBrushOverlapHash(ezUInt32 uiIndex) const;

  /// Bakes the given voxel terrain piece and blocks while reading the mesh back to the CPU.
  ///
  /// out_verts / out_indices receive the compacted surface-nets mesh; out_vertexCount and
  /// out_primitiveCount give the valid element counts (out_indices holds out_primitiveCount*3 indices).
  /// Must be called from the main/render thread with GPU device access.
  ezResult ReadbackVoxelData(ezUInt32 uiIndex, ezTempArray<VoxelGpuVertex>& out_verts, ezDynamicArray<ezUInt32>& out_indices, ezUInt32& out_uiVertexCount, ezUInt32& out_uiPrimitiveCount, ezTime timeout = ezTime::MakeFromSeconds(5.0));

private:
  /// Immediately destroys the GPU buffers for the given slot and marks it free. Not thread-safe; call via RemoveVoxelTerrain.
  void DestroyVoxelTerrain(ezUInt32& uiIndex);

  /// Adds the full voxel bake pass sequence to graph: solid/SDF bake → SDF blur → topology cleanup iterations
  /// → surface nets pass 1 and 2 → fill indirect dispatch args → GPU-driven compact copy to final render buffers.
  void UpdateVoxels(ezUInt32 uiIndex, ezRenderGraph& graph);

  /// (Re)creates the shared voxel bake scratch when the requested dimensions exceed the current capacity, else reuses it.
  /// Content is valid only within a single bake sequence; one buffer set is shared across all voxel volumes.
  void EnsureSharedVoxelScratch(ezUInt32 uiPackPitch, ezUInt32 uiBufYZ, ezUInt32 uiResolution);

  void DestroyVoxelVolumes();
  void DestroySharedVoxelScratch();

  /// Populates brushes with all active brushes whose footprint overlaps the voxel volume's 3D extent,
  /// sorted in bake order: priority ascending, Carve last within the same priority.
  void FindVoxelOverlappingBrushes(const ezTerrainData_Voxel& vol, ezDynamicArray<TerrainBrushData>& brushes, ezDynamicArray<TerrainSplineNode>& splineNodes) const;

  /// Same as the heightfield counterparts.
  ezUInt64 ComputeVoxelBrushContribution(const ezTerrainData_Voxel& vol, const CullRegion& region, ezUInt32 uiBrush) const;
  bool UpdateVoxelBrushContributions(ezTerrainData_Voxel& vol, bool bAllBrushes);
  static ezUInt64 ComputeVoxelBrushOverlapHash(const ezTerrainData_Voxel& vol, const ezDynamicArray<ezTerrainBrushContribution>& contributions);

  ezConstantBufferStorageHandle m_hVoxelBakeConstants;
  ezDynamicArray<ezTerrainData_Voxel> m_VoxelVolumes;
  ezHybridArray<ezUInt32, 8> m_QueuedVoxelVolumesToDelete;
  ezShaderResourceHandle m_hVoxelBakeShader;
  ezShaderResourceHandle m_hVoxelMeshClearShader;
  ezShaderResourceHandle m_hVoxelSurfaceNetsPass1Shader;
  ezShaderResourceHandle m_hVoxelSurfaceNetsPass2Shader;
  ezShaderResourceHandle m_hVoxelBlurDistShader;
  ezShaderResourceHandle m_hVoxelCleanupShader;
  ezShaderResourceHandle m_hVoxelFillCompactCopyArgsShader;
  ezShaderResourceHandle m_hVoxelCompactCopyShader;

  /// Shared voxel bake scratch — one set reused across all voxel volumes; content is valid only within a single bake sequence.
  /// Sized to the largest volume seen so far; m_uiSharedVoxel* track the current capacity dimensions.
  ezGALBufferHandle m_hSharedVoxels;                  ///< Packed solidity bits (uint, 8 voxels per uint along X).
  ezGALBufferHandle m_hSharedVoxelDist;               ///< Per-voxel SDF (float), unpacked.
  ezGALBufferHandle m_hSharedVoxelDistScratch;        ///< Ping-pong partner of m_hSharedVoxelDist for blur/cleanup.
  ezGALBufferHandle m_hSharedMeshRemap;               ///< linearCellIndex → compact vertex slot.
  ezGALBufferHandle m_hSharedMeshCompactVertices;     ///< Densely packed surface-nets vertices.
  ezGALBufferHandle m_hSharedMeshIndices;             ///< Surface-nets indices (worst-case cells*18).
  ezGALBufferHandle m_hSharedMeshCounts;              ///< VoxelMeshCounts, 1 entry.
  ezGALBufferHandle m_hSharedCompactCopyDispatchArgs; ///< DispatchIndirect args for VoxelCompactCopyCS.
  ezUInt32 m_uiSharedVoxelPackPitch = 0;
  ezUInt32 m_uiSharedVoxelBufYZ = 0;
  ezUInt32 m_uiSharedVoxelResolution = 0;
};
