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

/// CPU-side description of one terrain modification brush.
///
/// Components write to this each frame; the terrain system uploads the data to GPU before each bake.
struct ezTerrainData_Brush
{
  bool m_bInUse = false;
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
  ezGALBufferHandle m_hCellMaterials;         ///< Per-cell top-3 material indices (uint/cell), baked by Step3 CS. SRV in VS.
  ezGALBufferHandle m_hVertexWeights;         ///< Per-vertex f16 blend weights relative to cell top-3 (uint/vertex), baked by Step3 CS. SRV in VS.
  ezUInt8 m_uiDefaultMaterialIndex = 0;
  ezImageDataResourceHandle m_hHeightImage;   ///< Optional greyscale image used as baseline height source; sampled each bake.
  ezVec2 m_vImageOffset = ezVec2::MakeZero(); ///< UV offset into m_hHeightImage; selects the top-left corner of the sampled rect.
  ezVec2 m_vImageSize = ezVec2(1.0f);         ///< UV extent of the sampled rect within m_hHeightImage. Values < 1 select a sub-region.
  float m_fHeightScale = 0.0f;                ///< Multiplier applied to the [0, 1] greyscale sample to produce a world-space height.
  float m_fGridSpacing = 1.0f;
  ezTagSet m_Tags;                            ///< Identity tags for this heightfield; matched against brush include-tag filters.
  /// Hash of the indices of brushes that spatially overlap this heightfield.
  /// Updated each time brush state is re-evaluated; compared against the previous value to skip
  /// re-bakes when the set of relevant brushes (and whether any are present) has not changed.
  ezUInt64 m_uiBrushOverlapHash = 0;
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

  /// Releases the brush slot and sets uiIdx to ezInvalidIndex. Marks all brushes dirty for rebake.
  void RemoveBrushData(ezUInt32& ref_uiIdx);

  const ezTerrainData_Brush& ReadBrushData(ezUInt32 uiIdx) const;

  /// Returns a mutable reference and marks the brush set dirty so all patches rebake next frame.
  ezTerrainData_Brush& ModifyBrushData(ezUInt32 uiIdx);

private:
  /// Creates a transient GPU structured buffer from the provided brush array.
  /// Always allocates at least one element so shader bindings stay valid when the brush list is empty.
  ezGALBufferHandle CreateBrushBuffer(ezDynamicArray<TerrainBrushData>& brushes, ezGALDevice* pDevice) const;

  bool m_bBrushesDirty = true;
  ezDeque<ezTerrainData_Brush> m_Brushes;

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

  /// Returns the number of quads per side (= resolution enum value) for the given patch.
  ezUInt32 GetHeightfieldCellsPerSide(ezUInt32 uiPatchIndex) const;

  /// Returns a hash over all brushes whose footprint overlaps this patch's XY extent.
  /// Changes when the set of relevant brushes changes. Returns 0 for invalid indices.
  ezUInt64 GetHeightfieldBrushOverlapHash(ezUInt32 uiPatchIndex) const;

  /// Bakes the given heightfield patch and blocks while reading the result back to the CPU.
  ///
  /// out_heights receives the full stored height grid (CellsPerSide+9)² floats — 4 border rings on each
  /// side beyond the rendered vertices. out_dominantMat receives the dominant material index per stored cell.
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
  void FindHeightfieldOverlappingBrushes(const ezTerrainData_Heightfield& heightfield, ezDynamicArray<TerrainBrushData>& brushes) const;

  /// Returns a hash over the indices of all brushes whose footprint overlaps the heightfield's XY extent.
  /// Returns 0 when no brush overlaps. Used to detect changes in the set of relevant brushes per patch.
  ezUInt64 ComputeHeightfieldBrushOverlapHash(const ezTerrainData_Heightfield& heightfield) const;

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
};
