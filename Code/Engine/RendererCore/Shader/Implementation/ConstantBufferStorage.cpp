#include <RendererCore/PCH.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>


ezConstantBufferStorageBase::ezConstantBufferStorageBase(ezUInt32 uiSizeInBytes)
{
  m_Data = ezMakeArrayPtr(static_cast<ezUInt8*>(ezFoundation::GetAlignedAllocator()->Allocate(uiSizeInBytes, 16)), uiSizeInBytes);

  m_hGALConstantBuffer = ezGALDevice::GetDefaultDevice()->CreateConstantBuffer(uiSizeInBytes);
}

ezConstantBufferStorageBase::~ezConstantBufferStorageBase()
{
  ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hGALConstantBuffer);
  
  ezFoundation::GetAlignedAllocator()->Deallocate(m_Data.GetPtr());
  m_Data.Reset();
}

ezArrayPtr<ezUInt8> ezConstantBufferStorageBase::GetRawDataForWriting()
{
  m_bHasBeenModified = true;
  return m_Data;
}

ezArrayPtr<const ezUInt8> ezConstantBufferStorageBase::GetRawDataForReading() const
{
  return m_Data;
}

void ezConstantBufferStorageBase::UploadData(ezGALContext* pContext)
{
  if (!m_bHasBeenModified)
    return;

  m_bHasBeenModified = false;

  pContext->UpdateBuffer(m_hGALConstantBuffer, 0, m_Data);
}


