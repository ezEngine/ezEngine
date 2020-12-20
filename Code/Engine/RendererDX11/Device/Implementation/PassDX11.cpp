#include <RendererDX11PCH.h>

#include <RendererDX11/CommandEncoder/ComputeCommandEncoderDX11.h>
#include <RendererDX11/CommandEncoder/RenderCommandEncoderDX11.h>
#include <RendererDX11/Device/PassDX11.h>

ezGALPassDX11::ezGALPassDX11(ezGALDevice& device)
  : ezGALPass(device)
{
  m_pRenderCommandEncoder = EZ_DEFAULT_NEW(ezGALRenderCommandEncoderDX11, device);
  m_pComputeCommandEncoder = EZ_DEFAULT_NEW(ezGALComputeCommandEncoderDX11, device);
}

ezGALPassDX11::~ezGALPassDX11() = default;

ezGALRenderCommandEncoder* ezGALPassDX11::BeginRenderingPlatform(const ezGALRenderingSetup& renderingSetup, const char* szName)
{
  m_pRenderCommandEncoder->BeginEncode(renderingSetup);

  return m_pRenderCommandEncoder.Borrow();
}

void ezGALPassDX11::EndRenderingPlatform(ezGALRenderCommandEncoder* pCommandEncoder)
{
  EZ_ASSERT_DEV(m_pRenderCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder");

  m_pRenderCommandEncoder->EndEncode();
}

ezGALComputeCommandEncoder* ezGALPassDX11::BeginComputePlatform(const char* szName)
{
  return m_pComputeCommandEncoder.Borrow();
}

void ezGALPassDX11::EndComputePlatform(ezGALComputeCommandEncoder* pCommandEncoder)
{
  EZ_ASSERT_DEV(m_pComputeCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder");
}

void ezGALPassDX11::BeginPass(const char* szName)
{
  m_pRenderCommandEncoder->PushMarkerPlatform(szName);
}

void ezGALPassDX11::EndPass()
{
  m_pRenderCommandEncoder->PopMarkerPlatform();
}
