#include <PCH.h>

#include <RendererFoundation/Resources/Buffer.h>

ezGALBuffer::ezGALBuffer(const ezGALBufferCreationDescription& Description)
    : ezGALResource(Description)
{
}

ezGALBuffer::~ezGALBuffer() {}



EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_Buffer);

