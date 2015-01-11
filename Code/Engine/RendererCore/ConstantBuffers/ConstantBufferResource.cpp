#include <RendererCore/PCH.h>
#include <RendererCore/ConstantBuffers/ConstantBufferResource.h>
#include <Foundation/Logging/Log.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Context/Context.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezConstantBufferResource, ezResourceBase, 1, ezRTTIDefaultAllocator<ezConstantBufferResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE();


ezConstantBufferResource::ezConstantBufferResource()
{
  m_uiMaxQualityLevel = 1;
  m_Flags |= ezResourceFlags::UpdateOnMainThread;
}

void ezConstantBufferResource::UnloadData(bool bFullUnload)
{
  m_uiLoadedQualityLevel = 0;
  m_LoadingState = ezResourceLoadState::Uninitialized;

  m_Bytes.Clear();

  ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hGALConstantBuffer);
  m_hGALConstantBuffer.Invalidate();
}

void ezConstantBufferResource::UpdateContent(ezStreamReaderBase* Stream)
{
  EZ_REPORT_FAILURE("This resource type does not support loading data from file.");
}

void ezConstantBufferResource::UpdateMemoryUsage()
{
  SetMemoryUsageCPU(m_Bytes.GetCount());
  SetMemoryUsageGPU(m_Bytes.GetCount());
}

void ezConstantBufferResource::CreateResource(const ezConstantBufferResourceDescriptor& descriptor)
{
  m_bHasBeenModified = true;
  m_Bytes = std::move(descriptor.m_Bytes);

  m_uiLoadedQualityLevel = 1;
  m_LoadingState = ezResourceLoadState::Loaded;

  m_hGALConstantBuffer = ezGALDevice::GetDefaultDevice()->CreateConstantBuffer(m_Bytes.GetCount());
}

void ezConstantBufferResource::UploadStateToGPU(ezGALContext* pContext)
{
  if (!m_bHasBeenModified)
    return;

  m_bHasBeenModified = false;

  /// \todo Does it have benefits to only upload a part of the constant buffer? If yes, it should be easy to track the modified range

  pContext->UpdateBuffer(m_hGALConstantBuffer, 0, m_Bytes.GetData(), m_Bytes.GetCount());
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_ConstantBuffers_ConstantBufferResource);

