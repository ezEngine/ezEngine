#include <RendererWebGPU/RendererWebGPUPCH.h>

#include <RendererWebGPU/Device/DeviceWebGPU.h>
#include <RendererWebGPU/RendererWebGPUDLL.h>
#include <RendererWebGPU/Resources/BufferWebGPU.h>

ezGALBufferWebGPU::ezGALBufferWebGPU(const ezGALBufferCreationDescription& Description)
  : ezGALBuffer(Description)
{
}

ezGALBufferWebGPU::~ezGALBufferWebGPU() = default;

ezResult ezGALBufferWebGPU::InitPlatform(ezGALDevice* pDevice, ezArrayPtr<const ezUInt8> initialData)
{
  // EZ_WEBGPU_TRACE();

  ezGALDeviceWebGPU* pRealDevice = static_cast<ezGALDeviceWebGPU*>(pDevice);

  EZ_ASSERT_DEV(initialData.GetCount() <= m_Description.m_uiTotalSize, "Buffer not large enough.");

  wgpu::BufferDescriptor bd;
  bd.size = m_Description.m_uiTotalSize;
  bd.usage = wgpu::BufferUsage::None;

  // m_Description.m_Format // unused ?
  // m_Description.m_uiStructSize // unused ?

  switch (m_Description.m_ResourceAccess.m_MemoryUsage)
  {
    case ezGALMemoryUsage::Staging:
      bd.usage |= wgpu::BufferUsage::CopySrc;
      bd.usage |= wgpu::BufferUsage::MapWrite;
      break;
    case ezGALMemoryUsage::Readback:
      bd.usage |= wgpu::BufferUsage::CopyDst;
      bd.usage |= wgpu::BufferUsage::MapRead;
      break;
    default:
      break;
  }

  if (m_Description.m_ResourceAccess.IsImmutable() == false)
  {
    bd.usage |= wgpu::BufferUsage::CopyDst;
    // bd.usage |= wgpu::BufferUsage::MapWrite; // not allowed together with CopyDst
  }

  if (m_Description.m_BufferFlags.IsSet(ezGALBufferUsageFlags::VertexBuffer))
  {
    bd.usage |= wgpu::BufferUsage::Vertex;
  }
  if (m_Description.m_BufferFlags.IsSet(ezGALBufferUsageFlags::IndexBuffer))
  {
    bd.usage |= wgpu::BufferUsage::Index;
  }
  if (m_Description.m_BufferFlags.IsSet(ezGALBufferUsageFlags::ConstantBuffer))
  {
    bd.usage |= wgpu::BufferUsage::Uniform;
  }
  if (m_Description.m_BufferFlags.IsSet(ezGALBufferUsageFlags::DrawIndirect))
  {
    bd.usage |= wgpu::BufferUsage::Indirect;
    EZ_ASSERT_NOT_IMPLEMENTED;
  }
  if (m_Description.m_BufferFlags.IsSet(ezGALBufferUsageFlags::ShaderResource))
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }
  if (m_Description.m_BufferFlags.IsSet(ezGALBufferUsageFlags::StructuredBuffer))
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }
  if (m_Description.m_BufferFlags.IsSet(ezGALBufferUsageFlags::TexelBuffer))
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }
  if (m_Description.m_BufferFlags.IsSet(ezGALBufferUsageFlags::UnorderedAccess))
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  bd.mappedAtCreation = true;

  m_Buffer = pRealDevice->GetDevice().CreateBuffer(&bd);

  ezMemoryUtils::RawByteCopy(m_Buffer.GetMappedRange(), initialData.GetPtr(), initialData.GetCount());

  m_Buffer.Unmap();

  return EZ_SUCCESS;
}

ezResult ezGALBufferWebGPU::DeInitPlatform(ezGALDevice* pDevice)
{
  if (m_Buffer)
  {
    m_Buffer.Destroy();
    m_Buffer = nullptr;
  }

  return EZ_SUCCESS;
}

void ezGALBufferWebGPU::SetDebugNamePlatform(const char* szName) const
{
  if (m_Buffer)
  {
    m_Buffer.SetLabel(szName);
  }
}
