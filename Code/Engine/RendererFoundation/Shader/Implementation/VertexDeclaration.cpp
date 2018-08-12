#include <PCH.h>

#include <RendererFoundation/Shader/VertexDeclaration.h>

ezGALVertexDeclaration::ezGALVertexDeclaration(const ezGALVertexDeclarationCreationDescription& Description)
    : ezGALObject(Description)
{
}

ezGALVertexDeclaration::~ezGALVertexDeclaration() {}



EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Shader_Implementation_VertexDeclaration);
