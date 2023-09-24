#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/Utils/Blackboard.h>
#include <GameEngine/GameEngineDLL.h>

using ezBlackboardTemplateResourceHandle = ezTypedResourceHandle<class ezBlackboardTemplateResource>;

struct ezBlackboardEntry
{
  ezHashedString m_sName;
  ezVariant m_InitialValue;
  ezBitflags<ezBlackboardEntryFlags> m_Flags;

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezBlackboardEntry);

//////////////////////////////////////////////////////////////////////////

struct EZ_GAMEENGINE_DLL ezMsgBlackboardEntryChanged : public ezEventMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgBlackboardEntryChanged, ezEventMessage);

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
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezBlackboardComponent

public:
  ezBlackboardComponent();
  ezBlackboardComponent(ezBlackboardComponent&& other);
  ~ezBlackboardComponent();

  ezBlackboardComponent& operator=(ezBlackboardComponent&& other);

  /// \brief Try to find a ezBlackboardComponent on pSearchObject or its parents with the given name and returns its blackboard.
  ///
  /// The blackboard name is only checked if the given name is not empty. If no matching blackboard component is found,
  /// the function will call ezBlackboard::GetOrCreateGlobal() with the given name. Thus if you provide a name, you will always get a result, either from a component or from the global storage.
  ///
  /// \sa ezBlackboard::GetOrCreateGlobal()
  static ezSharedPtr<ezBlackboard> FindBlackboard(ezGameObject* pSearchObject, ezStringView sBlackboardName = ezStringView());

  /// \brief Returns the blackboard owned by this component
  const ezSharedPtr<ezBlackboard>& GetBoard();
  ezSharedPtr<const ezBlackboard> GetBoard() const;

  void SetShowDebugInfo(bool bShow); // [ property ]
  bool GetShowDebugInfo() const;     // [ property ]

  void SetSendEntryChangedMessage(bool bSend); // [ property ]
  bool GetSendEntryChangedMessage() const;     // [ property ]

  void SetBlackboardName(const char* szName); // [ property ]
  const char* GetBlackboardName() const;      // [ property ]

  void SetEntryValue(const char* szName, const ezVariant& value); // [ scriptable ]
  ezVariant GetEntryValue(const char* szName) const;              // [ scriptable ]

  void SetTemplateFile(const char* szName); // [ property ]
  const char* GetTemplateFile() const;      // [ property ]

private:
  ezUInt32 Entries_GetCount() const;
  const ezBlackboardEntry& Entries_GetValue(ezUInt32 uiIndex) const;
  void Entries_SetValue(ezUInt32 uiIndex, const ezBlackboardEntry& entry);
  void Entries_Insert(ezUInt32 uiIndex, const ezBlackboardEntry& entry);
  void Entries_Remove(ezUInt32 uiIndex);

  static ezBlackboard* Reflection_FindBlackboard(ezGameObject* pSearchObject, ezStringView sBlackboardName);

  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const;
  void OnExtractRenderData(ezMsgExtractRenderData& msg) const;
  void OnEntryChanged(const ezBlackboard::EntryEvent& e);
  void InitializeFromTemplate();

  ezSharedPtr<ezBlackboard> m_pBoard;

  // this array is not held during runtime, it is only needed during editor time until the component is serialized out
  ezDynamicArray<ezBlackboardEntry> m_InitialEntries;

  ezEventMessageSender<ezMsgBlackboardEntryChanged> m_EntryChangedSender; // [ event ]

  ezBlackboardTemplateResourceHandle m_hTemplate;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct ezGlobalBlackboardInitMode
{
  using StorageType = ezUInt8;

  enum Enum : StorageType
  {
    EnsureEntriesExist,    ///< Brief only adds entries to the blackboard, that haven't been added before. Doesn't change the values of existing entries.
    ResetEntryValues,      ///< Overwrites values of existing entries, to reset them to the start value defined in the template.
    ClearEntireBlackboard, ///< Removes all entries from the blackboard and only adds the ones from the template. This also gets rid of temporary values.

    Default = ClearEntireBlackboard
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezGlobalBlackboardInitMode);

using ezGlobalBlackboardComponentManager = ezComponentManager<class ezGlobalBlackboardComponent, ezBlockStorageType::Compact>;

class EZ_GAMEENGINE_DLL ezGlobalBlackboardComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezGlobalBlackboardComponent, ezComponent, ezGlobalBlackboardComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezGlobalBlackboardComponent

public:
  ezGlobalBlackboardComponent();
  ezGlobalBlackboardComponent(ezGlobalBlackboardComponent&& other);
  ~ezGlobalBlackboardComponent();

  ezGlobalBlackboardComponent& operator=(ezGlobalBlackboardComponent&& other);

  /// \brief Returns the global blackboard referenced by this component
  const ezSharedPtr<ezBlackboard>& GetBoard();

  /// \brief Returns the global blackboard referenced by this component
  ezSharedPtr<const ezBlackboard> GetBoard() const;

  void SetBlackboardName(const char* szName); // [ property ]
  const char* GetBlackboardName() const;      // [ property ]

  void SetTemplateFile(const char* szName); // [ property ]
  const char* GetTemplateFile() const;      // [ property ]

  ezEnum<ezGlobalBlackboardInitMode> m_InitMode; // [ property ]

private:
  void InitializeFromTemplate();

  bool m_bClearBlackboard = false;
  bool m_bOverwriteValues = false;
  ezHashedString m_sName;
  ezSharedPtr<ezBlackboard> m_pBoard;
  ezBlackboardTemplateResourceHandle m_hTemplate;
};
