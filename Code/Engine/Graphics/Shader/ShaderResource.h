#pragma once

#include <Graphics/Basics.h>
#include <Graphics/Shader/ShaderPermutationBinary.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>

class ezShaderResource;
typedef ezResourceHandle<ezShaderResource> ezShaderResourceHandle;

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GRAPHICS_DLL, ezShaderResource);

struct ezShaderResourceDescriptor
{
};

class EZ_GRAPHICS_DLL ezShaderResource : public ezResource<ezShaderResource, ezShaderResourceDescriptor>
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

