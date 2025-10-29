#pragma once

#include <RmlUiPlugin/Components/RmlUiMessages.h>
#include <RmlUiPlugin/Resources/RmlUiResource.h>
#include <RmlUiPlugin/RmlUiInputSnapshot.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Components/RenderComponent.h>

struct ezMsgExtractGeometry;
struct ezMsgExtractRenderData;
class ezRmlUiContext;
class ezRmlUiDataBinding;
class ezBlackboard;

using ezRmlUiResourceHandle = ezTypedResourceHandle<class ezRmlUiResource>;

using ezRmlUiCanvas3DComponentManager = ezComponentManagerSimple<class ezRmlUiCanvas3DComponent, ezComponentUpdateType::Always, ezBlockStorageType::Compact, ezWorldUpdatePhase::PostTransform>;

class EZ_RMLUIPLUGIN_DLL ezRmlUiCanvas3DComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRmlUiCanvas3DComponent, ezRenderComponent, ezRmlUiCanvas3DComponentManager);

public:
  ezRmlUiCanvas3DComponent();
  ~ezRmlUiCanvas3DComponent();

  ezRmlUiCanvas3DComponent& operator=(ezRmlUiCanvas3DComponent&& rhs);

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void Update();
  void ReceiveInput(const ezVec2& vMousePos, ezRmlUiInputSnapshot input);

  void SetRmlResource(const ezRmlUiResourceHandle& hResource);                     // [ property ]
  const ezRmlUiResourceHandle& GetRmlResource() const { return m_hResource; }      // [ property ]

  void SetTextureSize(const ezVec2U32& vSize);                                     // [ property ]
  const ezVec2U32& GetTextureSize() const { return m_vTextureSize; }               // [ property ]

  /// \brief Look for a blackboard component on the owner object and its parent and bind their blackboards during initialization of this component.
  void SetAutobindBlackboards(bool bAutobind);                                     // [ property ]
  bool GetAutobindBlackboards() const { return m_bAutobindBlackboards; }           // [ property ]

  void SetOnDemandUpdate(bool bOnDemandUpdate);                                    // [ property ]
  bool GetOnDemandUpdate() const { return m_bOnDemandUpdate; }                     // [ property ]

  void SetClearStaleInput(bool bClearStaleInput);                                  // [ property ]
  bool GetClearStaleInput() const { return m_bClearStaleInput; }                   // [ property ]

  void SetInteractive(bool bIsInteractive);                                        // [ property ]
  bool IsInteractive() const { return m_bIsInteractive; }                          // [ property ]

  /// \brief Changes which mesh to render.
  void SetMesh(const ezMeshResourceHandle& hMesh);                                 // [ property ]
  EZ_ALWAYS_INLINE const ezMeshResourceHandle& GetMesh() const { return m_hMesh; } // [ property ]

  // adds SetMeshFile() and GetMeshFile() for convenience
  EZ_ADD_RESOURCEHANDLE_ACCESSORS_WITH_SETTER(Mesh, m_hMesh, SetMesh);

  ezUInt32 AddDataBinding(ezUniquePtr<ezRmlUiDataBinding>&& pDataBinding);
  void RemoveDataBinding(ezUInt32 uiDataBindingIndex);

  /// \brief Adds the given blackboard as data binding. The name of the board is used as model name for the binding.
  ezUInt32 AddBlackboardBinding(const ezSharedPtr<ezBlackboard>& pBlackboard);
  void RemoveBlackboardBinding(ezUInt32 uiDataBindingIndex);

  ezRmlUiContext* GetOrCreateRmlContext();
  ezRmlUiContext* GetRmlContext() { return m_pContext; }

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) override;

protected:
  void OnMsgExtractGeometry(ezMsgExtractGeometry& ref_msg) const;                  // [ msg handler ]
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;                  // [ msg handler ]
  void OnMsgReload(ezMsgRmlUiReload& msg);                                         // [ msg handler ]

  bool UpdateTexture();
  void UpdateCachedValues();
  void UpdateAutobinding();

  ezRmlUiResourceHandle m_hResource;
  ezEvent<const ezResourceEvent&, ezMutex>::Unsubscriber m_ResourceEventUnsubscriber;

  ezVec2U32 m_vTextureSize = ezVec2U32(512, 512);
  bool m_bAutobindBlackboards = true;
  bool m_bOnDemandUpdate = true;
  bool m_bClearStaleInput = true;
  bool m_bNeedsUpdate = false;
  bool m_bIsInteractive = true;
  ezInt8 m_iInputAge = -1;

  ezMeshResourceHandle m_hMesh;
  ezGALTextureHandle m_hTexture;

  ezRmlUiContext* m_pContext = nullptr;
  ezRmlUiInputProvider m_InputProvider;

  ezDynamicArray<ezUniquePtr<ezRmlUiDataBinding>> m_DataBindings;
  ezDynamicArray<ezUInt32> m_AutoBindings;
};
