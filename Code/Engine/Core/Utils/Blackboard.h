#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Variant.h>

class ezStreamReader;
class ezStreamWriter;

/// \brief Flags for entries in ezBlackboard.
struct EZ_CORE_DLL ezBlackboardEntryFlags
{
  using StorageType = ezUInt16;

  enum Enum
  {
    None = 0,
    Save = EZ_BIT(0),          ///< Include the entry during serialization
    OnChangeEvent = EZ_BIT(1), ///< Broadcast the 'ValueChanged' event when this entry's value is modified

    UserFlag0 = EZ_BIT(7),
    UserFlag1 = EZ_BIT(8),
    UserFlag2 = EZ_BIT(9),
    UserFlag3 = EZ_BIT(10),
    UserFlag4 = EZ_BIT(11),
    UserFlag5 = EZ_BIT(12),
    UserFlag6 = EZ_BIT(13),
    UserFlag7 = EZ_BIT(14),

    Invalid = EZ_BIT(15),

    Default = None
  };

  struct Bits
  {
    StorageType Save : 1;
    StorageType OnChangeEvent : 1;
    StorageType Reserved : 5;
    StorageType UserFlag0 : 1;
    StorageType UserFlag1 : 1;
    StorageType UserFlag2 : 1;
    StorageType UserFlag3 : 1;
    StorageType UserFlag4 : 1;
    StorageType UserFlag5 : 1;
    StorageType UserFlag6 : 1;
    StorageType UserFlag7 : 1;
    StorageType Invalid : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezBlackboardEntryFlags);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezBlackboardEntryFlags);


/// \brief A blackboard is a key/value store that provides OnChange events to be informed when a value changes.
///
/// Blackboards are used to gather typically small pieces of data. Some systems write the data, other systems read it.
/// Through the blackboard, arbitrary systems can interact.
///
/// For example this is commonly used in game AI, where some system gathers interesting pieces of data about the environment,
/// and then NPCs might use that information to make decisions.
class EZ_CORE_DLL ezBlackboard
{
public:
  ezBlackboard();
  ~ezBlackboard();

  struct Entry
  {
    ezVariant m_Value;
    ezBitflags<ezBlackboardEntryFlags> m_Flags;

    /// The change counter is increased every time the entry's value changes.
    /// Read this and compare it to a previous known value, to detect whether the value was changed since the last check.
    ezUInt32 m_uiChangeCounter = 0;
  };

  struct EntryEvent
  {
    ezHashedString m_sName;
    ezVariant m_OldValue;
    const Entry* m_pEntry;
  };

  /// \brief Registers an entry with a name, value and flags.
  ///
  /// If the same entry already exists, this will overwrite the existing entry (and broadcast an event if necessary).
  /// This is currently the only time at which the entry flags can be set.
  void RegisterEntry(const ezHashedString& name, const ezVariant& initialValue, ezBitflags<ezBlackboardEntryFlags> flags = ezBlackboardEntryFlags::None);

  /// \brief Removes the named entry. Does nothing, if no such entry exists.
  void UnregisterEntry(const ezHashedString& name);

  ///  \brief Removes all entries.
  void UnregisterAllEntries();

  /// \brief Sets the value of the named entry.
  ///
  /// The named entry has to have been registered before via RegisterEntry().
  ///
  /// If the 'OnChangeEvent' flag is set for this entry, OnEntryEvent() will be broadcast.
  /// However, if the new value is no different to the old, no event will be broadcast, unless 'force' is set to true.
  ///
  /// Logs an error, if the named entry hasn't been registered before.
  void SetEntryValue(const ezTempHashedString& name, const ezVariant& value, bool force = false);

  /// \brief Returns a pointer to the named entry, or nullptr if no such entry was registered.
  const Entry* GetEntry(const ezTempHashedString& name) const;

  /// \brief Returns the flags of the named entry, or ezBlackboardEntryFlags::Invalid, if no such entry was registered.
  ezBitflags<ezBlackboardEntryFlags> GetEntryFlags(const ezTempHashedString& name) const;

  /// \brief Returns the value of the named entry, or an invalid ezVariant, if no such entry was registered.
  ezVariant GetEntryValue(const ezTempHashedString& name) const;

  /// \brief Grants read access to the entire map of entries.
  const ezHashTable<ezHashedString, Entry>& GetAllEntries() const
  {
    return m_Entries;
  }

  /// \brief Allows you to register to the OnEntryEvent. This is broadcast whenever an entry is modified that has the flag ezBlackboardEntryFlags::OnEntryEvent.
  const ezEvent<EntryEvent>& OnEntryEvent() const
  {
    return m_EntryEvents;
  }

  /// \brief This counter is increased every time an entry is added or removed (but not when it is modified).
  ///
  /// Comparing this value to a previous known value allows to quickly detect whether the set of entries has changed.
  ezUInt32 GetBlackboardChangeCounter() const
  {
    return m_uiBlackboardChangeCounter;
  }

  /// \brief This counter is increased every time any entry's value is modified.
  ///
  /// Comparing this value to a previous known value allows to quickly detect whether any entry has changed recently.
  ezUInt32 GetBlackboardEntryChangeCounter() const
  {
    return m_uiBlackboardEntryChangeCounter;
  }

  /// \brief Stores all entries that have the 'Save' flag in the stream.
  ezResult Serialize(ezStreamWriter& stream) const;

  /// \brief Restores entries from the stream.
  ///
  /// If the blackboard already contains entries, the deserialized data is ADDED to the blackboard.
  /// If deserialized entries overlap with existing ones, the deserialized entries will overwrite the existing ones (both values and flags).
  ezResult Deserialize(ezStreamReader& stream);

private:
  ezEvent<EntryEvent> m_EntryEvents;
  ezUInt32 m_uiBlackboardChangeCounter = 0;
  ezUInt32 m_uiBlackboardEntryChangeCounter = 0;
  ezHashTable<ezHashedString, Entry> m_Entries;
};
