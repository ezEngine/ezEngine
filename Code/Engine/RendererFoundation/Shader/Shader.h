#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class EZ_RENDERERFOUNDATION_DLL ezGALShader : public ezGALObject<ezGALShaderCreationDescription>
{
public:
  virtual void SetDebugName(ezStringView sName) const = 0;

  /// \brief Returns the number of bind groups in the shader. Every bind group must be bound for the shader to be used.
  EZ_ALWAYS_INLINE ezUInt32 GetBindGroupCount() const { return m_BindGroupLayouts.GetCount(); }
  /// \brief Returns the layout of the given bind group.
  /// \param uiBindGroup Must be less than GetBindGroupCount.
  EZ_ALWAYS_INLINE ezGALBindGroupLayoutHandle GetBindGroupLayout(ezUInt32 uiBindGroup = 0) const { return m_BindGroupLayouts[uiBindGroup]; }
  /// \brief Returns the pipeline layout for this shader. I.e. the umbrella of all bind group layouts. This can be used to e.g. sort draw calls by to reduce state changes.
  EZ_ALWAYS_INLINE ezGALPipelineLayoutHandle GetPipelineLayout() const { return m_hPipelineLayout; }
  /// Convenience function that returns ezGALBindGroupLayoutCreationDescription::m_ResourceBindings of the given bind group layout.
  ezArrayPtr<const ezShaderResourceBinding> GetBindings(ezUInt32 uiBindGroup = 0) const;

  /// Returns the list of vertex input attributes. Compute shaders return an empty array.
  ezArrayPtr<const ezShaderVertexInputAttribute> GetVertexInputAttributes() const;

protected:
  friend class ezGALDevice;

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;

  ezResult CreateBindingMapping(bool bAllowMultipleBindingPerName);
  void DestroyBindingMapping();
  ezResult CreateLayouts(ezGALDevice* pDevice, bool bSupportsImmutableSamplers);
  void DestroyLayouts(ezGALDevice* pDevice);

  ezGALShader(const ezGALShaderCreationDescription& Description);
  virtual ~ezGALShader();

protected:
  ezGALDevice* m_pDevice = nullptr;
  ezDynamicArray<ezShaderResourceBinding> m_BindingMapping;

  ezHybridArray<ezGALBindGroupLayoutHandle, EZ_GAL_MAX_BIND_GROUPS> m_BindGroupLayouts;
  ezGALPipelineLayoutHandle m_hPipelineLayout;
};
