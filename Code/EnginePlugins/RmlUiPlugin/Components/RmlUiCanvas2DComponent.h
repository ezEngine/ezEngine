#pragma once

#include <RendererCore/Components/RenderComponent.h>

#include <RmlUiPluginDLL.h>

struct ezMsgExtractRenderData;
class ezRmlUiContext;

using ezRmlUiCanvas2DComponentManager = ezComponentManagerSimple<class ezRmlUiCanvas2DComponent, ezComponentUpdateType::Always, ezBlockStorageType::Compact>;

class EZ_RMLUIPLUGIN_DLL ezRmlUiCanvas2DComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRmlUiCanvas2DComponent, ezRenderComponent, ezRmlUiCanvas2DComponentManager);

public:
  ezRmlUiCanvas2DComponent();
  ~ezRmlUiCanvas2DComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void Update();

  void SetRmlFile(const char* szFile); // [ property ]
  const char* GetRmlFile() const;      // [ property ]

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;

protected:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  ezString m_sRmlFile;
  ezRmlUiContext* m_pContext = nullptr;
};
