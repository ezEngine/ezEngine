#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>

ezConstantBufferStorageBase::ezConstantBufferStorageBase(ezUInt32 uiSizeInBytes)
{
  m_Data = ezMakeArrayPtr(static_cast<ezUInt8*>(ezFoundation::GetAlignedAllocator()->Allocate(uiSizeInBytes, 16)), uiSizeInBytes);
  ezMemoryUtils::ZeroFill(m_Data.GetPtr(), m_Data.GetCount());

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
  ezRenderContext::MarktConstantBufferStorageModified(this);
  return m_Data;
}

ezArrayPtr<const ezUInt8> ezConstantBufferStorageBase::GetRawDataForReading() const
{
  return m_Data;
}

void ezConstantBufferStorageBase::UploadData(ezGALCommandEncoder* pCommandEncoder)
{
  if (!m_bHasBeenModified && !m_bStartOfFrame)
    return;

  m_bHasBeenModified = false;

  ezUInt32 uiNewHash = ezHashingUtils::xxHash32(m_Data.GetPtr(), m_Data.GetCount());
  if (m_uiLastHash != uiNewHash || m_bStartOfFrame)
  {
    pCommandEncoder->UpdateBuffer(m_hGALConstantBuffer, 0, m_Data, ezGALUpdateMode::TransientConstantBuffer);
    m_uiLastHash = uiNewHash;
  }
  m_bStartOfFrame = false;
}
