
void ezGALContextState::Invalidate()
{

  m_hShader = ezGALShaderHandle();

  m_hBlendState = ezGALBlendStateHandle();

  m_hDepthStencilState = ezGALDepthStencilStateHandle();

  m_hRasterizerState = ezGALRasterizerStateHandle();

  m_hRenderTargetConfig = ezGALRenderTargetConfigHandle();

  ezGALBufferHandle m_hConstantBuffers[EZ_GAL_MAX_CONSTANT_BUFFER_COUNT];

  ezGALResourceViewHandle m_hResourceViews[ezGALShaderStage::ENUM_COUNT][EZ_GAL_MAX_SHADER_RESOURCE_VIEW_COUNT];

  ezGALSamplerStateHandle m_hSamplerStates[ezGALShaderStage::ENUM_COUNT][EZ_GAL_MAX_SHADER_RESOURCE_VIEW_COUNT];

  ezGALBufferHandle m_hVertexBuffers[EZ_GAL_MAX_VERTEX_BUFFER_COUNT];

  m_hIndexBuffer = ezGALBufferHandle();

  m_hVertexDeclaration = ezGALVertexDeclarationHandle();

  m_ScissorRect = ezRectU32(UINT_MAX, UINT_MAX, 0, 0);

  m_ViewPortRect = ezRectFloat(FLT_MAX, FLT_MAX, 0.0f, 0.0f);

  m_fViewPortMinDepth = FLT_MAX;

  m_fViewPortMaxDepth = -FLT_MAX;

  m_Topology = ezGALPrimitiveTopology::ENUM_COUNT;
}
