#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class EZ_RENDERERFOUNDATION_DLL ezGALShader : public ezGALObject<ezGALShaderCreationDescription>
{
public:
  virtual void SetDebugName(ezStringView sName) const = 0;

  /// Returns the list of shader resources and their binding information. These must be bound before the shader can be used.
  /// TODO: This list across all sets will be removed once bind groups are implemented.
  ezArrayPtr<const ezShaderResourceBinding> GetBindingMapping() const;
  /// Convenience function that finds 'sName' in GetBindingMapping and returns it if present.
  const ezShaderResourceBinding* GetShaderResourceBinding(const ezTempHashedString& sName) const;

  EZ_ALWAYS_INLINE ezUInt32 GetSetCount() const { return m_BindGroupLayouts.GetCount(); }
  EZ_ALWAYS_INLINE ezGALBindGroupLayoutHandle GetBindGroupLayout(ezUInt32 uiSet = 0) const { return m_BindGroupLayouts[uiSet]; }
  EZ_ALWAYS_INLINE ezGALPipelineLayoutHandle GetPipelineLayout() const { return m_hPipelineLayout; }
  /// Convenience function that returns ezGALBindGroupLayoutCreationDescription::m_ResourceBindings of the given bind group layout.
  ezArrayPtr<const ezShaderResourceBinding> GetBindings(ezUInt32 uiSet = 0) const;

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

  ezHybridArray<ezGALBindGroupLayoutHandle, EZ_GAL_MAX_SETS> m_BindGroupLayouts;
  ezGALPipelineLayoutHandle m_hPipelineLayout;
};
