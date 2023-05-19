#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Shader/VertexDeclaration.h>

ezGALVertexDeclaration::ezGALVertexDeclaration(const ezGALVertexDeclarationCreationDescription& Description)
  : ezGALObject(Description)
{
}

ezGALVertexDeclaration::~ezGALVertexDeclaration() = default;



EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Shader_Implementation_VertexDeclaration);
