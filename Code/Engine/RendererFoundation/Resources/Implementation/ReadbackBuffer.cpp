#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/ReadbackBuffer.h>

ezGALReadbackBuffer::ezGALReadbackBuffer(const ezGALBufferCreationDescription& Description)
  : ezGALResource(Description)
{
}

ezGALReadbackBuffer::~ezGALReadbackBuffer() = default;
