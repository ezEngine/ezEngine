#pragma once

#include <Foundation/Utilities/EnumerableClass.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Basics/Types/Bitflags.h>
#include <Foundation/Communication/Event.h>

/// \brief Describes of which type a CVar is. Use that info to cast an ezCVar* to the proper derived class.
struct ezCVarType
{
  enum Enum
  {
    Int,        ///< Can cast the ezCVar* to ezCVarInt*
    Float,      ///< Can cast the ezCVar* to ezCVarFloat*
    Bool,       ///< Can cast the ezCVar* to ezCVarBool*
    String,     ///< Can cast the ezCVar* to ezCVarString*
    ENUM_COUNT
  };
};

/// \brief The flags that can be used on an ezCVar.
struct ezCVarFlags
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    None              = 0,

    /// \brief If this flag is set, the CVar will be stored on disk and loaded again.
    /// Otherwise all changes to it will be lost on shutdown.
    Save              = 1 << 0,

    /// \brief Indicates that changing this cvar will only take effect after the proper subsystem has been reinitialized.
    /// This will always enforce the 'Save' flag as well.
    /// With this flag set, the 'Current' value never changes, unless 'SetToRestartValue' is called.
    RequiresRestart   = 1 << 1,

    /// \brief By default CVars are not saved.
    Default           = None
  };

  struct Bits
  {
    StorageType Save            : 1;
    StorageType RequiresRestart : 1;
  };
};

EZ_DECLARE_FLAGS_OR_OPERATOR(ezCVarFlags);

/// \todo Awesome detailed documentation
class EZ_FOUNDATION_DLL ezCVar : public ezEnumerable<ezCVar>
{
  EZ_DECLARE_ENUMERABLE_CLASS(ezCVar);

public:

  /// \brief Sets the path (folder) in which all CVar setting files should be stored.
  ///
  /// The path is used by SaveCVars and LoadCVars. However those functions will create one settings file per plugin,
  /// so \a szFolder must not be a file name, but only a path to a folder.
  ///
  /// After setting the storage folder, one should immediately load all CVars via LoadCVars.
  static void SetStorageFolder(const char* szFolder);

  /// \brief Searches all CVars for one with the given name. Returns NULL if no CVar could be found. The name is case-sensitive.
  static ezCVar* FindCVarByName(const char* szName);
  /// \brief Stores all CVar values in files in the storage folder, that must have been set via 'SetStorageFolder'.
  ///
  /// This function has no effect, if 'SetStorageFolder' has not been called, or the folder has been set to be empty.
  /// This function is also automatically called whenever plugin changes occur, or when the engine is shut down.
  /// So it might not be necessary to call this function manually at shutdown.
  static void SaveCVars();

  /// \brief Loads the CVars from the settings files in the storage folder.
  ///
  /// This function has no effect, if the storage folder has not been set via 'SetStorageFolder' yet
  /// or it has been set to be empty.
  /// If \a bOnlyNewOnes is set, only CVars that have been never been loaded from file before are loaded.
  /// All other CVars will stay unchanged.
  /// If \a bSetAsCurrentValue is true, variables that are flagged as 'RequiresRestart', will be set
  /// to the restart value immediately ('SetToRestartValue' is called on them).
  /// Otherwise their 'Current' value will always stay unchanged and the value from disk will only be
  /// stored in the 'Restart' value.
  /// Independent on the parameter settings, all CVar changes during loading will always trigger change events.
  static void LoadCVars(bool bOnlyNewOnes = true, bool bSetAsCurrentValue = true);



  /// \brief Copies the 'Restart' value into the 'Current' value.
  ///
  /// This change will not trigger a 'restart value changed' event, but it might trigger a 'current value changed' event.
  /// Code that uses a CVar that is flagged as 'RequiresRestart' for its initialization (and which is the reason, that that CVar
  /// is flagged as such) should always call this BEFORE it uses the CVar value.
  virtual void SetToRestartValue() = 0;

  /// \brief Returns the (display) name of the CVar.
  const char* GetName() const { return m_szName; }

  /// \brief Returns the type of the CVar.
  virtual ezCVarType::Enum GetType() const = 0;

