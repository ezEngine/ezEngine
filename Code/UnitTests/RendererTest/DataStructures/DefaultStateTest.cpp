#include <RendererTest/RendererTestPCH.h>

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Descriptors/Enumerations.h>
#include <RendererTest/TestClass/SimpleRendererTest.h>

EZ_CREATE_SIMPLE_RENDERER_TEST(DataStructures, TextureDefaultState)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Immutable SRV texture defaults to ShaderResource")
  {
    ezGALTextureCreationDescription desc;
    desc.m_uiWidth = 4;
    desc.m_uiHeight = 4;
    desc.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
    desc.m_TextureFlags = ezGALTextureUsageFlags::ShaderResource;
    desc.m_ResourceAccess.m_bImmutable = true;
    EZ_TEST_BOOL(desc.GetDefaultState() == ezGALResourceState::ShaderResource);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Immutable depth texture defaults to DepthStencilRead")
  {
    ezGALTextureCreationDescription desc;
    desc.m_uiWidth = 4;
    desc.m_uiHeight = 4;
    desc.m_Format = ezGALResourceFormat::D16;
    desc.m_TextureFlags = ezGALTextureUsageFlags::ShaderResource;
    desc.m_ResourceAccess.m_bImmutable = true;
    EZ_TEST_BOOL(desc.GetDefaultState() == ezGALResourceState::DepthStencilRead);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Color render target with SRV defaults to ShaderResource")
  {
    ezGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(4, 4, ezGALResourceFormat::RGBAUByteNormalized);
    EZ_TEST_BOOL(desc.GetDefaultState() == ezGALResourceState::ShaderResource);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Depth render target with SRV defaults to DepthStencilRead")
  {
    ezGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(4, 4, ezGALResourceFormat::D16);
    EZ_TEST_BOOL(desc.GetDefaultState() == ezGALResourceState::DepthStencilRead);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Presentable texture defaults to Present")
  {
    ezGALTextureCreationDescription desc;
    desc.m_uiWidth = 4;
    desc.m_uiHeight = 4;
    desc.m_Format = ezGALResourceFormat::BGRAUByteNormalized;
    desc.m_TextureFlags = ezGALTextureUsageFlags::RenderTarget | ezGALTextureUsageFlags::Presentable;
    EZ_TEST_BOOL(desc.GetDefaultState() == ezGALResourceState::Present);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "UAV-only texture defaults to UnorderedAccess")
  {
    ezGALTextureCreationDescription desc;
    desc.m_uiWidth = 4;
    desc.m_uiHeight = 4;
    desc.m_Format = ezGALResourceFormat::RGBAFloat;
    desc.m_TextureFlags = ezGALTextureUsageFlags::UnorderedAccess;
    EZ_TEST_BOOL(desc.GetDefaultState() == ezGALResourceState::UnorderedAccess);
  }
}

EZ_CREATE_SIMPLE_RENDERER_TEST(DataStructures, BufferDefaultState)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Buffer with UAV defaults to UnorderedAccess")
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = 4;
    desc.m_uiTotalSize = 64;
    desc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource | ezGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceAccess.m_bImmutable = false;
    EZ_TEST_BOOL(desc.GetDefaultState() == ezGALResourceState::UnorderedAccess);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Buffer with VB+IB defaults to combined read states")
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = 4;
    desc.m_uiTotalSize = 64;
    desc.m_BufferFlags = ezGALBufferUsageFlags::VertexBuffer | ezGALBufferUsageFlags::IndexBuffer;
    desc.m_ResourceAccess.m_bImmutable = true;
    auto defaultState = desc.GetDefaultState();
    EZ_TEST_BOOL(defaultState.IsSet(ezGALResourceState::VertexBuffer));
    EZ_TEST_BOOL(defaultState.IsSet(ezGALResourceState::IndexBuffer));
    EZ_TEST_BOOL(!defaultState.IsAnySet(ezGALResourceState::AllWriteStates));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Buffer with SRV-only defaults to ShaderResource")
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = 4;
    desc.m_uiTotalSize = 64;
    desc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource;
    desc.m_ResourceAccess.m_bImmutable = true;
    EZ_TEST_BOOL(desc.GetDefaultState() == ezGALResourceState::ShaderResource);
  }
}
