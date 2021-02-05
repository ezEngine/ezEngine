#pragma once

#include <Core/World/World.h>
#include <Core/Utils/Blackboard.h>
#include <GameEngine/GameEngineDLL.h>

struct ezBlackboardEntry
{
  ezHashedString m_sName;
  ezVariant m_InitialValue;
  ezBitflags<ezBlackboardEntryFlags> m_Flags;
  ezEnum<ezVariantType> m_Type;

  void SetName(const char* szName) { m_sName.Assign(szName); }
  const char* GetName() const { return m_sName; }

  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezBlackboardEntry);

//////////////////////////////////////////////////////////////////////////

struct ezMsgUpdateLocalBounds;
struct ezMsgExtractRenderData;

using ezBlackboardComponentManager = ezComponentManager<class ezBlackboardComponent, ezBlockStorageType::Compact>;

class EZ_GAMEENGINE_DLL ezBlackboardComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezBlackboardComponent, ezComponent, ezBlackboardComponentManager);

public:
  ezBlackboardComponent();
  ezBlackboardComponent(ezBlackboardComponent&& other);
  ~ezBlackboardComponent();

  ezBlackboardComponent& operator=(ezBlackboardComponent&& other);

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const;
  void OnExtractRenderData(ezMsgExtractRenderData& msg) const;

  ezBlackboard& GetBoard();
  const ezBlackboard& GetBoard() const;

  void SetShowDebugInfo(bool bShow); // [ property ]
  bool GetShowDebugInfo() const;     // [ property ]

private:
  ezUInt32 Entries_GetCount() const;
  const ezBlackboardEntry& Entries_GetValue(ezUInt32 uiIndex) const;
  void Entries_SetValue(ezUInt32 uiIndex, const ezBlackboardEntry& entry);
  void Entries_Insert(ezUInt32 uiIndex, const ezBlackboardEntry& entry);
  void Entries_Remove(ezUInt32 uiIndex);

  void RegisterEntries(ezArrayPtr<ezBlackboardEntry> entries);

  ezUniquePtr<ezBlackboard> m_pBoard;

  ezDynamicArray<ezBlackboardEntry> m_InitialEntries;
};
