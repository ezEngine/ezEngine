#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/Buffer.h>

ezGALBuffer::ezGALBuffer(const ezGALBufferCreationDescription& Description)
  : ezGALResource(Description)
{
}

ezGALBuffer::~ezGALBuffer()
{
  EZ_ASSERT_DEV(m_hDefaultTexelBufferView.IsInvalidated(), "");
  EZ_ASSERT_DEV(m_hDefaultStructuredBufferView.IsInvalidated(), "");
  EZ_ASSERT_DEV(m_hDefaultByteAddressBufferView.IsInvalidated(), "");
  EZ_ASSERT_DEV(m_ResourceViews.IsEmpty(), "Dangling resource views");
  EZ_ASSERT_DEV(m_UnorderedAccessViews.IsEmpty(), "Dangling unordered access views");
}
