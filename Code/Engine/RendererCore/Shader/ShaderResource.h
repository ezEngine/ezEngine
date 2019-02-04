#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Basics.h>

typedef ezTypedResourceHandle<class ezShaderResource> ezShaderResourceHandle;

struct ezShaderResourceDescriptor
{
};

class EZ_RENDERERCORE_DLL ezShaderResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezShaderResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezShaderResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezShaderResource, ezShaderResourceDescriptor);

public:
  ezShaderResource();

  bool IsShaderValid() const { return m_bShaderResourceIsValid; }

  ezArrayPtr<const ezHashedString> GetUsedPermutationVars() const { return m_PermutationVarsUsed; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  ezHybridArray<ezHashedString, 16> m_PermutationVarsUsed;
  bool m_bShaderResourceIsValid;
};

