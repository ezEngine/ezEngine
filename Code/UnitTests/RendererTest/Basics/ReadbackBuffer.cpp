#include <RendererTest/RendererTestPCH.h>

#include <Core/GameState/GameStateWindow.h>
#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/RendererReflection.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererTest/Basics/ReadbackBuffer.h>

void ezRendererTestReadbackBuffer::SetupSubTests()
{
  const ezGALDeviceCapabilities& caps = GetDeviceCapabilities();

  AddSubTest("VertexBuffer", SubTests::ST_VertexBuffer);
  AddSubTest("IndexBuffer", SubTests::ST_IndexBuffer);
  if (caps.m_bSupportsTexelBuffer)
  {
    AddSubTest("TexelBuffer", SubTests::ST_TexelBuffer);
  }
  AddSubTest("StructuredBuffer", SubTests::ST_StructuredBuffer);
  AddSubTest("ByteAddressBuffer", SubTests::ST_ByteAddressBuffer);
}

ezResult ezRendererTestReadbackBuffer::InitializeSubTest(ezInt32 iIdentifier)
{
  m_iFrame = -1;
  m_bReadbackInProgress = true;

  EZ_SUCCEED_OR_RETURN(ezGraphicsTest::InitializeSubTest(iIdentifier));
  EZ_SUCCEED_OR_RETURN(CreateWindow(320, 240));

  // m_hUVColorShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/ReadbackFloat.ezShader");

  switch (iIdentifier)
  {
    case ST_VertexBuffer:
    {
      ezDynamicArray<float> data;
      data.SetCountUninitialized(128);
      for (ezUInt32 i = 0; i < 128; i++)
      {
        data[i] = (float)i;
      }
      m_BufferData = data.GetByteArrayPtr();

      m_hBufferReadback = m_pDevice->CreateVertexBuffer(4 * sizeof(float), 32, m_BufferData.GetByteArrayPtr());
      EZ_ASSERT_DEBUG(!m_hBufferReadback.IsInvalidated(), "Failed to create buffer");
    }
    break;
    case ST_IndexBuffer:
    {
      ezDynamicArray<ezUInt32> data;
      data.SetCountUninitialized(128);
      for (ezUInt32 i = 0; i < 128; i++)
      {
        data[i] = i;
      }
      m_BufferData = data.GetByteArrayPtr();

      m_hBufferReadback = m_pDevice->CreateIndexBuffer(ezGALIndexType::UInt, 128, m_BufferData.GetByteArrayPtr());
      EZ_ASSERT_DEBUG(!m_hBufferReadback.IsInvalidated(), "Failed to create buffer");
    }
    break;
    case ST_TexelBuffer:
    {
      ezDynamicArray<ezUInt32> data;
      data.SetCountUninitialized(128);
      for (ezUInt32 i = 0; i < 128; i++)
      {
        data[i] = i;
      }
      m_BufferData = data.GetByteArrayPtr();

      ezGALBufferCreationDescription desc;
      desc.m_uiStructSize = 0;
      desc.m_uiTotalSize = 128 * ezGALResourceFormat::GetBitsPerElement(ezGALResourceFormat::RGBAUByteNormalized) / 8;
      desc.m_BufferFlags = ezGALBufferUsageFlags::TexelBuffer | ezGALBufferUsageFlags::ShaderResource;
      desc.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
      desc.m_ResourceAccess.m_bImmutable = false;
      m_hBufferReadback = m_pDevice->CreateBuffer(desc, m_BufferData.GetByteArrayPtr());
      EZ_ASSERT_DEBUG(!m_hBufferReadback.IsInvalidated(), "Failed to create buffer");
    }
    break;
    case ST_StructuredBuffer:
    {
      ezDynamicArray<ezColor> data;
      data.SetCountUninitialized(128);
      for (ezUInt32 i = 0; i < 128; i++)
      {
        data[i] = ezColor::MakeFromKelvin(1000 + 10 * i);
      }
      m_BufferData = data.GetByteArrayPtr();

      ezGALBufferCreationDescription desc;
      desc.m_uiStructSize = sizeof(ezColor);
      desc.m_uiTotalSize = 128 * desc.m_uiStructSize;
      desc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource;
      desc.m_ResourceAccess.m_bImmutable = false;
      m_hBufferReadback = m_pDevice->CreateBuffer(desc, m_BufferData.GetByteArrayPtr());
      EZ_ASSERT_DEBUG(!m_hBufferReadback.IsInvalidated(), "Failed to create buffer");
    }
    break;
    case ST_ByteAddressBuffer:
    {
      ezDynamicArray<ezUInt8> data;
      data.SetCountUninitialized(128);
      for (ezUInt8 i = 0; i < 128; i++)
      {
        data[i] = i;
      }
      m_BufferData = data.GetByteArrayPtr();

      ezGALBufferCreationDescription desc;
      desc.m_uiStructSize = 0;
      desc.m_uiTotalSize = 128 * sizeof(ezUInt8);
      desc.m_BufferFlags = ezGALBufferUsageFlags::ByteAddressBuffer | ezGALBufferUsageFlags::ShaderResource;
      desc.m_ResourceAccess.m_bImmutable = false;
      m_hBufferReadback = m_pDevice->CreateBuffer(desc, m_BufferData.GetByteArrayPtr());
      EZ_ASSERT_DEBUG(!m_hBufferReadback.IsInvalidated(), "Failed to create buffer");
    }
    break;
    default:
      break;
  }

  return EZ_SUCCESS;
}

