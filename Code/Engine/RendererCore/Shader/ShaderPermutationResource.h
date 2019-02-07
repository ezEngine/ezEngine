#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Time/Timestamp.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererCore/Shader/ShaderPermutationBinary.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>

typedef ezTypedResourceHandle<class ezShaderPermutationResource> ezShaderPermutationResourceHandle;
typedef ezTypedResourceHandle<class ezShaderStateResource> ezShaderStateResourceHandle;

struct ezShaderPermutationResourceDescriptor
{
};

class EZ_RENDERERCORE_DLL ezShaderPermutationResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezShaderPermutationResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezShaderPermutationResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezShaderPermutationResource, ezShaderPermutationResourceDescriptor);

public:
  ezShaderPermutationResource();

  ezGALShaderHandle GetGALShader() const { return m_hShader; }
  const ezShaderStageBinary* GetShaderStageBinary(ezGALShaderStage::Enum stage) const { return m_pShaderStageBinaries[stage]; }

  ezGALBlendStateHandle GetBlendState() const { return m_hBlendState; }
  ezGALDepthStencilStateHandle GetDepthStencilState() const { return m_hDepthStencilState; }
  ezGALRasterizerStateHandle GetRasterizerState() const { return m_hRasterizerState; }

  bool IsShaderValid() const { return m_bShaderPermutationValid; }

  ezArrayPtr<const ezPermutationVar> GetPermutationVars() const { return m_PermutationVars; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceTypeLoader* GetDefaultResourceTypeLoader() const override;

private:
  friend class ezShaderManager;

  ezShaderStageBinary* m_pShaderStageBinaries[ezGALShaderStage::ENUM_COUNT];

  bool m_bShaderPermutationValid;
  ezGALShaderHandle m_hShader;

  ezGALBlendStateHandle m_hBlendState;
  ezGALDepthStencilStateHandle m_hDepthStencilState;
  ezGALRasterizerStateHandle m_hRasterizerState;

  ezHybridArray<ezPermutationVar, 16> m_PermutationVars;
};


class ezShaderPermutationResourceLoader : public ezResourceTypeLoader
{
public:
  virtual ezResourceLoadData OpenDataStream(const ezResource* pResource) override;
  virtual void CloseDataStream(const ezResource* pResource, const ezResourceLoadData& LoaderData) override;

  virtual bool IsResourceOutdated(const ezResource* pResource) const override;

private:
  ezResult RunCompiler(const ezResource* pResource, ezShaderPermutationBinary& BinaryInfo, bool bForce);
};

