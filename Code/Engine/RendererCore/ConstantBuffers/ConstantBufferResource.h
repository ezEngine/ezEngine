#pragma once

#include <RendererCore/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <RendererFoundation/Basics.h>

typedef ezResourceHandle<class ezConstantBufferResource> ezConstantBufferResourceHandle;

struct ezConstantBufferResourceDescriptor
{
  template<typename STRUCT>
  void Initialize()
  {
    m_Bytes.SetCount(sizeof(STRUCT));
    ezMemoryUtils::Construct<STRUCT>(m_Bytes.GetData());
  }

  ezDynamicArray<ezUInt8> m_Bytes;
};

class EZ_RENDERERCORE_DLL ezConstantBufferResource : public ezResource<ezConstantBufferResource, ezConstantBufferResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezConstantBufferResource);

public:
  ezConstantBufferResource();

  void UploadStateToGPU(ezGALContext* pContext);

  ezGALBufferHandle GetGALBufferHandle() const { return m_hGALConstantBuffer; }

  template<typename STRUCT>
  STRUCT& Modify()
  {
    m_bHasBeenModified = true;
    return *reinterpret_cast<STRUCT*>(m_Bytes.GetData());
  }

private:
  virtual void UnloadData(bool bFullUnload) override;
  virtual void UpdateContent(ezStreamReaderBase* Stream) override;
  virtual void UpdateMemoryUsage() override;
  virtual void CreateResource(const ezConstantBufferResourceDescriptor& descriptor) override;

private:

  bool m_bHasBeenModified;
  ezGALBufferHandle m_hGALConstantBuffer;
  ezDynamicArray<ezUInt8> m_Bytes;
};
