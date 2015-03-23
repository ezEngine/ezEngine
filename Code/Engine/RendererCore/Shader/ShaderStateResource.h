#pragma once

#include <RendererCore/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

typedef ezResourceHandle<class ezShaderStateResource> ezShaderStateResourceHandle;

struct EZ_RENDERERCORE_DLL ezShaderStateResourceDescriptor
{
  ezGALBlendStateCreationDescription m_BlendDesc;
  ezGALDepthStencilStateCreationDescription m_DepthStencilDesc;
  ezGALRasterizerStateCreationDescription m_RasterizerDesc;
  //ezGALSamplerStateCreationDescription m_SamplerDesc;

  ezResult Load(const char* szSource);
  void Load(ezStreamReaderBase& stream);
  void Save(ezStreamWriterBase& stream) const;

  ezUInt32 CalculateHash() const;
};

class EZ_RENDERERCORE_DLL ezShaderStateResource : public ezResource<ezShaderStateResource, ezShaderStateResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezShaderStateResource);

public:
  ezShaderStateResource();

  const ezGALBlendStateHandle& GetBlendState() const { return m_hBlendState; }
  const ezGALDepthStencilStateHandle& GetDepthStencilState() const { return m_hDepthStencilState; }
  const ezGALRasterizerStateHandle& GetRasterizerState() const { return m_hRasterizerState; }


private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReaderBase* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceLoadDesc CreateResource(const ezShaderStateResourceDescriptor& descriptor) override;

  ezGALBlendStateHandle m_hBlendState;
  ezGALDepthStencilStateHandle m_hDepthStencilState;
  ezGALRasterizerStateHandle m_hRasterizerState;
  // ezGALSamplerStateHandle m_hSamplerState; ???
};




