#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RmlUiPlugin/Components/RmlUiMessages.h>

struct ezMsgExtractRenderData;
class ezRmlUiContext;
class ezRmlUiDataBinding;
class ezBlackboard;

using ezRmlUiResourceHandle = ezTypedResourceHandle<class ezRmlUiResource>;

using ezRmlUiCanvas2DComponentManager = ezComponentManagerSimple<class ezRmlUiCanvas2DComponent, ezComponentUpdateType::Always, ezBlockStorageType::Compact>;

class EZ_RMLUIPLUGIN_DLL ezRmlUiCanvas2DComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRmlUiCanvas2DComponent, ezRenderComponent, ezRmlUiCanvas2DComponentManager);

public:
  ezRmlUiCanvas2DComponent();
  ~ezRmlUiCanvas2DComponent();

  ezRmlUiCanvas2DComponent& operator=(ezRmlUiCanvas2DComponent&& rhs);

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void Update();

  void SetRmlFile(const char* szFile); // [ property ]
  const char* GetRmlFile() const;      // [ property ]

  void SetRmlResource(const ezRmlUiResourceHandle& hResource);
  const ezRmlUiResourceHandle& GetRmlResource() const { return m_hResource; }

  void SetOffset(const ezVec2I32& offset);                // [ property ]
  const ezVec2I32& GetOffset() const { return m_Offset; } // [ property ]

  void SetSize(const ezVec2U32& size);                // [ property ]
  const ezVec2U32& GetSize() const { return m_Size; } // [ property ]

  void SetAnchorPoint(const ezVec2& anchorPoint);                // [ property ]
  const ezVec2& GetAnchorPoint() const { return m_AnchorPoint; } // [ property ]

  void SetPassInput(bool bPassInput);                // [ property ]
  bool GetPassInput() const { return m_bPassInput; } // [ property ]

  ezUInt32 AddDataBinding(ezUniquePtr<ezRmlUiDataBinding>&& dataBinding);
  void RemoveDataBinding(ezUInt32 uiDataBindingIndex);

  ezUInt32 AddBlackboardBinding(ezBlackboard& blackboard, const char* szModelName);
  void RemoveBlackboardBinding(ezUInt32 uiDataBindingIndex);

  ezRmlUiContext* GetRmlContext() { return m_pContext; }

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;

protected:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;
  void OnMsgReload(ezMsgRmlUiReload& msg);
  void UpdateCachedValues();

  ezRmlUiResourceHandle m_hResource;
  ezEvent<const ezResourceEvent&, ezMutex>::Unsubscriber m_ResourceEventUnsubscriber;

  ezVec2I32 m_Offset = ezVec2I32::ZeroVector();
  ezVec2U32 m_Size = ezVec2U32::ZeroVector();
  ezVec2 m_AnchorPoint = ezVec2::ZeroVector();
  ezVec2U32 m_ReferenceResolution = ezVec2U32::ZeroVector();
  bool m_bPassInput = true;

  ezRmlUiContext* m_pContext = nullptr;

  ezDynamicArray<ezUniquePtr<ezRmlUiDataBinding>> m_DataBindings;
};
