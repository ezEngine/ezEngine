#include <Player/Player.h>

#include <Core/Input/DeviceTypes/MouseKeyboard.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Texture/Image/Formats/ImageFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>

// this injects the main function
EZ_APPLICATION_ENTRY_POINT(ezPlayerApplication);

// these command line options may not all be directly used in ezPlayer, but the ezFallbackGameState reads those options to determine which scene to load
ezCommandLineOptionString opt_Project("_Player", "-project", "Path to the project folder.\nUsually an absolute path, though relative paths will work for projects that are located inside the EZ SDK directory.", "");
ezCommandLineOptionString opt_Scene("_Player", "-scene", "Path to a scene file.\nUsually given relative to the corresponding project data directory where it resides, but can also be given as an absolute path.", "");

// these options exist to run ezPlayer unattended, e.g. as a smoke test from a script
ezCommandLineOptionInt opt_RunFrames("_Player", "-runframes", "Quit automatically after this many rendered frames.\nUse this to check that a project starts up and renders at all.", -1, -1);
ezCommandLineOptionFloat opt_Timeout("_Player", "-timeout", "Quit automatically after this many seconds, no matter what.\nSafety net in case startup hangs. Sets the return code to 2 when it triggers.", 0.0f, 0.0f);
ezCommandLineOptionPath opt_Screenshot("_Player", "-screenshot", "Absolute path to a PNG file to write a screenshot to, right before quitting.\nOnly useful together with -runframes or -timeout.", "");
ezCommandLineOptionPath opt_LogFile("_Player", "-logfile", "Absolute path to a text file to write the full log to.", "");
ezCommandLineOptionBool opt_FailOnError("_Player", "-failonerror", "Set the return code to 1 if any error was logged during the run.", false);

ezPlayerApplication::ezPlayerApplication()
  : ezGameApplication("ezPlayer", nullptr) // we don't have a fixed project path in this app, so we need to pass that in a bit later
{
}

ezResult ezPlayerApplication::BeforeCoreSystemsStartup()
{
  // show the command line options, if help is requested
  {
    // since this is a GUI application (not a console app), printf has no effect
    // therefore we have to show the command line options with a message box

    ezStringBuilder cmdHelp;
    if (ezCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, ezCommandLineOption::LogAvailableModes::IfHelpRequested))
    {
      ezLog::OsMessageBox(cmdHelp);
      SetReturnCode(-1);
      return EZ_FAILURE;
    }
  }

  ezStartup::AddApplicationTag("player");

  // do this before anything else may log, so that the log file contains the entire startup
  SetupAutomation();

  EZ_SUCCEED_OR_RETURN(SUPER::BeforeCoreSystemsStartup());

  DetermineProjectPath();

  return EZ_SUCCESS;
}

void ezPlayerApplication::SetupAutomation()
{
  const ezStringBuilder sLogFile = opt_LogFile.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified);

  if (!sLogFile.IsEmpty())
  {
    // uses ezOSFile, so this works before the ezFileSystem is configured
    if (m_LogFile.BeginLog(sLogFile).Succeeded())
    {
      m_LogFile.SetTimestampMode(ezLog::TimestampMode::TimeOnly);
      m_LogToFileID = ezGlobalLog::AddLogWriter(ezMakeDelegate(&ezLogWriter::TextFile::LogMessageHandler, &m_LogFile));
    }
    else
    {
      ezLog::Error("Could not open log file '{}' for writing.", sLogFile);
    }
  }

  m_bFailOnError = opt_FailOnError.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified);

  if (m_bFailOnError)
  {
    m_LogErrorCounterID = ezGlobalLog::AddLogWriter(ezMakeDelegate(&ezPlayerApplication::OnLogEvent, this));
  }

  m_iRunFrames = opt_RunFrames.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified);
  m_Timeout = ezTime::MakeFromSeconds(opt_Timeout.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified));
  m_sScreenshotPath = opt_Screenshot.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified);
}


