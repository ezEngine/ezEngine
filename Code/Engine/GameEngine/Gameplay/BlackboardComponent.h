#pragma once

#include <Core/Utils/Blackboard.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

struct ezBlackboardEntry
{
  ezHashedString m_sName;
  ezVariant m_InitialValue;
  ezBitflags<ezBlackboardEntryFlags> m_Flags;

  void SetName(const char* szName) { m_sName.Assign(szName); }
  const char* GetName() const { return m_sName; }

  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezBlackboardEntry);

//////////////////////////////////////////////////////////////////////////

struct EZ_GAMEENGINE_DLL ezMsgBlackboardEntryChanged : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgBlackboardEntryChanged, ezMessage);

  ezHashedString m_sName;
  ezVariant m_OldValue;
  ezVariant m_NewValue;

private:
  const char* GetName() const { return m_sName; }
  void SetName(const char* szName) { m_sName.Assign(szName); }
};

//////////////////////////////////////////////////////////////////////////

struct ezMsgUpdateLocalBounds;
struct ezMsgExtractRenderData;

using ezBlackboardComponentManager = ezComponentManager<class ezBlackboardComponent, ezBlockStorageType::Compact>;

/// \brief This component holds an ezBlackboard which can be used to share state between multiple components.
class EZ_GAMEENGINE_DLL ezBlackboardComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezBlackboardComponent, ezComponent, ezBlackboardComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezBlackboardComponent

public:
  ezBlackboardComponent();
  ezBlackboardComponent(ezBlackboardComponent&& other);
  ~ezBlackboardComponent();

  ezBlackboardComponent& operator=(ezBlackboardComponent&& other);

  /// \brief Try to find a ezBlackboardComponent on pSearchObject or its parents and returns its blackboard
  static ezBlackboard* FindBlackboard(ezGameObject* pSearchObject);

  /// \brief Returns the blackboard owned by this component
  ezBlackboard& GetBoard();
  const ezBlackboard& GetBoard() const;

  void SetShowDebugInfo(bool bShow); // [ property ]
  bool GetShowDebugInfo() const;     // [ property ]

  void SetSendEntryChangedMessage(bool bSend); // [ property ]
  bool GetSendEntryChangedMessage() const;     // [ property ]

  void SetBlackboardName(const char* szName); // [ property ]
  const char* GetBlackboardName() const;      // [ property ]

  void SetEntryValue(const char* szName, const ezVariant& value); // [ scriptable ]
  ezVariant GetEntryValue(const char* szName);                    // [ scriptable ]

private:
  ezUInt32 Entries_GetCount() const;
  const ezBlackboardEntry& Entries_GetValue(ezUInt32 uiIndex) const;
  void Entries_SetValue(ezUInt32 uiIndex, const ezBlackboardEntry& entry);
  void Entries_Insert(ezUInt32 uiIndex, const ezBlackboardEntry& entry);
  void Entries_Remove(ezUInt32 uiIndex);

  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const;
  void OnExtractRenderData(ezMsgExtractRenderData& msg) const;

  ezUniquePtr<ezBlackboard> m_pBoard;

  // this array is not held during runtime, it is only needed during editor time until the component is serialized out
  ezDynamicArray<ezBlackboardEntry> m_InitialEntries;
};
