#pragma once

#include <RendererCore/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <RendererFoundation/Basics.h>

typedef ezResourceHandle<class ezConstantBufferResource> ezConstantBufferResourceHandle;

struct ezConstantBufferResourceDescriptorBase
{
protected:
  ezConstantBufferResourceDescriptorBase() { m_pBytes = nullptr; m_uiSize = 0; }

  friend class ezConstantBufferResource;
  ezUInt8* m_pBytes;
  ezUInt32 m_uiSize;
};

template<typename STRUCT>
struct ezConstantBufferResourceDescriptor : public ezConstantBufferResourceDescriptorBase
{
  ezConstantBufferResourceDescriptor()
  {
    m_pBytes = reinterpret_cast<ezUInt8*>(&m_Data);
    m_uiSize = sizeof(STRUCT);
  }

  STRUCT m_Data;
};

class EZ_RENDERERCORE_DLL ezConstantBufferResource : public ezResource<ezConstantBufferResource, ezConstantBufferResourceDescriptorBase>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezConstantBufferResource);

public:
  ezConstantBufferResource();

private:
  virtual void UnloadData(bool bFullUnload) override;
  virtual void UpdateContent(ezStreamReaderBase* Stream) override;
  virtual void UpdateMemoryUsage() override;
  virtual void CreateResource(const ezConstantBufferResourceDescriptorBase& descriptor) override;

private:
  friend class ezRendererCore;
  ezGALBufferHandle GetGALBufferHandle() const { return m_hGALConstantBuffer; }

  void UploadStateToGPU(ezGALContext* pContext);

  bool m_bHasBeenModified;
  ezGALBufferHandle m_hGALConstantBuffer;
  ezDynamicArray<ezUInt8> m_Bytes;
};
