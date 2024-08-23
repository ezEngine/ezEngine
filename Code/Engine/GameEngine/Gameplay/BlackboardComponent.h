#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/Utils/Blackboard.h>
#include <GameEngine/GameEngineDLL.h>

struct ezMsgUpdateLocalBounds;
struct ezMsgExtractRenderData;

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

/// \brief This base component represents an ezBlackboard, which can be used to share state between multiple components and objects.
///
/// The derived implementations may either create their own blackboards or reference other blackboards.
class EZ_GAMEENGINE_DLL ezBlackboardComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezBlackboardComponent, ezComponent);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezBlackboardComponent

public:
  ezBlackboardComponent();
  ~ezBlackboardComponent();

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

  void SetShowDebugInfo(bool bShow);                              // [ property ]
  bool GetShowDebugInfo() const;                                  // [ property ]

  void SetEntryValue(const char* szName, const ezVariant& value); // [ scriptable ]
  ezVariant GetEntryValue(const char* szName) const;              // [ scriptable ]

protected:
  static ezBlackboard* Reflection_FindBlackboard(ezGameObject* pSearchObject, ezStringView sBlackboardName);

  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const;
  void OnExtractRenderData(ezMsgExtractRenderData& msg) const;

  ezSharedPtr<ezBlackboard> m_pBoard;

  ezBlackboardTemplateResourceHandle m_hTemplate;
};

//////////////////////////////////////////////////////////////////////////

using ezLocalBlackboardComponentManager = ezComponentManager<class ezLocalBlackboardComponent, ezBlockStorageType::Compact>;

/// \brief This component creates its own ezBlackboard, and thus locally holds state.
class EZ_GAMEENGINE_DLL ezLocalBlackboardComponent : public ezBlackboardComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezLocalBlackboardComponent, ezBlackboardComponent, ezLocalBlackboardComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void Initialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezBlackboardComponent

public:
  ezLocalBlackboardComponent();
  ezLocalBlackboardComponent(ezLocalBlackboardComponent&& other);
  ~ezLocalBlackboardComponent();

  ezLocalBlackboardComponent& operator=(ezLocalBlackboardComponent&& other);

  void SetSendEntryChangedMessage(bool bSend); // [ property ]
  bool GetSendEntryChangedMessage() const;     // [ property ]

  void SetBlackboardName(const char* szName);  // [ property ]
  const char* GetBlackboardName() const;       // [ property ]

private:
  ezUInt32 Entries_GetCount() const;
  const ezBlackboardEntry& Entries_GetValue(ezUInt32 uiIndex) const;
  void Entries_SetValue(ezUInt32 uiIndex, const ezBlackboardEntry& entry);
  void Entries_Insert(ezUInt32 uiIndex, const ezBlackboardEntry& entry);
  void Entries_Remove(ezUInt32 uiIndex);

  void OnEntryChanged(const ezBlackboard::EntryEvent& e);
  void InitializeFromTemplate();

  // this array is not held during runtime, it is only needed during editor time until the component is serialized out
  ezDynamicArray<ezBlackboardEntry> m_InitialEntries;

  ezEventMessageSender<ezMsgBlackboardEntryChanged> m_EntryChangedSender; // [ event ]
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

/// \brief This component references a global blackboard by name. If necessary, the blackboard will be created.
///
/// This allows to initialize a global blackboard with known values.
class EZ_GAMEENGINE_DLL ezGlobalBlackboardComponent : public ezBlackboardComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezGlobalBlackboardComponent, ezBlackboardComponent, ezGlobalBlackboardComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void Initialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezGlobalBlackboardComponent

public:
  ezGlobalBlackboardComponent();
  ezGlobalBlackboardComponent(ezGlobalBlackboardComponent&& other);
  ~ezGlobalBlackboardComponent();

  ezGlobalBlackboardComponent& operator=(ezGlobalBlackboardComponent&& other);

  void SetBlackboardName(const char* szName);    // [ property ]
  const char* GetBlackboardName() const;         // [ property ]

  ezEnum<ezGlobalBlackboardInitMode> m_InitMode; // [ property ]

private:
  void InitializeFromTemplate();

  ezHashedString m_sName;
};