  /// \brief Returns the description of the CVar.
  const char* GetDescription() const { return m_szDescription; }

  /// \brief Returns all the CVar flags.
  ezBitflags<ezCVarFlags> GetFlags() const { return m_Flags; }

  /// \brief The data that is broadcasted whenever a cvar is changed.
  struct CVarEvent
  {
    CVarEvent(ezCVar* pCVar) : m_pCVar(pCVar), m_EventType(ValueChanged) { }

    enum Type
    {
      ValueChanged,         ///< Sent whenever the 'Current' value of the CVar is changed.
      RestartValueChanged,  ///< Sent whenever the 'Restart' value of the CVar changes. It might actually change back to the 'Current' value though.
    };

    /// \brief The type of this event.
    Type m_EventType;

    /// \brief Which CVar is involved. This is only for convenience, it is always the CVar on which the event is triggered.
    ezCVar* m_pCVar;
  };

  /// \brief Code that needs to be execute whenever a cvar is changed can register itself here to be notified of such events.
  ezEvent<const CVarEvent&, void*> m_CVarEvents;

protected:
  ezCVar(const char* szName, ezBitflags<ezCVarFlags> Flags, const char* szDescription);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, CVars);

  static void AssignSubSystemPlugin(const char* szPluginName);
  static void PluginEventHandler(const ezPlugin::PluginEvent& EventData, void* pPassThrough);

  bool m_bHasNeverBeenLoaded;
  const char* m_szName;
  const char* m_szDescription;
  const char* m_szPluginName;
  ezBitflags<ezCVarFlags> m_Flags;

  static ezHybridString<32, ezStaticAllocatorWrapper> s_StorageFolder;
};

/// \brief Each CVar stores several values internally. The 'Current' value is the most important one.
struct ezCVarValue
{
  enum Enum
  {
    Current,    ///< The value that should be used.
    Default,    ///< The 'default' value of the CVar. Can be used to reset a variable to its default state.
    Stored,     ///< The value that was read from disk (or the default). Can be used to reset a CVar to the 'saved' state, if desired.
    Restart,    ///< The state that will be saved to disk. This is identical to 'Current' unless the 'RequiresRestart' flag is set (in which case the 'Current' value never changes).
    ENUM_COUNT
  };
};

/// \brief [internal] Helper class to implement ezCVarInt, ezCVarFlag, ezCVarBool and ezCVarString.
template<typename Type, ezCVarType::Enum CVarType>
class ezTypedCVar : public ezCVar
{
public:
  ezTypedCVar(const char* szName, const Type& Value, ezBitflags<ezCVarFlags> Flags, const char* szDescription);

  /// \brief Returns the 'current' value of the CVar. Same as 'GetValue(ezCVarValue::Current)'
  operator const Type&() const;

  /// \brief Returns the internal values of the CVar.
  const Type& GetValue(ezCVarValue::Enum val = ezCVarValue::Current) const;

  /// \brief Changes the CVar's value and broadcasts the proper events.
  ///
  /// Usually the 'Current' value is changed, unless the 'RequiresRestart' flag is set.
  /// In that case only the 'Restart' value is modified.
  void operator= (const Type& value);

  virtual ezCVarType::Enum GetType() const EZ_OVERRIDE;
  virtual void SetToRestartValue() EZ_OVERRIDE;

private:
  friend class ezCVar;

  Type m_Values[ezCVarValue::ENUM_COUNT];
};

/// \brief A CVar that stores a float value.
typedef ezTypedCVar<float, ezCVarType::Float> ezCVarFloat;

/// \brief A CVar that stores a bool value.
typedef ezTypedCVar<bool, ezCVarType::Bool> ezCVarBool;

/// \brief A CVar that stores an int value.
typedef ezTypedCVar<int, ezCVarType::Int> ezCVarInt;

/// \brief A CVar that stores a string.
typedef ezTypedCVar<ezHybridString<32, ezStaticAllocatorWrapper>, ezCVarType::String> ezCVarString;



#include <Foundation/Configuration/Implementation/CVar_inl.h>