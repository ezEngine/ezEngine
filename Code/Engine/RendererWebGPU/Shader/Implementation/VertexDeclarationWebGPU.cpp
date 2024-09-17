#include <RendererWebGPU/RendererWebGPUPCH.h>

#include <RendererFoundation/Shader/Shader.h>
#include <RendererWebGPU/Device/DeviceWebGPU.h>
#include <RendererWebGPU/Shader/VertexDeclarationWebGPU.h>

ezGALVertexDeclarationWebGPU::ezGALVertexDeclarationWebGPU(const ezGALVertexDeclarationCreationDescription& Description)
  : ezGALVertexDeclaration(Description)
{
}

ezGALVertexDeclarationWebGPU::~ezGALVertexDeclarationWebGPU() = default;

static ezUInt32 ToSize(wgpu::VertexFormat format)
{
  switch (format)
  {
    case wgpu::VertexFormat::Uint8x2:
    case wgpu::VertexFormat::Sint8x2:
    case wgpu::VertexFormat::Unorm8x2:
    case wgpu::VertexFormat::Snorm8x2:
      return 2;

    case wgpu::VertexFormat::Uint8x4:
    case wgpu::VertexFormat::Sint8x4:
    case wgpu::VertexFormat::Unorm8x4:
    case wgpu::VertexFormat::Snorm8x4:
      return 4;

    case wgpu::VertexFormat::Uint16x2:
    case wgpu::VertexFormat::Sint16x2:
    case wgpu::VertexFormat::Unorm16x2:
    case wgpu::VertexFormat::Snorm16x2:
      return 4;

    case wgpu::VertexFormat::Uint16x4:
    case wgpu::VertexFormat::Sint16x4:
    case wgpu::VertexFormat::Unorm16x4:
    case wgpu::VertexFormat::Snorm16x4:
      return 8;

    case wgpu::VertexFormat::Float16x2:
      return 4;

    case wgpu::VertexFormat::Float16x4:
      return 8;

    case wgpu::VertexFormat::Float32:
      return 4;

    case wgpu::VertexFormat::Float32x2:
      return 8;

    case wgpu::VertexFormat::Float32x3:
      return 12;

    case wgpu::VertexFormat::Float32x4:
      return 16;

    case wgpu::VertexFormat::Uint32:
    case wgpu::VertexFormat::Sint32:
      return 4;

    case wgpu::VertexFormat::Uint32x2:
    case wgpu::VertexFormat::Sint32x2:
      return 8;

    case wgpu::VertexFormat::Uint32x3:
    case wgpu::VertexFormat::Sint32x3:
      return 12;

    case wgpu::VertexFormat::Uint32x4:
    case wgpu::VertexFormat::Sint32x4:
      return 16;

    case wgpu::VertexFormat::Unorm10_10_10_2:
      return 4;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
  return 0;
}

static wgpu::VertexFormat ToWGPU(ezGALResourceFormat::Enum format)
{
  switch (format)
  {
    case ezGALResourceFormat::RGBAFloat:
      return wgpu::VertexFormat::Float32x4;

    case ezGALResourceFormat::RGBAUInt:
      return wgpu::VertexFormat::Uint32x4;

    case ezGALResourceFormat::RGBAInt:
      return wgpu::VertexFormat::Sint32x4;

    case ezGALResourceFormat::RGBFloat:
      return wgpu::VertexFormat::Float32x3;

    case ezGALResourceFormat::BGRAUByteNormalized: // ?
      return wgpu::VertexFormat::Unorm8x4;

    case ezGALResourceFormat::RGBAUShort:
      return wgpu::VertexFormat::Uint16x4;

    case ezGALResourceFormat::RGBAUShortNormalized:
      return wgpu::VertexFormat::Unorm16x4;

    case ezGALResourceFormat::RGBAShort:
      return wgpu::VertexFormat::Sint16x4;

    case ezGALResourceFormat::RGBAShortNormalized:
      return wgpu::VertexFormat::Snorm16x4;

    case ezGALResourceFormat::RGFloat:
      return wgpu::VertexFormat::Float32x2;

    case ezGALResourceFormat::RGUInt:
      return wgpu::VertexFormat::Uint32x2;

    case ezGALResourceFormat::RGInt:
      return wgpu::VertexFormat::Sint32x2;

    case ezGALResourceFormat::RGB10A2UIntNormalized:
      return wgpu::VertexFormat::Unorm10_10_10_2;

    case ezGALResourceFormat::RGBAUByteNormalized:
      return wgpu::VertexFormat::Unorm8x4;

    case ezGALResourceFormat::RGBAUByte:
      return wgpu::VertexFormat::Uint8x4;

    case ezGALResourceFormat::RGBAByteNormalized:
      return wgpu::VertexFormat::Snorm8x4;

    case ezGALResourceFormat::RGBAByte:
      return wgpu::VertexFormat::Sint8x4;

    case ezGALResourceFormat::RGUShort:
      return wgpu::VertexFormat::Uint16x2;

    case ezGALResourceFormat::RGUShortNormalized:
      return wgpu::VertexFormat::Unorm16x2;

    case ezGALResourceFormat::RGShort:
      return wgpu::VertexFormat::Sint16x2;

    case ezGALResourceFormat::RGShortNormalized:
      return wgpu::VertexFormat::Snorm16x2;

    case ezGALResourceFormat::RGUByte:
      return wgpu::VertexFormat::Uint8x2;

    case ezGALResourceFormat::RGUByteNormalized:
      return wgpu::VertexFormat::Unorm8x2;

    case ezGALResourceFormat::RGByte:
      return wgpu::VertexFormat::Sint8x2;

    case ezGALResourceFormat::RGByteNormalized:
      return wgpu::VertexFormat::Snorm8x2;

    case ezGALResourceFormat::RFloat:
      return wgpu::VertexFormat::Float32;

    case ezGALResourceFormat::RUInt:
      return wgpu::VertexFormat::Uint32;

    case ezGALResourceFormat::RInt:
      return wgpu::VertexFormat::Sint32;

    case ezGALResourceFormat::RGHalf:
      return wgpu::VertexFormat::Float16x2;

    case ezGALResourceFormat::RGBAHalf:
      return wgpu::VertexFormat::Float16x4;

    case ezGALResourceFormat::RGBUInt:
      return wgpu::VertexFormat::Uint32x3;
    case ezGALResourceFormat::RGBInt:
      return wgpu::VertexFormat::Sint32x3;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
  return wgpu::VertexFormat(0);
}

EZ_DEFINE_AS_POD_TYPE(wgpu::VertexAttribute);

ezResult ezGALVertexDeclarationWebGPU::InitPlatform(ezGALDevice* pDevice)
{
  // EZ_WEBGPU_TRACE();

  m_Attributes.SetCount(m_Description.m_VertexAttributes.GetCount());

  ezUInt32 uiTotalSize = 0;

  for (ezUInt32 i = 0; i < m_Attributes.GetCount(); ++i)
  {
    const auto& org = m_Description.m_VertexAttributes[i];
    auto& wga = m_Attributes[i];

    wga.offset = org.m_uiOffset;
    wga.shaderLocation = (int)org.m_eSemantic; // TODO WebGPU: need a mapping for m_eSemantic to shaderLocation
    wga.format = ToWGPU(org.m_eFormat);

    uiTotalSize += ToSize(wga.format);
  }

  m_Layout.arrayStride = uiTotalSize;
  m_Layout.attributeCount = m_Attributes.GetCount();
  m_Layout.attributes = m_Attributes.GetData();
  m_Layout.stepMode = wgpu::VertexStepMode::Vertex;

  return EZ_SUCCESS;
}

ezResult ezGALVertexDeclarationWebGPU::DeInitPlatform(ezGALDevice* pDevice)
{
  m_Attributes.Clear();
  return EZ_SUCCESS;
}
