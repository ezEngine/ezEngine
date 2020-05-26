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

  void SetRmlFile(const char* szFile);                  // [ property ]
  const char* GetRmlFile() const { return m_sRmlFile; } // [ property ]

  void SetOffset(const ezVec2I32& offset);                // [ property ]
  const ezVec2I32& GetOffset() const { return m_Offset; } // [ property ]

  void SetSize(const ezVec2U32& size);                // [ property ]
  const ezVec2U32& GetSize() const { return m_Size; } // [ property ]

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;

protected:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  ezString m_sRmlFile;
  ezVec2I32 m_Offset = ezVec2I32::ZeroVector();
  ezVec2U32 m_Size = ezVec2U32(100);

  ezRmlUiContext* m_pContext = nullptr;
};
