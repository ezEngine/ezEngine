
#pragma once


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

  ezApplication()
    : m_iReturnCode(0), m_iArgumentCount(0), m_ppArguments(NULL)
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
  /// Return false if the application should quit. You may set a return code via SetReturnCode() beforehand.
  virtual bool Run() = 0;


  inline void SetReturnCode(ezInt32 iReturnCode)
  {
    m_iReturnCode = iReturnCode;
  }

  inline ezInt32 GetReturnCode() const
  {
    return m_iReturnCode;
  }

  inline void SetCommandLineArguments(ezInt32 iArgumentCount, const char** ppArguments)
  {
    m_iArgumentCount = iArgumentCount;
    m_ppArguments = ppArguments;
  }


private:

  ezInt32 m_iReturnCode;

  ezInt32 m_iArgumentCount;

  const char** m_ppArguments;

  static ezApplication* s_pApplicationInstance;

  friend EZ_CORE_DLL void ezRun(ezApplication* pApplicationInstance);


};