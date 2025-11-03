#pragma once

#include <RmlUiPlugin/Components/RmlUiMessages.h>
#include <RmlUiPlugin/Resources/RmlUiResource.h>
#include <RmlUiPlugin/RmlUiInput.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct ezMsgExtractRenderData;
class ezRmlUiContext;
class ezRmlUiDataBinding;
class ezBlackboard;

using ezRmlUiResourceHandle = ezTypedResourceHandle<class ezRmlUiResource>;

class EZ_RMLUIPLUGIN_DLL ezRmlUiCanvasComponentBase : public ezRenderComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezRmlUiCanvasComponentBase, ezRenderComponent);

public:
  ezRmlUiCanvasComponentBase();
  ~ezRmlUiCanvasComponentBase();

  ezRmlUiCanvasComponentBase& operator=(ezRmlUiCanvasComponentBase&& rhs);

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  virtual void Update();

  virtual bool ReceiveInput(const ezVec2& vMousePosInsideCanvas, ezRmlUiInputSnapshot input);

  void SetRmlResource(const ezRmlUiResourceHandle& hResource);                     // [ property ]
  const ezRmlUiResourceHandle& GetRmlResource() const { return m_hResource; }      // [ property ]

  /// \brief Look for a blackboard component on the owner object and its parent and bind their blackboards during initialization of this component.
  void SetAutobindBlackboards(bool bAutobind);                                     // [ property ]
  bool GetAutobindBlackboards() const { return m_bAutobindBlackboards; }           // [ property ]

  void SetOnDemandUpdate(bool bOnDemandUpdate);                                    // [ property ]
  bool GetOnDemandUpdate() const { return m_bOnDemandUpdate; }                     // [ property ]

  ezUInt32 AddDataBinding(ezUniquePtr<ezRmlUiDataBinding>&& pDataBinding);
  void RemoveDataBinding(ezUInt32 uiDataBindingIndex);

  /// \brief Adds the given blackboard as data binding. The name of the board is used as model name for the binding.
  ezUInt32 AddBlackboardBinding(const ezSharedPtr<ezBlackboard>& pBlackboard);
  void RemoveBlackboardBinding(ezUInt32 uiDataBindingIndex);

  ezRmlUiContext* GetOrCreateRmlContext();
  ezRmlUiContext* GetRmlContext() { return m_pContext; }

  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) override;

protected:
  void OnMsgReload(ezMsgRmlUiReload& msg);                                         // [ msg handler ]
  virtual void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const = 0;      // [ msg handler ]

  void UpdateCachedValues();
  void UpdateAutobinding();

  ezRmlUiResourceHandle m_hResource;
  ezEvent<const ezResourceEvent&, ezMutex>::Unsubscriber m_ResourceEventUnsubscriber;

  ezVec2U32 m_vSize = ezVec2U32::MakeZero();
  ezVec2U32 m_vReferenceResolution = ezVec2U32::MakeZero();
  bool m_bAutobindBlackboards = true;
  bool m_bOnDemandUpdate = true;
  bool m_bNeedsUpdate = false;

  ezRmlUiContext* m_pContext = nullptr;
  ezRmlUiInputProvider m_InputProvider;

  ezDynamicArray<ezUniquePtr<ezRmlUiDataBinding>> m_DataBindings;
  ezDynamicArray<ezUInt32> m_AutoBindings;
};