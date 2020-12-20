
#include <RendererDX11PCH.h>

#include <RendererDX11/CommandEncoder/ComputeCommandEncoderDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>

ezGALComputeCommandEncoderDX11::ezGALComputeCommandEncoderDX11(ezGALDevice& device)
  : SUPER(device)
{
}

ezGALComputeCommandEncoderDX11::~ezGALComputeCommandEncoderDX11() = default;

void ezGALComputeCommandEncoderDX11::DispatchPlatform(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ)
{
  FlushDeferredStateChanges();

  m_pDXContext->Dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
}

void ezGALComputeCommandEncoderDX11::DispatchIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  m_pDXContext->DispatchIndirect(static_cast<const ezGALBufferDX11*>(pIndirectArgumentBuffer)->GetDXBuffer(), uiArgumentOffsetInBytes);
}
