#pragma once

#include <Core/World/WorldModule.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererFoundation/Shader/Types.h>

// temporary until we moved all the render data caching into this module
#include <RendererCore/RenderWorld/RenderWorld.h>

struct ezPerInstanceData;
struct ezRenderWorldExtractionEvent;

/// \brief Manager for render data and instance data buffers.
///
/// Render data is used to extract rendering information from components during the extraction phase that is then used for rendering.
/// If many objects should be rendered with one instanced draw call, instance data buffers are used to hold the per-instance information.
/// For that the render data should derive from ezInstanceableRenderData. See ezPerInstanceData what data is supported by default for each instance.
/// When more per instance data is needed it is possible to register a custom instance data buffer and attach that to the render data as well.
class EZ_RENDERERCORE_DLL ezRenderDataManager : public ezWorldModule
{
  EZ_DECLARE_WORLD_MODULE();
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderDataManager, ezWorldModule);

public:
  ezRenderDataManager(ezWorld* pWorld);
  virtual ~ezRenderDataManager();

  virtual void Initialize() override;

  /// \brief Creates render data that is only valid for this frame. The data is automatically deleted after the frame has been rendered.
  template <typename T>
  T* CreateRenderDataForThisFrame(const ezGameObject* pOwner) const;

  // TODO: move render data caching into this world module as well

  /// \brief Gets or creates per-instance data for the given instance data offset.
  ///
  /// This function is thread-safe and is typically called in an ezMsgExtractRenderData message handler.
  /// The render data manager holds two instance data buffers, one for static objects and one for dynamic objects.
  /// Typically one would pass GetOwner()->IsDynamic() as bDynamic. If the corresponding render data is not cached
  /// it is better to always pass true so that the static buffer does not need to be uploaded every frame.
  ezArrayPtr<ezPerInstanceData> GetOrCreateInstanceData(const ezComponent* pOwnerComponent, bool bDynamic, ezGALDynamicBufferHandle& out_hBuffer, ezInstanceDataOffset& inout_instanceDataOffset, ezUInt32 uiCount = 1) const;

  /// \brief Deletes the instance data associated with the given instance data offset.
  ///
  /// This function is thread-safe but is typically called in the OnDeactivated function of a component.
  void DeleteInstanceData(ezInstanceDataOffset& inout_instanceDataOffset) const;

  /// \brief Helper function to fill ezPerInstanceData.
  static void FillPerInstanceData(ezPerInstanceData& out_perInstanceData, const ezGameObject* pObject, const ezTransform& globalTransform, ezUInt32 uiUniqueID = 0, const ezColor& color = ezColor::White, const ezVec4& vCustomData = ezVec4(0, 1, 0, 1), float fBoundingSphereRadius = 1.0f, ezUInt32 uiRandomSeed = 0);

  /// \brief Helper function that combines GetOrCreateInstanceData and FillPerInstanceData.
  ezGALDynamicBufferHandle GetOrCreateInstanceDataAndFill(const ezComponent& ownerComponent, bool bDynamic, const ezTransform& globalTransform, ezInstanceDataOffset& inout_instanceDataOffset, ezUInt32 uiUniqueID = 0, const ezColor& color = ezColor::White, const ezVec4& vCustomData = ezVec4(0, 1, 0, 1)) const;


  /// \brief Registers a custom instance data buffer that can be used to store additional per-instance data.
  ///
  /// The beforeUploadCallback is called just before the buffer is uploaded each frame, so it can be used to e.g. wait for a task that generated the data.
  ezUInt32 RegisterCustomInstanceData(const ezGALBufferCreationDescription& desc, ezStringView sDebugName, ezDelegate<void()> beforeUploadCallback = {});

  /// \brief Gets or creates custom per-instance data for the given instance data offset.
  ///
  /// This function is thread-safe and is typically called in an ezMsgExtractRenderData message handler.
  template <typename T>
  ezArrayPtr<T> GetOrCreateCustomInstanceData(ezUInt32 uiCustomDataIndex, const ezComponent* pOwnerComponent, ezGALDynamicBufferHandle& out_hBuffer, ezCustomInstanceDataOffset& inout_instanceDataOffset, ezUInt32 uiCount = 1) const;

  /// \brief Deletes the custom instance data associated with the given instance data offset.
  ///
  /// This function is thread-safe but is typically called in the OnDeactivated function of a component.
  void DeleteCustomInstanceData(ezUInt32 uiCustomDataIndex, ezCustomInstanceDataOffset& inout_instanceDataOffset) const;

  /// \brief Helper function that combines GetOrCreateCustomInstanceData and fills it with the given data.
  template <typename T>
  ezGALDynamicBufferHandle GetOrCreateCustomInstanceDataAndFill(ezUInt32 uiCustomDataIndex, const ezComponent& ownerComponent, ezCustomInstanceDataOffset& inout_instanceDataOffset, const T& data) const;

  /// \brief Returns the underlying dynamic buffer for the given custom instance data buffer index.
  ezGALDynamicBufferHandle GetCustomInstanceDataBuffer(ezUInt32 uiCustomDataIndex) const;

  /// \brief Compacts the given custom instance data buffer to reduce fragmentation.
  ///
  /// This is only necessary if allocations with different counts were created and deleted over time.
  void CompactCustomInstanceDataBuffer(ezUInt32 uiCustomDataIndex, ezUInt32 uiMaxSteps = 16);


  /// \brief Gets or creates skinning data for the given instance data offset.
  ///
  /// ezSkinningState wraps around these functions to manage skinning data for skinned meshes
  /// and should be preferred instead of calling these functions directly.
  ezArrayPtr<ezShaderTransform> GetOrCreateSkinningData(const ezComponent* pOwnerComponent, ezCustomInstanceDataOffset& inout_instanceDataOffset, ezUInt32 uiNumTransforms) const;

  /// \brief Gets the skinning data for reading for the given instance data offset.
  ezArrayPtr<const ezShaderTransform> GetSkinningData(const ezCustomInstanceDataOffset& instanceDataOffset) const;

  /// \brief Deletes the skinning data associated with the given instance data offset.
  void DeleteSkinningData(ezCustomInstanceDataOffset& inout_instanceDataOffset) const;

  /// \brief Returns the underlying dynamic buffer that holds the skinning data.
  ezGALDynamicBufferHandle GetSkinningDataBuffer() const;

private:
  ezByteArrayPtr GetOrCreateCustomInstanceData(ezUInt32 uiCustomDataIndex, ezUInt32 uiStructByteSize, const ezComponent* pOwnerComponent, ezGALDynamicBufferHandle& out_hBuffer, ezCustomInstanceDataOffset& inout_instanceDataOffset, ezUInt32 uiCount) const;

  void CompactSkinningDataBuffer(const UpdateContext& context);
  void OnExtractionEvent(const ezRenderWorldExtractionEvent& e);

  mutable ezMutex m_Mutex;

  ezHybridArray<ezGALDynamicBufferHandle, 16> m_Buffers;
  ezDynamicArray<ezDelegate<void()>> m_BeforeUploadCallbacks;

  struct ExtractionData
  {
    ezHybridArray<ezGALDynamicBuffer*, 16> m_pBuffers;
  };

  ExtractionData m_ExtractionData;
};

#include <RendererCore/Pipeline/Implementation/RenderDataManager_inl.h>
