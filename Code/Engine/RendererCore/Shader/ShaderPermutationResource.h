#pragma once

#include <RendererCore/Basics.h>
#include <RendererCore/Shader/ShaderPermutationBinary.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Time/Timestamp.h>

class ezShaderPermutationResource;
typedef ezResourceHandle<ezShaderPermutationResource> ezShaderPermutationResourceHandle;

struct ezShaderPermutationResourceDescriptor
{
};

class EZ_RENDERERCORE_DLL ezShaderPermutationResource : public ezResource<ezShaderPermutationResource, ezShaderPermutationResourceDescriptor>
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
  virtual ezResourceTypeLoader* GetDefaultResourceTypeLoader() const override;

private:

  ezShaderPermutationBinary m_PermutationBinary;

  bool m_bValid;
  ezGALShaderHandle m_hShader;
  ezGALVertexDeclarationHandle m_hVertexDeclaration;
};


class ezShaderPermutationResourceLoader : public ezResourceTypeLoader
{
public:

  virtual ezResourceLoadData OpenDataStream(const ezResourceBase* pResource) override;
  virtual void CloseDataStream(const ezResourceBase* pResource, const ezResourceLoadData& LoaderData) override;

private:

  ezResult RunCompiler(const ezResourceBase* pResource, ezShaderPermutationBinary& BinaryInfo, bool bForce);
  ezTimestamp GetFileTimestamp(const char* szFile);

  struct FileCheckCache
  {
    ezTimestamp m_FileTimestamp;
    ezTime m_LastCheck;
  };

  ezMap<ezString, FileCheckCache> m_FileTimestamps;
};