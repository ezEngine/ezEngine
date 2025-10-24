#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Time/Timestamp.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererCore/Shader/ShaderPermutationBinary.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>

using ezShaderPermutationResourceHandle = ezTypedResourceHandle<class ezShaderPermutationResource>;
using ezShaderStateResourceHandle = ezTypedResourceHandle<class ezShaderStateResource>;

/// Descriptor for shader permutation resources.
struct ezShaderPermutationResourceDescriptor
{
  // empty
};

/// Runtime resource representing a specific shader permutation variant.
///
/// Shaders use permutation variables to create variants for different features (e.g., with/without shadows,
/// skinning, etc.). Each unique combination of permutation values results in a separate compiled shader.
/// This resource holds the compiled shader bytecode, render states, and permutation variable values.
class EZ_RENDERERCORE_DLL ezShaderPermutationResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezShaderPermutationResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezShaderPermutationResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezShaderPermutationResource, ezShaderPermutationResourceDescriptor);

public:
  ezShaderPermutationResource();

  ezGALShaderHandle GetGALShader() const { return m_hShader; }
  const ezGALShaderByteCode* GetShaderByteCode(ezGALShaderStage::Enum stage) const { return m_ByteCodes[stage]; }

  ezGALBlendStateHandle GetBlendState() const { return m_hBlendState; }
  ezGALDepthStencilStateHandle GetDepthStencilState() const { return m_hDepthStencilState; }
  ezGALRasterizerStateHandle GetRasterizerState() const { return m_hRasterizerState; }

  /// Returns true if the shader compiled successfully.
  bool IsShaderValid() const { return m_bShaderPermutationValid; }

  /// Returns the permutation variable values that define this shader variant.
  ezArrayPtr<const ezPermutationVar> GetPermutationVars() const { return m_PermutationVars; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceTypeLoader* GetDefaultResourceTypeLoader() const override;

private:
  friend class ezShaderManager;

  ezSharedPtr<const ezGALShaderByteCode> m_ByteCodes[ezGALShaderStage::ENUM_COUNT];

  bool m_bShaderPermutationValid;
  ezGALShaderHandle m_hShader;

  ezGALBlendStateHandle m_hBlendState;
  ezGALDepthStencilStateHandle m_hDepthStencilState;
  ezGALRasterizerStateHandle m_hRasterizerState;

  ezHybridArray<ezPermutationVar, 16> m_PermutationVars;
};


/// Resource loader for shader permutation resources.
///
/// Handles loading compiled shader bytecode. If the permutation is not found or outdated,
/// it triggers shader compilation on-demand.
class ezShaderPermutationResourceLoader : public ezResourceTypeLoader
{
public:
  virtual ezResourceLoadData OpenDataStream(const ezResource* pResource) override;
  virtual void CloseDataStream(const ezResource* pResource, const ezResourceLoadData& loaderData) override;

  virtual bool IsResourceOutdated(const ezResource* pResource) const override;

private:
  ezResult RunCompiler(const ezResource* pResource, ezShaderPermutationBinary& BinaryInfo, bool bForce);
};
