#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class EZ_RENDERERFOUNDATION_DLL ezGALShader : public ezGALObject<ezGALShaderCreationDescription>
{
public:
  virtual void SetDebugName(const char* szName) const = 0;

  /// Returns the list of shader resources and their binding information. These must be bound before the shader can be used.
  ezArrayPtr<const ezShaderResourceBinding> GetBindingMapping() const;
  /// Convenience function that finds 'sName' in GetBindingMapping and returns it if present.
  const ezShaderResourceBinding* GetShaderResourceBinding(const ezTempHashedString& sName) const;

  /// Returns the list of vertex input attributes. Compute shaders return an empty array.
  ezArrayPtr<const ezShaderVertexInputAttribute> GetVertexInputAttributes() const;

protected:
  friend class ezGALDevice;

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;

  ezResult CreateBindingMapping(bool bAllowMultipleBindingPerName);
  void DestroyBindingMapping();

  ezGALShader(const ezGALShaderCreationDescription& Description);
  virtual ~ezGALShader();

protected:
  ezDynamicArray<ezShaderResourceBinding> m_BindingMapping;
};
