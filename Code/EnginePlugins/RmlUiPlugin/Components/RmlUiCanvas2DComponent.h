#pragma once

#include <RendererCore/Components/RenderComponent.h>

#include <RmlUiPlugin/RmlUiPluginDLL.h>

struct ezMsgExtractRenderData;
class ezRmlUiContext;

using ezRmlUiCanvas2DComponentManager = ezComponentManagerSimple<class ezRmlUiCanvas2DComponent, ezComponentUpdateType::Always, ezBlockStorageType::Compact>;

class EZ_RMLUIPLUGIN_DLL ezRmlUiCanvas2DComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRmlUiCanvas2DComponent, ezRenderComponent, ezRmlUiCanvas2DComponentManager);

public:
  ezRmlUiCanvas2DComponent();
  ~ezRmlUiCanvas2DComponent();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void Update();

  void SetRmlFile(const char* szFile);                  // [ property ]
  const char* GetRmlFile() const { return m_sRmlFile; } // [ property ]

  void SetOffset(const ezVec2I32& offset);                // [ property ]
  const ezVec2I32& GetOffset() const { return m_Offset; } // [ property ]

  void SetSize(const ezVec2U32& size);                // [ property ]
  const ezVec2U32& GetSize() const { return m_Size; } // [ property ]

  void SetPassInput(bool bPassInput);                // [ property ]
  bool GetPassInput() const { return m_bPassInput; } // [ property ]

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;

protected:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  ezString m_sRmlFile;
  ezVec2I32 m_Offset = ezVec2I32::ZeroVector();
  ezVec2U32 m_Size = ezVec2U32(100);
  bool m_bPassInput = true;

  ezRmlUiContext* m_pContext = nullptr;
};
