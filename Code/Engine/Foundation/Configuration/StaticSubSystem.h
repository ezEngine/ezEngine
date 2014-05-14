#pragma once

/// \file

/// *** Example Subsystem declarations ***
///
///EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, ExampleSubSystem)
///
///  BEGIN_SUBSYSTEM_DEPENDENCIES
///    "SomeOtherSubSystem",
///    "SomeOtherSubSystem2"
///  END_SUBSYSTEM_DEPENDENCIES
///
///  ON_CORE_STARTUP
///  {
///    ezExampleSubSystem::BasicStartup();
///  }
///
///  ON_CORE_SHUTDOWN
///  {
///    ezExampleSubSystem::BasicShutdown();
///  }
///
///  ON_ENGINE_STARTUP
///  {
///    ezExampleSubSystem::EngineStartup();
///  }
///
///  ON_ENGINE_SHUTDOWN
///  {
///    ezExampleSubSystem::EngineShutdown();
///  }
///
///EZ_END_SUBSYSTEM_DECLARATION

/// \brief Put this in some cpp file of a subsystem to start its startup / shutdown sequence declaration.
///
/// The first parameter is the name of the group, in which the subsystem resides, the second is the name of the subsystem itself.
#define EZ_BEGIN_SUBSYSTEM_DECLARATION(GroupName, SubsystemName) \
  class GroupName##SubsystemName##SubSystem; \
  typedef GroupName##SubsystemName##SubSystem SubSystemType; \
  class GroupName##SubsystemName##SubSystem : public ezSubSystem { \
    public: virtual const char* GetGroupName() const { return #GroupName; } \
    public: virtual const char* GetSubSystemName() const { return #SubsystemName; } \

/// \brief Finishes a subsystem's startup / shutdown sequence declaration.
#define EZ_END_SUBSYSTEM_DECLARATION \
  }; static SubSystemType EZ_CONCAT(s_SubSystem, EZ_SOURCE_LINE);

/// \brief Defines what code is to be executed upon base startup.
///
/// Put this inside the subsystem declaration block.
#define ON_BASE_STARTUP \
  private: virtual void OnBaseStartup()

/// \brief Defines what code is to be executed upon core startup.
///
/// Put this inside the subsystem declaration block.
#define ON_CORE_STARTUP \
  private: virtual void OnCoreStartup()

/// \brief Defines what code is to be executed upon core shutdown.
///
/// Put this inside the subsystem declaration block.
#define ON_CORE_SHUTDOWN \
  private: virtual void OnCoreShutdown()

/// \brief Defines what code is to be executed upon engine startup.
///
/// Put this inside the subsystem declaration block.
#define ON_ENGINE_STARTUP \
  private: virtual void OnEngineStartup()

/// \brief Defines what code is to be executed upon engine shutdown.
///
/// Put this inside the subsystem declaration block.
#define ON_ENGINE_SHUTDOWN \
  private: virtual void OnEngineShutdown()

/// \brief Begins the list of subsystems, on which the currently declared system depends on.
///
/// Must be followed by a series of strings with the names of the dependencies.
#define BEGIN_SUBSYSTEM_DEPENDENCIES \
  public: virtual const char* GetDependency(ezInt32 iDep) \
  { \
    const char* szDeps[] = { \

/// \brief Ends the list of subsystems, on which the currently declared system depends on.
#define END_SUBSYSTEM_DEPENDENCIES \
      , nullptr }; \
    return szDeps[iDep]; \
  }

/// \brief This inserts a friend declaration into a class, such that the given group/subsystem can access private functions which it might need.
#define EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(GroupName, SubsystemName) \
  friend class GroupName##SubsystemName##SubSystem;


