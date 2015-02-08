#pragma once

#include <RendererCore/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>

typedef ezResourceHandle<class ezShaderResource> ezShaderResourceHandle;

struct ezShaderResourceDescriptor
{
};

class EZ_RENDERERCORE_DLL ezShaderResource : public ezResource<ezShaderResource, ezShaderResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezShaderResource);

public:
  ezShaderResource();

  bool IsShaderValid() const { return m_bShaderResourceIsValid; }

  const ezString& GetUsedPermutationVars() const { return m_PermutationVarsUsed; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReaderBase* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  ezString m_PermutationVarsUsed;
  bool m_bShaderResourceIsValid;
};

