#include <RendererDX11PCH.h>

#include <RendererDX11/CommandEncoder/CommandEncoderImplDX11.h>
#include <RendererDX11/Device/PassDX11.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>
#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>

ezGALPassDX11::ezGALPassDX11(ezGALDevice& device)
  : ezGALPass(device)
{
  m_pCommandEncoderState = EZ_DEFAULT_NEW(ezGALCommandEncoderRenderState);
  m_pCommandEncoderImpl = EZ_DEFAULT_NEW(ezGALCommandEncoderImplDX11, static_cast<ezGALDeviceDX11&>(device));

  m_pRenderCommandEncoder = EZ_DEFAULT_NEW(ezGALRenderCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);
  m_pComputeCommandEncoder = EZ_DEFAULT_NEW(ezGALComputeCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);

  m_pCommandEncoderImpl->m_pOwner = m_pRenderCommandEncoder.Borrow();
}

ezGALPassDX11::~ezGALPassDX11() = default;

ezGALRenderCommandEncoder* ezGALPassDX11::BeginRenderingPlatform(const ezGALRenderingSetup& renderingSetup, const char* szName)
{
  m_pCommandEncoderImpl->BeginRender(renderingSetup);

  return m_pRenderCommandEncoder.Borrow();
}

void ezGALPassDX11::EndRenderingPlatform(ezGALRenderCommandEncoder* pCommandEncoder)
{
  EZ_ASSERT_DEV(m_pRenderCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder");
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
  m_pCommandEncoderImpl->PushMarkerPlatform(szName);
}

void ezGALPassDX11::EndPass()
{
  m_pCommandEncoderImpl->PopMarkerPlatform();
}
