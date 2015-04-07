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

struct ezConstantBufferResourceDescriptorRawBytes : public ezConstantBufferResourceDescriptorBase
{
  ezConstantBufferResourceDescriptorRawBytes(ezUInt32 uiBytes)
  {
    m_Data.SetCount(uiBytes);

    m_pBytes = m_Data.GetData();
    m_uiSize = uiBytes;
  }

  ezDynamicArray<ezUInt8> m_Data;
};

class EZ_RENDERERCORE_DLL ezConstantBufferResource : public ezResource<ezConstantBufferResource, ezConstantBufferResourceDescriptorBase>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezConstantBufferResource);

public:
  ezConstantBufferResource();

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReaderBase* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceLoadDesc CreateResource(const ezConstantBufferResourceDescriptorBase& descriptor) override;


private:
  friend class ezRenderContext;
  ezGALBufferHandle GetGALBufferHandle() const { return m_hGALConstantBuffer; }

  void UploadStateToGPU(ezGALContext* pContext);

  bool m_bHasBeenModified;
  ezGALBufferHandle m_hGALConstantBuffer;
  ezDynamicArray<ezUInt8> m_Bytes;
};
