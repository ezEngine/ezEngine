#pragma once

#include <Core/World/World.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Textures/Texture2DResource.h>

struct ezMsgExtractRenderData;

using ezRenderTargetComponentManager = ezComponentManager<class ezRenderTargetActivatorComponent, ezBlockStorageType::Compact>;

/// \brief Attach this component to an object that uses a render target for reading, to ensure that the render target gets written to.
///
/// If you build a monitor that displays the output of a security camera in your level, the engine needs to know when it should
/// update the render target that displays the security camera footage, and when it can skip that part to not waste performance.
/// Thus, by default, the engine will not update the render target, as long as this isn't requested.
/// This component implements this request functionality.
///
/// It is a render component, which means that it tracks when it is visible and when visible, it will 'activate' the desired
/// render target, so that it will be updated.
/// By attaching it to an object, like the monitor, it activates the render target whenever the monitor object itself gets rendered.
class EZ_RENDERERCORE_DLL ezRenderTargetActivatorComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRenderTargetActivatorComponent, ezRenderComponent, ezRenderTargetComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent
public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) override;


  //////////////////////////////////////////////////////////////////////////
  // ezRenderTargetActivatorComponent

public:
  ezRenderTargetActivatorComponent();
  ~ezRenderTargetActivatorComponent();

  /// \brief Sets the ezRenderToTexture2DResource to render activate.
  void SetRenderTarget(const ezRenderToTexture2DResourceHandle& hResource);                    // [ property ]
  const ezRenderToTexture2DResourceHandle& GetRenderTarget() const { return m_hRenderTarget; } // [ property ]

private:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  ezRenderToTexture2DResourceHandle m_hRenderTarget;
};
