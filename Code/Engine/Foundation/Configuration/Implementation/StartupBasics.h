#pragma once

/// Describes the different stages during startup and shutdown
struct ezStartupStage
{
  enum Enum
  {
    Base,       ///< In this stage the absolute base functionality is started / shut down. This should only be used by the Foundation library.
    Core,       ///< In this stage the core functionality is being started / shut down
    Engine,     ///< In this stage the higher level functionality, which depends on a rendering context, is being started / shut down

    ENUM_COUNT
  };
};

/// [internal] Base class for all module declarations.
class EZ_FOUNDATION_DLL ezSubSystemDeclarationBase : public ezEnumerable<ezSubSystemDeclarationBase>
{
  EZ_DECLARE_ENUMERABLE_CLASS(ezSubSystemDeclarationBase);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezSubSystemDeclarationBase);

public:
  ezSubSystemDeclarationBase()
  {
    for (ezInt32 i = 0; i < ezStartupStage::ENUM_COUNT; ++i)
      m_bStartupDone[i] = false;
  }

  /// Returns the name of the subsystem.
  virtual const char* GetSubSystemName() const = 0;

  /// Returns the name of the module to which this subsystem belongs.
  virtual const char* GetModuleName() const = 0;

  /// Returns a series of strings with the names of the subsystem, which this subsystem depends on. NULL indicates the last entry.
  virtual const char* GetDependency(ezInt32 iDep) { return NULL; }

private:
  // only the startup system may access the following functionality
  friend class ezStartup;

  /// This will be called to initialize the subsystems base components.
  virtual void OnBaseStartup() { }

  /// This will be called to shut down the subsystems base components.
  virtual void OnBaseShutdown() { }

  /// This will be called to initialize the subsystems core components.
  virtual void OnCoreStartup() { }

  /// This will be called to shut down the subsystems core components.
  virtual void OnCoreShutdown() { }

  /// This will be called to initialize the subsystems engine / rendering components.
  virtual void OnEngineStartup() { }

  /// This will be called to shut down the subsystems engine / rendering components.
  virtual void OnEngineShutdown() { }

  /// Stores which startup phase has been done already.
  bool m_bStartupDone[ezStartupStage::ENUM_COUNT];
};

#include <Foundation/Configuration/StartupDeclarations.h>