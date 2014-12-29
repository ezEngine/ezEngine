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
  const ezShaderStageBinary* GetShaderStageBinary(ezGALShaderStage::Enum stage) const { return m_pShaderStageBinaries[stage]; }

  bool IsShaderValid() const { return m_bShaderPermutationValid; }

private:
  virtual void UnloadData(bool bFullUnload) override;
  virtual void UpdateContent(ezStreamReaderBase* Stream) override;
  virtual void UpdateMemoryUsage() override;
  virtual ezResourceTypeLoader* GetDefaultResourceTypeLoader() const override;

private:

  ezShaderPermutationBinary m_PermutationBinary;
  ezShaderStageBinary* m_pShaderStageBinaries[ezGALShaderStage::ENUM_COUNT];

  bool m_bShaderPermutationValid;
  ezGALShaderHandle m_hShader;
};


class ezShaderPermutationResourceLoader : public ezResourceTypeLoader
{
public:

  virtual ezResourceLoadData OpenDataStream(const ezResourceBase* pResource) override;
  virtual void CloseDataStream(const ezResourceBase* pResource, const ezResourceLoadData& LoaderData) override;

  virtual bool IsResourceOutdated(const ezResourceBase* pResource) const override { return true; }

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