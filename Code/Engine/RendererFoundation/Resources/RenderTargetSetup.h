
#pragma once

#include <Foundation/Basics.h>
#include <RendererFoundation/Basics.h>

// \brief This class can be used to construct render target setups on the stack.
class EZ_RENDERERFOUNDATION_DLL ezGALRenderTagetSetup
{
public:

  ezGALRenderTagetSetup();

  ezGALRenderTagetSetup& SetRenderTarget(ezUInt8 uiIndex, ezGALRenderTargetViewHandle hRenderTarget);
  ezGALRenderTagetSetup& SetDepthStencilTarget(ezGALRenderTargetViewHandle hDSTarget);

  bool operator == (const ezGALRenderTagetSetup& Other) const;

  inline bool HasRenderTargets() const;

  inline ezUInt8 GetMaxRenderTargetIndex() const;

  inline ezGALRenderTargetViewHandle GetRenderTarget(ezUInt8 uiIndex) const;
  inline ezGALRenderTargetViewHandle GetDepthStencilTarget() const;

  

protected:

  ezGALRenderTargetViewHandle m_hRTs[EZ_GAL_MAX_RENDERTARGET_COUNT];
  ezGALRenderTargetViewHandle m_hDSTarget;

  ezUInt8 m_uiMaxRTIndex;
};

#include <RendererFoundation/Resources/Implementation/RenderTargetSetup_inl.h>