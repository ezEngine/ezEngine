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
  const ezVec2I32& GetOffset() const { return m_vOffset; } // [ property ]

  void SetSize(const ezVec2U32& size);                // [ property ]
  const ezVec2U32& GetSize() const { return m_vSize; } // [ property ]

  void SetAnchorPoint(const ezVec2& anchorPoint);                // [ property ]
  const ezVec2& GetAnchorPoint() const { return m_vAnchorPoint; } // [ property ]

  void SetPassInput(bool bPassInput);                // [ property ]
  bool GetPassInput() const { return m_bPassInput; } // [ property ]

  /// \brief Look for a blackboard component on the owner object and its parent and bind their blackboards during initialization of this component.
  void SetAutobindBlackboards(bool bAutobind);                           // [ property ]
  bool GetAutobindBlackboards() const { return m_bAutobindBlackboards; } // [ property ]

  ezUInt32 AddDataBinding(ezUniquePtr<ezRmlUiDataBinding>&& dataBinding);
  void RemoveDataBinding(ezUInt32 uiDataBindingIndex);

  /// \brief Adds the given blackboard as data binding. The name of the board is used as model name for the binding.
  ezUInt32 AddBlackboardBinding(const ezSharedPtr<ezBlackboard>& pBlackboard);
  void RemoveBlackboardBinding(ezUInt32 uiDataBindingIndex);

  ezRmlUiContext* GetOrCreateRmlContext();
  ezRmlUiContext* GetRmlContext() { return m_pContext; }

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;

protected:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;
  void OnMsgReload(ezMsgRmlUiReload& msg);
  void UpdateCachedValues();
  void UpdateAutobinding();

  ezRmlUiResourceHandle m_hResource;
  ezEvent<const ezResourceEvent&, ezMutex>::Unsubscriber m_ResourceEventUnsubscriber;

  ezVec2I32 m_vOffset = ezVec2I32::ZeroVector();
  ezVec2U32 m_vSize = ezVec2U32::ZeroVector();
  ezVec2 m_vAnchorPoint = ezVec2::ZeroVector();
  ezVec2U32 m_vReferenceResolution = ezVec2U32::ZeroVector();
  bool m_bPassInput = true;
  bool m_bAutobindBlackboards = true;

  ezRmlUiContext* m_pContext = nullptr;

  ezDynamicArray<ezUniquePtr<ezRmlUiDataBinding>> m_DataBindings;
  ezDynamicArray<ezUInt32> m_AutoBindings;
};
