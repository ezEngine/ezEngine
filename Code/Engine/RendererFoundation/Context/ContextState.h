
#pragma once

#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Color.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

struct EZ_RENDERERFOUNDATION_DLL ezGALContextState
{
  ezGALShaderHandle m_hShader;

  ezGALBlendStateHandle m_hBlendState;

  ezColor m_BlendFactor;

  ezUInt32 m_uiSampleMask;

  ezGALDepthStencilStateHandle m_hDepthStencilState;

  ezUInt8 m_uiStencilRefValue;

  ezGALRasterizerStateHandle m_hRasterizerState;

  ezGALRenderTagetSetup m_RenderTargetSetup;

  ezGALBufferHandle m_hConstantBuffers[EZ_GAL_MAX_CONSTANT_BUFFER_COUNT];

  ezGALResourceViewHandle m_hResourceViews[ezGALShaderStage::ENUM_COUNT][EZ_GAL_MAX_SHADER_RESOURCE_VIEW_COUNT];

  ezGALSamplerStateHandle m_hSamplerStates[ezGALShaderStage::ENUM_COUNT][EZ_GAL_MAX_SHADER_RESOURCE_VIEW_COUNT];

  ezGALBufferHandle m_hVertexBuffers[EZ_GAL_MAX_VERTEX_BUFFER_COUNT];

  ezGALBufferHandle m_hIndexBuffer;

  ezGALVertexDeclarationHandle m_hVertexDeclaration;

  ezRectU32 m_ScissorRect;

  ezRectFloat m_ViewPortRect;

  float m_fViewPortMinDepth;

  float m_fViewPortMaxDepth;

  ezGALPrimitiveTopology::Enum m_Topology;

  inline void Invalidate();

};



#include <RendererFoundation/Context/Implementation/ContextState_inl.h>