void ezPlayerApplication::AfterCoreSystemsStartup()
{
  // ezTime is only usable once the core systems are up, so the timeout can't start any earlier than this
  m_StartTime = ezTime::Now();

  ExecuteInitFunctions();

  ezStartup::StartupHighLevelSystems();

  // we need a game state to do anything
  // if no custom game state is available, ezFallbackGameState will be used
  // the game state is also responsible for either creating a world, or loading it
  // the ezFallbackGameState inspects the command line to figure out which scene to load
  ActivateGameState(nullptr, {}, ezTransform::MakeIdentity());

  if (m_iRunFrames >= 0 || !m_sScreenshotPath.IsEmpty())
  {
    m_ExecutionEventsID = m_ExecutionEvents.AddEventHandler(ezMakeDelegate(&ezPlayerApplication::OnExecutionEvent, this));
  }
}

void ezPlayerApplication::OnLogEvent(const ezLoggingEventData& e)
{
  if (e.m_EventType == ezLogMsgType::ErrorMsg || e.m_EventType == ezLogMsgType::SeriousWarningMsg)
  {
    m_iLoggedErrors.Increment();
  }
}

void ezPlayerApplication::OnExecutionEvent(const ezGameApplicationExecutionEvent& e)
{
  // BeforePresent, not AfterPresent: Run_FinishFrame() resets the 'take screenshot' flag at the end of each frame,
  // so the request has to be made before the frame is presented
  if (e.m_Type != ezGameApplicationExecutionEvent::Type::BeforePresent)
    return;

  ++m_uiRenderedFrames;

  if (m_bScreenshotRequested)
  {
    // the capture is started during the present of the frame in which it was requested and can only be retrieved
    // one or more frames later, so wait for it, rather than quitting with no (or a broken) file
    if (m_bScreenshotDone)
    {
      QuitApplication();
    }

    return;
  }

  if (m_iRunFrames < 0 || m_uiRenderedFrames < (ezUInt32)m_iRunFrames)
    return;

  if (m_sScreenshotPath.IsEmpty())
  {
    ezLog::Info("Rendered {} frames, quitting.", m_uiRenderedFrames);
    QuitApplication();
    return;
  }

  m_bScreenshotRequested = true;
  TakeScreenshot();
}

void ezPlayerApplication::StoreScreenshot(ezImage&& image, ezStringView sContext)
{
  if (m_sScreenshotPath.IsEmpty())
  {
    SUPER::StoreScreenshot(std::move(image), sContext);
    return;
  }

  m_bScreenshotDone = true;

  // written synchronously and through ezOSFile, so that the file is guaranteed to exist when the process exits
  // and the path doesn't have to be inside a writable data directory

  if (image.Convert(ezImageFormat::R8G8B8_UNORM_SRGB).Failed())
  {
    ezLog::Error("Could not convert the screenshot to RGB8.");
    return;
  }

  const ezStringView sExtension = ezPathUtils::GetFileExtension(m_sScreenshotPath);
  const ezImageFileFormat* pFormat = ezImageFileFormat::GetWriterFormat(sExtension);

  if (pFormat == nullptr)
  {
    ezLog::Error("No image file format is available to write '{}'.", m_sScreenshotPath);
    return;
  }

  ezDefaultMemoryStreamStorage storage;
  ezMemoryStreamWriter memoryWriter(&storage);

  if (pFormat->WriteImage(memoryWriter, image, sExtension).Failed())
  {
    ezLog::Error("Could not encode the screenshot as '{}'.", sExtension);
    return;
  }

  ezStringBuilder sFolder = m_sScreenshotPath;
  sFolder.PathParentDirectory();

  if (!sFolder.IsEmpty() && ezOSFile::CreateDirectoryStructure(sFolder).Failed())
  {
    ezLog::Error("Could not create the folder for screenshot '{}'.", m_sScreenshotPath);
    return;
  }

  ezOSFile file;
  if (file.Open(m_sScreenshotPath, ezFileOpenMode::Write).Failed())
  {
    ezLog::Error("Could not open screenshot file '{}' for writing.", m_sScreenshotPath);
    return;
  }

  ezMemoryStreamReader memoryReader(&storage);
  ezHybridArray<ezUInt8, 4096> chunk;
  chunk.SetCountUninitialized(4096);

  while (const ezUInt64 uiRead = memoryReader.ReadBytes(chunk.GetData(), chunk.GetCount()))
  {
    if (file.Write(chunk.GetData(), uiRead).Failed())
    {
      ezLog::Error("Could not write screenshot file '{}'.", m_sScreenshotPath);
      return;
    }
  }

  ezLog::Success("Screenshot: '{}'", m_sScreenshotPath);
}

