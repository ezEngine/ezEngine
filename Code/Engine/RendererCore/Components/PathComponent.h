#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <RendererCore/RendererCoreDLL.h>

struct ezMsgTransformChanged;
struct ezMsgParentChanged;

struct EZ_RENDERERCORE_DLL ezPathNodeData
{
  ezResult Serialize(ezStreamWriter& writer) const;
  ezResult Deserialize(ezStreamReader& reader);

  ezVec3 m_vPosition;
  ezQuat m_qRotation;
  ezColor m_Color;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezPathNodeData);

//////////////////////////////////////////////////////////////////////////

struct EZ_RENDERERCORE_DLL ezEventMsgPathChanged : public ezEventMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezEventMsgPathChanged, ezEventMessage);
};

//////////////////////////////////////////////////////////////////////////

using ezPathComponentManager = ezComponentManagerSimple<class ezPathComponent, ezComponentUpdateType::Always>;

class EZ_RENDERERCORE_DLL ezPathComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPathComponent, ezComponent, ezPathComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezPathComponent

public:
  ezPathComponent();
  ~ezPathComponent();

  void Update();
  void UpdatePath();

  void OnEventMsgPathChanged(ezEventMsgPathChanged& msg);

protected:
  bool m_bPathChanged = true;
  bool m_bLooping = false;
  ezDynamicArray<ezPathNodeData> m_PathNodes;
};

//////////////////////////////////////////////////////////////////////////

using ezPathNodeComponentManager = ezComponentManager<class ezPathNodeComponent, ezBlockStorageType::Compact>;

struct EZ_RENDERERCORE_DLL ezPathNodeConnection
{
  ezString m_sTarget;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezPathNodeConnection);

class EZ_RENDERERCORE_DLL ezPathNodeComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPathNodeComponent, ezComponent, ezPathNodeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezPathNodeComponent

public:
  ezPathNodeComponent();
  ~ezPathNodeComponent();

  ezHybridArray<ezPathNodeConnection, 1> m_Connections;

protected:
  void OnMsgTransformChanged(ezMsgTransformChanged& msg);
  void OnMsgParentChanged(ezMsgParentChanged& msg);

  virtual void OnActivated() override;

  ezUInt32 Connections_GetCount() const;                                          // [ property ]
  const ezPathNodeConnection& Connections_GetValue(ezUInt32 uiIndex) const;       // [ property ]
  void Connections_SetValue(ezUInt32 uiIndex, const ezPathNodeConnection& value); // [ property ]
  void Connections_Insert(ezUInt32 uiIndex, const ezPathNodeConnection& value);   // [ property ]
  void Connections_Remove(ezUInt32 uiIndex);                                      // [ property ]

  void PathChanged();
};
