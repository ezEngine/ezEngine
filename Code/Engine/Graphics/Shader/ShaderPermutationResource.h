#pragma once

#include <Graphics/Basics.h>
#include <Graphics/Shader/ShaderPermutationBinary.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>

class ezShaderPermutationResource;
typedef ezResourceHandle<ezShaderPermutationResource> ezShaderPermutationResourceHandle;

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GRAPHICS_DLL, ezShaderPermutationResource);

struct ezShaderPermutationResourceDescriptor
{
};

class EZ_GRAPHICS_DLL ezShaderPermutationResource : public ezResource<ezShaderPermutationResource, ezShaderPermutationResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezShaderPermutationResource);

public:
  ezShaderPermutationResource();

  ezGALShaderHandle GetGALShader() const { return m_hShader; }
  ezGALVertexDeclarationHandle GetGALVertexDeclaration() const { return m_hVertexDeclaration; }

  bool IsShaderValid() const { return m_bValid; }

private:
  virtual void UnloadData(bool bFullUnload) override;
  virtual void UpdateContent(ezStreamReaderBase& Stream) override;
  virtual void UpdateMemoryUsage() override;
  virtual void CreateResource(const ezShaderPermutationResourceDescriptor& descriptor) override;

private:

  ezShaderPermutationBinary m_PermutationBinary;

  bool m_bValid;
  ezGALShaderHandle m_hShader;
  ezGALVertexDeclarationHandle m_hVertexDeclaration;
};

