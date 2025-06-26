#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererFoundation/Shader/BindGroup.h>

/// \brief Creates ezGALBindGroupCreationDescription that can be passed into ezGALCommandEncoder.
///
/// This class is usually not created manually but instead retrieved via ezRenderContext::GetBindGroup.
/// By calling the various Bind* methods resources can be bound to a shader. The evaluation is deferred until the CreateBindGroup call which matches the bound items to the bind group layout's ezShaderResourceBinding. Any binding that can't find a matching item will be filled with a fallback resource via ezGALRendererFallbackResources. These bind group items will be marked with the flag ezGALBindGroupItemFlags::Fallback.
class EZ_RENDERERCORE_DLL ezBindGroupBuilder
{
public:
  ezBindGroupBuilder();

  /// \brief Must be called before the builder can be used.
  /// This should be called at the start of each frame to make sure no stale resources are referenced inside the builder.
  /// \param pDevice The device used to validate resources in bind calls.
  void ResetBoundResources(const ezGALDevice* pDevice);

  /// \brief Returns whether a Bind* call modified in internal state since the last CreateBindGroup call.
  bool IsModified() const { return m_bModified; }

  /// Binds a sampler to this bind group
  /// @param sSlotName The slot under which the sampler is to be bound.
  /// @param hSampler If valid, it will be bound. If not, bind group item under this slot will be removed and replaced with a fallback resource if required.
  void BindSampler(ezTempHashedString sSlotName, ezGALSamplerStateHandle hSampler);

  /// Binds a buffer to this bind group
  /// @param sSlotName The slot under which the buffer is to be bound.
  /// @param hBuffer If valid, it will be bound. If not, bind group item under this slot will be removed and replaced with a fallback resource if required.
  /// @param bufferRange What part of the buffer should be bound. Default is entire buffer.
  /// @param overrideTexelBufferFormat Sets the format of the texel buffer. If invalid, default format of the buffer will be used.
  void BindBuffer(ezTempHashedString sSlotName, ezGALBufferHandle hBuffer, ezGALBufferRange bufferRange = {}, ezEnum<ezGALResourceFormat> overrideTexelBufferFormat = ezGALResourceFormat::Invalid);

  /// Binds a texture to this bind group
  /// @param sSlotName The slot under which the texture is to be bound.
  /// @param hTexture If valid, it will be bound. If not, bind group item under this slot will be removed and replaced with a fallback resource if required.
  /// @param textureRange What part of the texture should be bound. Default is entire texture. Or as much as the target ezShaderResourceBinding allows for.
  /// @param overrideViewFormat If set, re-interprets the format of the texture. This can cause performance penalties. Only use it to e.g. read linear vs gamma space or other formats of same type and width.
  void BindTexture(ezTempHashedString sSlotName, ezGALTextureHandle hTexture, ezGALTextureRange textureRange = {}, ezEnum<ezGALResourceFormat> overrideViewFormat = ezGALResourceFormat::Invalid);

  // Convenience functions:
  void BindTexture(ezTempHashedString sSlotName, const ezTexture2DResourceHandle& hTexture, ezResourceAcquireMode acquireMode = ezResourceAcquireMode::AllowLoadingFallback, ezGALTextureRange textureRange = {}, ezEnum<ezGALResourceFormat> overrideViewFormat = ezGALResourceFormat::Invalid);
  void BindTexture(ezTempHashedString sSlotName, const ezTexture3DResourceHandle& hTexture, ezResourceAcquireMode acquireMode = ezResourceAcquireMode::AllowLoadingFallback, ezGALTextureRange textureRange = {}, ezEnum<ezGALResourceFormat> overrideViewFormat = ezGALResourceFormat::Invalid);
  void BindTexture(ezTempHashedString sSlotName, const ezTextureCubeResourceHandle& hTexture, ezResourceAcquireMode acquireMode = ezResourceAcquireMode::AllowLoadingFallback, ezGALTextureRange textureRange = {}, ezEnum<ezGALResourceFormat> overrideViewFormat = ezGALResourceFormat::Invalid);
  void BindBuffer(ezTempHashedString sSlotName, ezConstantBufferStorageHandle hBuffer, ezGALBufferRange bufferRange = {}, ezGALResourceFormat::Enum overrideTexelBufferFormat = ezGALResourceFormat::Invalid);

  /// Create a new bind group for the given layout.
  /// @param hBindGroupLayout The bind group layout for which to create the bind group.
  /// @param out_bindGroup The resulting bind group. Will be undefined if EZ_FAILURE is returned.
  void CreateBindGroup(ezGALBindGroupLayoutHandle hBindGroupLayout, ezGALBindGroupCreationDescription& out_bindGroup);

public:
  /// \brief Number of modifications of the hash tables each frame. Used for stats.
  static ezUInt32 s_uiWrites;
  /// \brief Number of reads of the hash tables each frame. Used for stats.
  static ezUInt32 s_uiReads;

private:
  void RemoveItem(ezTempHashedString sSlotName, ezHashTable<ezUInt64, ezGALBindGroupItem>& ref_Container);
  void InsertItem(ezTempHashedString sSlotName, const ezGALBindGroupItem& item, ezHashTable<ezUInt64, ezGALBindGroupItem>& ref_Container);

private:
  const ezGALDevice* m_pDevice = nullptr;
  bool m_bModified = true;
  ezGALSamplerStateHandle m_hDefaultSampler;
  ezHashTable<ezUInt64, ezGALBindGroupItem> m_BoundSamplers;
  ezHashTable<ezUInt64, ezGALBindGroupItem> m_BoundBuffers;
  ezHashTable<ezUInt64, ezGALBindGroupItem> m_BoundTextures;
};
