#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Types/Delegate.h>

#include <RendererFoundation/Descriptors/Enumerations.h>

class ezGALDevice;

template <>
struct ezHashHelper<ezGALTextureHandle>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(ezGALTextureHandle value)
  {
    return ezHashHelper<ezGALTextureHandle::IdType::StorageType>::Hash(value.GetInternalID().m_Data);
  }

  EZ_ALWAYS_INLINE static bool Equal(ezGALTextureHandle a, ezGALTextureHandle b)
  {
    return a == b;
  }
};

template <>
struct ezHashHelper<ezGALBufferHandle>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(ezGALBufferHandle value)
  {
    return ezHashHelper<ezGALBufferHandle::IdType::StorageType>::Hash(value.GetInternalID().m_Data);
  }

  EZ_ALWAYS_INLINE static bool Equal(ezGALBufferHandle a, ezGALBufferHandle b)
  {
    return a == b;
  }
};

/// Tracks texture and buffer states and emits barriers through callbacks when synchronization is required.
///
/// For resources that are first seen without an explicit initial state, the tracker starts from the resource default state (ezGAL*CreationDescription::GetDefaultState) with stage Auto.
///
/// Barrier emission is callback-based: callbacks are only invoked when a barrier is needed. A barrier is required when the state changes, or when the previous
/// state contains ezGALResourceState::UnorderedAccess (UAV write-after-write).
class EZ_RENDERERFOUNDATION_DLL ezGALResourceStateTracker
{
public:
  struct SubResourceState
  {
    ezBitflags<ezGALResourceState> m_State;
    ezBitflags<ezGALShaderStageFlags> m_Stages;
  };

  struct TextureState
  {
    ezGALTextureRange m_FullRange; ///< Range covering the entire texture, filled on first occurrence.
    /// If size == 1, the single entry represents the entire resource (compressed).
    /// If size > 1, each entry corresponds to a sub-resource at index:
    /// uiMipLevel + uiLayer * m_FullRange.m_uiMipLevels
    ezHybridArray<SubResourceState, 1> m_SubResourceStates;
  };

public:
  explicit ezGALResourceStateTracker(ezGALDevice* pDevice);
  ~ezGALResourceStateTracker() = default;

  ezGALResourceStateTracker(const ezGALResourceStateTracker&) = delete;
  ezGALResourceStateTracker& operator=(const ezGALResourceStateTracker&) = delete;

  void Clear();

  /// \name Initial State
  ///@{

  /// Overrides the tracked state for a texture.
  ///
  /// Resets tracking to a single compressed entry that represents the full resource and clears tracked stage flags.
  void SetInitialTextureState(ezGALTextureHandle hTexture, ezBitflags<ezGALResourceState> state, ezBitflags<ezGALShaderStageFlags> stages = ezGALShaderStageFlags::Auto);

  /// Overrides the tracked state for a buffer and clears tracked stage flags.
  void SetInitialBufferState(ezGALBufferHandle hBuffer, ezBitflags<ezGALResourceState> state, ezBitflags<ezGALShaderStageFlags> stages = ezGALShaderStageFlags::Auto);

  ///@}
  /// \name State Transitions
  ///@{

  /// Transitions a texture range to the given state.
  ///
  /// If range uses EZ_GAL_ALL_* sentinels they are resolved against the full texture range. The callback is invoked once per emitted barrier.
  void ChangeState(ezGALTextureHandle hTexture,
    ezGALTextureRange range,
    ezBitflags<ezGALResourceState> newState,
    ezBitflags<ezGALShaderStageFlags> stage,
    const ezDelegate<void(const ezGALTextureBarrier&)>& barrierCallback);

  /// Transitions a buffer to the given state.
  ///
  /// The callback is invoked only when a barrier is required.
  void ChangeState(ezGALBufferHandle hBuffer,
    ezBitflags<ezGALResourceState> newState,
    ezBitflags<ezGALShaderStageFlags> stage,
    const ezDelegate<void(const ezGALBufferBarrier&)>& barrierCallback);

  /// Transitions all tracked textures back to their default state (ezGALTextureCreationDescription::GetDefaultState).
  ///
  /// Emits full-resource barriers for compressed texture entries and per-subresource barriers for expanded entries.
  void RevertTextureState(const ezDelegate<void(const ezGALTextureBarrier&)>& barrierCallback);

  /// Transitions all tracked buffers back to their default state (ezGALBufferCreationDescription::GetDefaultState).
  void RevertBufferState(const ezDelegate<void(const ezGALBufferBarrier&)>& barrierCallback);

  ///@}
  /// \name State Verification
  ///@{

  const SubResourceState* GetBufferState(ezGALBufferHandle hBuffer) const;
  const TextureState* GetTextureState(ezGALTextureHandle hTexture) const;
  static bool IsTextureBarrierNeeded(const SubResourceState& oldState, const SubResourceState& newState, bool bForceUAVBarrier = true);
  static bool IsBufferBarrierNeeded(const SubResourceState& oldState, const SubResourceState& newState, bool bForceUAVBarrier = true);
  static bool AreStagesCovered(ezBitflags<ezGALShaderStageFlags> coveredStages, ezBitflags<ezGALShaderStageFlags> requiredStages);

  ///@}
private:
  void ResolveProxyTexture(ezGALTextureHandle& ref_hTexture,
    ezGALTextureRange& ref_range) const;
  TextureState& GetOrCreateTextureState(ezGALTextureHandle hTexture);
  void ExpandTextureState(TextureState& state);
  SubResourceState& GetOrCreateBufferState(ezGALBufferHandle hBuffer);

private:
  ezGALDevice* m_pDevice = nullptr;
  ezHashTable<ezGALTextureHandle, TextureState> m_TextureStates;
  ezHashTable<ezGALBufferHandle, SubResourceState> m_BufferStates;
};