void ezPlayerApplication::Run()
{
  SUPER::Run();

  // checked outside the frame events, because those don't fire while the window is minimized or the app hangs during startup
  if (m_Timeout.IsPositive() && ezTime::Now() - m_StartTime > m_Timeout)
  {
    ezLog::Error("Timeout of {} seconds reached, quitting.", m_Timeout.GetSeconds());
    SetReturnCode(2);
    QuitApplication();
  }
}

void ezPlayerApplication::BeforeCoreSystemsShutdown()
{
  if (m_ExecutionEventsID != 0)
  {
    m_ExecutionEvents.RemoveEventHandler(m_ExecutionEventsID);
    m_ExecutionEventsID = 0;
  }

  if (m_bFailOnError && m_iLoggedErrors > 0)
  {
    ezLog::Info("{} errors were logged.", (ezInt32)m_iLoggedErrors);

    if (GetReturnCode() == 0)
    {
      SetReturnCode(1);
    }
  }

  SUPER::BeforeCoreSystemsShutdown();

  if (m_LogErrorCounterID != 0)
  {
    ezGlobalLog::RemoveLogWriter(m_LogErrorCounterID);
  }

  if (m_LogToFileID != 0)
  {
    ezGlobalLog::RemoveLogWriter(m_LogToFileID);
    m_LogFile.EndLog();
  }
}

void ezPlayerApplication::DetermineProjectPath()
{
  ezStringBuilder sProjectPath = opt_Project.GetOptionValue(ezCommandLineOption::LogMode::FirstTime);

#if EZ_DISABLED(EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  // We can't specify command line arguments on many platforms so the project must be defined by ezFileserve.
  // ezFileserve must be started with the project special dir set. For example:
  // -specialdirs project ".../ezEngine/Data/Samples/Testing Chambers

  if (sProjectPath.IsEmpty())
  {
    m_sAppProjectPath = ">project";
    return;
  }
#endif

  if (sProjectPath.IsEmpty())
  {
    const ezStringBuilder sScenePath = opt_Scene.GetOptionValue(ezCommandLineOption::LogMode::FirstTime);

    // project path is empty, need to extract it from the scene path

    if (!sScenePath.IsAbsolutePath())
    {
      // scene path is not absolute -> can't extract project path
      m_sAppProjectPath = ezFileSystem::GetSdkRootDirectory();
      SetReturnCode(1);
      return;
    }

    if (ezFileSystem::FindFolderWithSubPath(sProjectPath, sScenePath, "ezProject", "ezSdkRoot.txt").Failed())
    {
      // couldn't find the 'ezProject' file in any parent folder of the scene
      m_sAppProjectPath = ezFileSystem::GetSdkRootDirectory();
      SetReturnCode(1);
      return;
    }
  }
  else if (!ezPathUtils::IsAbsolutePath(sProjectPath))
  {
    // project path is not absolute, so must be relative to the SDK directory
    sProjectPath.Prepend(ezFileSystem::GetSdkRootDirectory(), "/");
  }

  sProjectPath.MakeCleanPath();
  sProjectPath.TrimWordEnd("/ezProject");

  if (sProjectPath.IsEmpty())
  {
    m_sAppProjectPath = ezFileSystem::GetSdkRootDirectory();
    SetReturnCode(1);
    return;
  }

  // store it now, even if it fails, for error reporting
  m_sAppProjectPath = sProjectPath;
}
