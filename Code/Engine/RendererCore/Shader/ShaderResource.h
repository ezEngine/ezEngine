#pragma once

#include <RendererCore/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <Foundation/Strings/HashedString.h>

typedef ezTypedResourceHandle<class ezShaderResource> ezShaderResourceHandle;

struct ezShaderResourceDescriptor
{
};

class EZ_RENDERERCORE_DLL ezShaderResource : public ezResource<ezShaderResource, ezShaderResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezShaderResource, ezResourceBase);

public:
  ezShaderResource();

  bool IsShaderValid() const { return m_bShaderResourceIsValid; }

  ezArrayPtr<const ezHashedString> GetUsedPermutationVars() const { return m_PermutationVarsUsed; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceLoadDesc CreateResource(const ezShaderResourceDescriptor& descriptor) override;

private:
  ezHybridArray<ezHashedString, 16> m_PermutationVarsUsed;
  bool m_bShaderResourceIsValid;
};

