#pragma once

#include <Foundation/Logging/TextFileWriter.h>
#include <GameEngine/GameApplication/GameApplication.h>

class ezPlayerApplication : public ezGameApplication
{
public:
  using SUPER = ezGameApplication;

  ezPlayerApplication();

protected:
  virtual ezResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeCoreSystemsShutdown() override;
  virtual void Run() override;
  virtual void StoreScreenshot(ezImage&& image, ezStringView sContext = {}) override;

private:
  void DetermineProjectPath();
  void SetupAutomation();
  void OnExecutionEvent(const ezGameApplicationExecutionEvent& e);
  void OnLogEvent(const ezLoggingEventData& e);

  // automation ('-runframes', '-timeout', '-screenshot', '-failonerror'), for using ezPlayer in automated tests
  ezTime m_StartTime;
  ezTime m_Timeout;           // zero = disabled
  ezInt32 m_iRunFrames = -1;  // negative = run until the user quits
  ezString m_sScreenshotPath; // empty = don't take one
  bool m_bFailOnError = false;
  bool m_bScreenshotRequested = false;
  bool m_bScreenshotDone = false;
  ezUInt32 m_uiRenderedFrames = 0;
  ezAtomicInteger32 m_iLoggedErrors = 0;

  ezLogWriter::TextFile m_LogFile;
  ezEventSubscriptionID m_ExecutionEventsID = 0;
  ezEventSubscriptionID m_LogToFileID = 0;
  ezEventSubscriptionID m_LogErrorCounterID = 0;
};