ezResult ezRendererTestReadbackBuffer::DeInitializeSubTest(ezInt32 iIdentifier)
{
  m_Readback.Reset();
  if (!m_hBufferReadback.IsInvalidated())
  {
    m_pDevice->DestroyBuffer(m_hBufferReadback);
    m_hBufferReadback.Invalidate();
  }

  m_hComputeShader.Invalidate();

  DestroyWindow();

  if (ezGraphicsTest::DeInitializeSubTest(iIdentifier).Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezTestAppRun ezRendererTestReadbackBuffer::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  m_iFrame = uiInvocationCount;
  m_bCaptureImage = false;
  BeginFrame();
  ezTestAppRun res = ReadbackBuffer(uiInvocationCount);
  EndFrame();
  return res;
}


ezTestAppRun ezRendererTestReadbackBuffer::ReadbackBuffer(ezUInt32 uiInvocationCount)
{
  if (m_iFrame == 1)
  {
    BeginCommands("Readback Buffer");
    // Queue readback
    {
      m_bReadbackInProgress = true;
      m_Readback.ReadbackBuffer(*m_pEncoder, m_hBufferReadback);
    }

    // Wait for results
    {
      ezEnum<ezGALAsyncResult> res = m_Readback.GetReadbackResult(ezTime::MakeFromHours(1));
      EZ_ASSERT_ALWAYS(res == ezGALAsyncResult::Ready, "Readback of texture failed");
    }

    // Readback result
    {
      m_bReadbackInProgress = false;
      {
        ezArrayPtr<const ezUInt8> memory;
        ezReadbackBufferLock lock = m_Readback.LockBuffer(memory);
        EZ_ASSERT_ALWAYS(lock, "Failed to lock readback buffer");

        if (EZ_TEST_INT(memory.GetCount(), m_BufferData.GetCount()))
        {
          EZ_TEST_INT(ezMemoryUtils::Compare(memory.GetPtr(), m_BufferData.GetData(), memory.GetCount()), 0);
        }
      }
    }
    EndCommands();
  }

  return m_bReadbackInProgress ? ezTestAppRun::Continue : ezTestAppRun::Quit;
}

static ezRendererTestReadbackBuffer g_ReadbackBufferTest;
