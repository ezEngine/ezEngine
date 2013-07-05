
#pragma once

#include <Core/Basics.h>
#include <Core/Application/Implementation/ApplicationEntryPoint.h>

class ezApplication;

/// \brief Platform independent run function for mainloop based systems (e.g. Win32, ..)
EZ_CORE_DLL void ezRun(ezApplication* pApplicationInstance);

/// \brief Base class to be used by applications based on the ez engine.
///
/// The platform abstraction layer will ensure that the correct functions are called independent of the basic mainloop structure (traditional or event-based).
class EZ_CORE_DLL ezApplication
{
public:

  enum ApplicationExecution
  {
    Continue, ///< The 'Run' function should return this to keep the application running
    Quit,     ///< The 'Run' function should return this to quit the application
  };

  ezApplication()
    : m_iReturnCode(0), m_uiArgumentCount(0), m_ppArguments(NULL)
  {
  }

  virtual ~ezApplication()
  {
  }


  virtual void BeforeEngineInit()
  {
  }

  virtual void AfterEngineInit()
  {
  }

  virtual void BeforeEngineShutdown()
  {
  }

  virtual void AfterEngineShutdown()
  {
  }


  virtual void BeforeEnterBackground()
  {
  }

  virtual void BeforeEnterForeground()
  {
  }

  /// \brief Main run function which is called periodically.
  ///
  /// Return ApplicationExecution::Quit when the application should quit. You may set a return code via SetReturnCode() beforehand.
  virtual ApplicationExecution Run() = 0;


  inline void SetReturnCode(ezInt32 iReturnCode)
  {
    m_iReturnCode = iReturnCode;
  }

  inline ezInt32 GetReturnCode() const
  {
    return m_iReturnCode;
  }

  inline void SetCommandLineArguments(ezUInt32 uiArgumentCount, const char** ppArguments)
  {
    m_uiArgumentCount = uiArgumentCount;
    m_ppArguments = ppArguments;
  }

  static ezApplication* GetApplicationInstance()
  {
    return s_pApplicationInstance;
  }

  ezUInt32 GetArgumentCount() const
  {
    return m_uiArgumentCount;
  }

  const char* GetArgument(ezUInt32 uiArgument) const
  {
    EZ_ASSERT(uiArgument < m_uiArgumentCount, "There are only %i arguments, cannot access argument %i.", m_uiArgumentCount, uiArgument);

    return m_ppArguments[uiArgument];
  }

  const char** GetArgumentsArray() const
  {
    return m_ppArguments;
  }

private:

  ezInt32 m_iReturnCode;

  ezUInt32 m_uiArgumentCount;

  const char** m_ppArguments;

  static ezApplication* s_pApplicationInstance;

  friend EZ_CORE_DLL void ezRun(ezApplication* pApplicationInstance);


};