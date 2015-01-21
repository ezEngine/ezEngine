
#pragma once

/// \file

#include <Foundation/Utilities/CommandLineUtils.h>
#include <Core/Basics.h>
#include <Core/Application/Implementation/ApplicationEntryPoint.h>

class ezApplication;

/// \brief Platform independent run function for main loop based systems (e.g. Win32, ..)
///
/// This is automatically called by EZ_APPLICATION_ENTRY_POINT() and EZ_CONSOLEAPP_ENTRY_POINT().
EZ_CORE_DLL void ezRun(ezApplication* pApplicationInstance);

/// \brief Base class to be used by applications based on ezEngine.
///
/// The platform abstraction layer will ensure that the correct functions are called independent of the basic main loop structure (traditional or event-based).
/// Derive an application specific class from ezApplication and implement at least the abstract Run() function.
/// Additional virtual functions allow to hook into specific events to run application specific code at the correct times.
///
/// Finally pass the name of your derived class to one of the macros EZ_APPLICATION_ENTRY_POINT() or EZ_CONSOLEAPP_ENTRY_POINT().
/// Those are used to abstract away the platform specific code to run an application.
///
/// A simple example how to get started is as follows:
///
/// \code{.cpp}
///   class ezSampleApp : public ezApplication
///   {
///   public:
///   
///     virtual void AfterEngineInit() override
///     {
///       // Setup Filesystem, Logging, etc.
///     }
///   
///     virtual void BeforeEngineShutdown() override
///     {
///       // Close log file, etc.
///     }
///   
///     virtual ezApplication::ApplicationExecution Run() override
///     {
///       // Either run a one-time application (e.g. console script) and return ezApplication::Quit
///       // Or run one update (frame) of your game loop and return ezApplication::Continue
///   
///       return ezApplication::Quit;
///     }
///   };
///   
///   EZ_APPLICATION_ENTRY_POINT(ezSampleApp);
/// \endcode
class EZ_CORE_DLL ezApplication
{
public:

  /// \brief Defines the possible return values for the ezApplication::Run() function.
  enum ApplicationExecution
  {
    Continue, ///< The 'Run' function should return this to keep the application running
    Quit,     ///< The 'Run' function should return this to quit the application
  };

  /// \brief Constructor.
  ezApplication() : 
    m_iReturnCode(0), 
    m_uiArgumentCount(0), 
    m_ppArguments(nullptr),
    m_bReportMemoryLeaks(true)
  {
  }

  /// \brief Virtual destructor.
  virtual ~ezApplication()
  {
  }

  /// \brief This function is called before any kind of engine initialization is done.
  ///
  /// Override this function to be able to configure subsystems, before they are initialized.
  /// After this function returns, ezStartup::StartupCore() is automatically called.
  /// If you need to set up custom allocators, this is the place to do this.
  virtual void BeforeEngineInit()
  {
  }

  /// \brief This function is called after basic engine initialization has been done.
  ///
  /// ezApplication will automatically call ezStartup::StartupCore() to initialize the application.
  /// This function can be overridden to do additional application specific initialization.
  /// To startup entire subsystems, you should however use the features provided by ezStartup and ezSubSystem.
  virtual void AfterEngineInit()
  {
  }

  /// \brief This function is called after the application main loop has run for the last time, before engine deinitialization.
  ///
  /// Override this function to do application specific deinitialization that still requires a running engine.
  /// After this function returns ezStartup::ShutdownBase() is called and thus everything, including allocators, is shut down.
  /// To shut down entire subsystems, you should however use the features provided by ezStartup and ezSubSystem.
  virtual void BeforeEngineShutdown()
  {
  }

  /// \brief This function is called after ezStartup::ShutdownBase() has been called.
  ///
  /// It is unlikely that there is any kind of deinitialization left, that can still be run at this point.
  virtual void AfterEngineShutdown()
  {
  }

  /// \brief This function is called when an application is moved to the background.
  ///
  /// On Windows that might simply mean that the main window lost the focus. 
  /// On other devices this might mean that the application is not visible at all anymore and
  /// might even get shut down later. Override this function to be able to put the application
  /// into a proper sleep mode.
  virtual void BeforeEnterBackground()
  {
  }

  /// \brief This function is called whenever an application is resumed from background mode.
  ///
  /// On Windows that might simply mean that the main window received focus again.
  /// On other devices this might mean that the application was suspended and is now active again.
  /// Override this function to reload the apps state or other resources, etc.
  virtual void BeforeEnterForeground()
  {
  }

  /// \brief Main run function which is called periodically. This function must be overridden.
  ///
  /// Return ApplicationExecution::Quit when the application should quit. You may set a return code via SetReturnCode() beforehand.
  virtual ApplicationExecution Run() = 0;

  /// \brief Sets the value that the application will return to the OS.
  /// You can call this function at any point during execution to update the return value of the application.
  /// Default is zero.
  inline void SetReturnCode(ezInt32 iReturnCode)
  {
    m_iReturnCode = iReturnCode;
  }

  /// \brief Returns the currently set value that the application will return to the OS.
  inline ezInt32 GetReturnCode() const
  {
    return m_iReturnCode;
  }

  /// \brief Will set the command line arguments that were passed to the app by the OS.
  /// This is automatically called by EZ_APPLICATION_ENTRY_POINT() and EZ_CONSOLEAPP_ENTRY_POINT().
  inline void SetCommandLineArguments(ezUInt32 uiArgumentCount, const char** ppArguments)
  {
    m_uiArgumentCount = uiArgumentCount;
    m_ppArguments = ppArguments;
    ezCommandLineUtils::GetInstance()->SetCommandLine(uiArgumentCount, ppArguments);
  }

  /// \brief Returns the one instance of ezApplication that is available.
  static ezApplication* GetApplicationInstance()
  {
    return s_pApplicationInstance;
  }

  /// \brief Returns the number of command lien arguments that were passed to the application.
  ///
  /// Note that the very first command line argument is typically the path to the application itself.
  ezUInt32 GetArgumentCount() const
  {
    return m_uiArgumentCount;
  }

  /// \brief Returns one of the command line arguments that was passed to the application.
  const char* GetArgument(ezUInt32 uiArgument) const
  {
    EZ_ASSERT_DEV(uiArgument < m_uiArgumentCount, "There are only %i arguments, cannot access argument %i.", m_uiArgumentCount, uiArgument);

    return m_ppArguments[uiArgument];
  }

  /// \brief Returns the complete array of command line arguments that were passed to the application.
  const char** GetArgumentsArray() const
  {
    return m_ppArguments;
  }

  void EnableMemoryLeakReporting(bool bEnable)
  {
    m_bReportMemoryLeaks = bEnable;
  }

private:

  ezInt32 m_iReturnCode;

  ezUInt32 m_uiArgumentCount;

  const char** m_ppArguments;

  bool m_bReportMemoryLeaks;

  static ezApplication* s_pApplicationInstance;

  friend EZ_CORE_DLL void ezRun(ezApplication* pApplicationInstance);
  };

