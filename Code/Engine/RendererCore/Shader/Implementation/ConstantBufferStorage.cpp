#include <RendererCorePCH.h>

#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>

ezConstantBufferStorageBase::ezConstantBufferStorageBase(ezUInt32 uiSizeInBytes)
  : m_bHasBeenModified(false)
  , m_uiLastHash(0)
{
  m_Data = ezMakeArrayPtr(static_cast<ezUInt8*>(ezFoundation::GetAlignedAllocator()->Allocate(uiSizeInBytes, 16)), uiSizeInBytes);

  m_hGALConstantBuffer = ezGALDevice::GetDefaultDevice()->CreateConstantBuffer(uiSizeInBytes);
}

ezConstantBufferStorageBase::~ezConstantBufferStorageBase()
{
  ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hGALConstantBuffer);

  ezFoundation::GetAlignedAllocator()->Deallocate(m_Data.GetPtr());
  m_Data.Clear();
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

void ezConstantBufferStorageBase::UploadData(ezGALCommandEncoder* pCommandEncoder)
{
  if (!m_bHasBeenModified)
    return;

  m_bHasBeenModified = false;

  ezUInt32 uiNewHash = ezHashingUtils::xxHash32(m_Data.GetPtr(), m_Data.GetCount());
  if (m_uiLastHash != uiNewHash)
  {
    pCommandEncoder->UpdateBuffer(m_hGALConstantBuffer, 0, m_Data);
    m_uiLastHash = uiNewHash;
  }
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Shader_Implementation_ConstantBufferStorage);
