
void ezGALContextState::Invalidate()
{

  m_hShader = ezGALShaderHandle();

  m_hBlendState = ezGALBlendStateHandle();

  m_BlendFactor = ezColor(0, 0, 0);

  m_uiSampleMask = 0x0;

  m_hDepthStencilState = ezGALDepthStencilStateHandle();

  m_uiStencilRefValue = 0;

  m_hRasterizerState = ezGALRasterizerStateHandle();

  m_RenderTargetSetup = ezGALRenderTagetSetup();

  ezGALBufferHandle m_hConstantBuffers[EZ_GAL_MAX_CONSTANT_BUFFER_COUNT];

  ezGALResourceViewHandle m_hResourceViews[ezGALShaderStage::ENUM_COUNT][EZ_GAL_MAX_SHADER_RESOURCE_VIEW_COUNT];

  ezGALSamplerStateHandle m_hSamplerStates[ezGALShaderStage::ENUM_COUNT][EZ_GAL_MAX_SHADER_RESOURCE_VIEW_COUNT];

  ezGALBufferHandle m_hVertexBuffers[EZ_GAL_MAX_VERTEX_BUFFER_COUNT];

  m_hIndexBuffer = ezGALBufferHandle();

  m_hVertexDeclaration = ezGALVertexDeclarationHandle();

  m_ScissorRect = ezRectU32(0xFFFFFFFF, 0xFFFFFFFF, 0, 0);

  m_ViewPortRect = ezRectFloat(ezMath::BasicType<float>::MaxValue(), ezMath::BasicType<float>::MaxValue(), 0.0f, 0.0f);

  m_fViewPortMinDepth = ezMath::BasicType<float>::MaxValue();

  m_fViewPortMaxDepth = -ezMath::BasicType<float>::MaxValue();

  m_Topology = ezGALPrimitiveTopology::ENUM_COUNT;
}
