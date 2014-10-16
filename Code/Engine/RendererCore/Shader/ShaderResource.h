#pragma once

#include <RendererCore/Basics.h>
#include <RendererCore/Shader/ShaderPermutationBinary.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>

class ezShaderResource;
typedef ezResourceHandle<ezShaderResource> ezShaderResourceHandle;

struct ezShaderResourceDescriptor
{
};

class EZ_RENDERERCORE_DLL ezShaderResource : public ezResource<ezShaderResource, ezShaderResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezShaderResource);

public:
  ezShaderResource();

  const ezString& GetUsedPermutationVars() const { return m_PermutationVarsUsed; }

private:
  virtual void UnloadData(bool bFullUnload) override;
  virtual void UpdateContent(ezStreamReaderBase& Stream) override;
  virtual void UpdateMemoryUsage() override;
  virtual void CreateResource(const ezShaderResourceDescriptor& descriptor) override;

private:
  ezString m_PermutationVarsUsed;

};

