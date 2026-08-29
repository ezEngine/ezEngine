#include <McpPlugin/McpPluginPCH.h>

#include <McpPlugin/McpEngineHost.h>
#include <McpPlugin/Tools/McpEngineAppTool.h>

#include <Core/System/Window.h>
#include <Core/System/WindowManager.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/System/Process.h>
#include <Foundation/Time/Clock.h>

#include <Texture/Image/Formats/ImageFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageUtils.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMcpEngineAppTool, 1, ezRTTIDefaultAllocator<ezMcpEngineAppTool>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  /// Collects the registered windows, so that an index means the same thing in app_info and in
  /// app_screenshot.
  void GetWindows(ezDynamicArray<ezRegisteredWndHandle>& out_windows)
  {
    if (ezWindowManager* pMan = ezWindowManager::GetSingleton())
    {
      pMan->GetRegistered(out_windows);
    }
  }

  /// Writes an image to an absolute path, without going through the ezFileSystem.
  ///
  /// ezImageView::SaveTo() uses ezFileWriter, which only reaches paths inside a writable data
  /// directory - and the whole point of the path this tool returns is that the caller picked it, so it
  /// is usually somewhere else entirely. ezGameApplicationBase::StoreScreenshot() is no use either: it
  /// writes into ':appdata/Screenshots' under a generated name, on a worker task, so there is no path
  /// to report back and no moment at which the file is known to exist.
  ezResult WriteImageToAbsolutePath(const ezImageView& image, ezStringView sAbsolutePath)
  {
    const ezStringView sExtension = ezPathUtils::GetFileExtension(sAbsolutePath);
    const ezImageFileFormat* pFormat = ezImageFileFormat::GetWriterFormat(sExtension);

    if (pFormat == nullptr)
    {
      ezLog::Error("No image file format is available to write '{}'.", sAbsolutePath);
      return EZ_FAILURE;
    }

    // Encoded into memory first, because ezOSFile is not an ezStreamWriter and the format writers only
    // take one of those. A failed encode then also leaves no half-written file behind.
    ezDefaultMemoryStreamStorage storage;
    ezMemoryStreamWriter memoryWriter(&storage);

    if (pFormat->WriteImage(memoryWriter, image, sExtension).Failed())
    {
      ezLog::Error("Could not encode the image as '{}'.", sExtension);
      return EZ_FAILURE;
    }

    ezStringBuilder sFolder = sAbsolutePath;
    sFolder.PathParentDirectory();

    if (!sFolder.IsEmpty() && ezOSFile::CreateDirectoryStructure(sFolder).Failed())
    {
      ezLog::Error("Could not create the folder for '{}'.", sAbsolutePath);
      return EZ_FAILURE;
    }

    ezOSFile file;
    if (file.Open(sAbsolutePath, ezFileOpenMode::Write).Failed())
    {
      ezLog::Error("Could not open '{}' for writing.", sAbsolutePath);
      return EZ_FAILURE;
    }

    ezMemoryStreamReader memoryReader(&storage);
    ezHybridArray<ezUInt8, 4096> chunk;
    chunk.SetCountUninitialized(4096);

    while (const ezUInt64 uiRead = memoryReader.ReadBytes(chunk.GetData(), chunk.GetCount()))
    {
      if (file.Write(chunk.GetData(), uiRead).Failed())
      {
        ezLog::Error("Could not write '{}'.", sAbsolutePath);
        return EZ_FAILURE;
      }
    }

    return EZ_SUCCESS;
  }
} // namespace

ezStringView ezMcpEngineAppTool::GetBuildTimestamp() const
{
  // this plugin's own timestamp, not the Mcp library's - the tool list comes from here
  return __DATE__ " " __TIME__;
}

ezStringView ezMcpEngineAppTool::GetRelaunchHint() const
{
  return "To start a game again afterwards, run ezPlayer with "
         "'-project <path-to-project-folder> -scene <path-to-binary-scene> -mcpport <port>'. It serves MCP at "
         "http://127.0.0.1:<port>/mcp once the scene is loaded, which takes a few seconds - poll the port rather than "
         "assuming a delay. Without '-mcpport' no server is started at all, which is the default. "
         "If this process is the editor's engine process instead, it is not launched directly: quitting it ends the "
         "editor's play-the-game session, and starting another one is done through the editor.";
}

void ezMcpEngineAppTool::AddHostInfo(ezMcpJsonWriter& ref_writer)
{
  // Which of the two hosts this is. They answer the same tools but are not interchangeable: one is
  // launched directly and free-runs, the other is a child of the editor and only renders when asked.
  ref_writer.AddVariableString("host", ezApplication::GetApplicationInstance()->GetApplicationName());

  ref_writer.AddVariableUInt64("frameCount", ezMcpEngineHost::GetFrameCount());

  // Where this process writes its log. Worth reporting for the same reason as in the editor: a crash
  // takes the server with it, and the file is what is left to read afterwards.
  ezStringBuilder sAbsLogFile;
  if (ezFileSystem::ResolvePath(":appdata/Log.htm", &sAbsLogFile, nullptr).Succeeded())
  {
    ref_writer.AddVariableString("logFileFolder", sAbsLogFile.GetFileDirectory());
  }

  // The windows app_screenshot can capture, by the index it takes.
  ezHybridArray<ezRegisteredWndHandle, 4> windows;
  GetWindows(windows);

  ezWindowManager* pMan = ezWindowManager::GetSingleton();

  ref_writer.BeginArray("windows");
  for (ezUInt32 i = 0; i < windows.GetCount(); ++i)
  {
    ref_writer.BeginObject();
    ref_writer.AddVariableUInt32("index", i);
    ref_writer.AddVariableString("name", pMan->GetName(windows[i]));

    if (const ezWindowBase* pWindow = pMan->GetWindow(windows[i]))
    {
      const ezSizeU32 size = pWindow->GetClientAreaSize();
      ref_writer.AddVariableUInt32("width", size.width);
      ref_writer.AddVariableUInt32("height", size.height);
    }

    // No output target means nothing renders into it, so app_screenshot cannot use it.
    ref_writer.AddVariableBool("canCapture", pMan->GetOutputTarget(windows[i]) != nullptr);
    ref_writer.EndObject();
  }
  ref_writer.EndArray();
}

void ezMcpEngineAppTool::RequestQuit(bool bDiscardChanges)
{
  EZ_IGNORE_UNUSED(bDiscardChanges);

  // Deferred to the end of the frame, because the response has not reached the socket yet.
  ezMcpEngineHost::RequestQuit();
}

void ezMcpEngineAppTool::GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const
{
  SUPER::GetSupportedTools(out_tools);

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "app_screenshot";
    desc.m_sDescription =
      "Captures what the game is currently rendering, writes it to an image file and returns the absolute path to it. "
      "Read that file to actually look at the frame - this is the only way to see what the game looks like, and a "
      "rendered frame is the evidence that a change works, not that the process did not crash.\n"
      "The path is returned rather than the image data, because a screenshot base64s to megabytes.\n"
      "The capture takes a frame or two, so the call blocks until the image is ready. If the game is not rendering - "
      "which is the normal state of the editor's engine process while nobody is playing the game - this fails after a "
      "few seconds rather than waiting forever; start play-the-game in the editor first.\n"
      "The result reports 'window' and 'windowName' for what was actually captured - worth checking, because in the "
      "editor's engine process there is one window per open document window and the default index 0 is whichever "
      "document was opened first, which is often a leftover asset preview rather than the scene. Target a specific one "
      "with 'windowName': an editor view is named 'EditorView <document guid>', so passing the guid alone finds it.";

    desc.m_sInputSchema = R"({"type":"object","properties":{"path":{"type":"string","description":"Absolute path to write to, including the file extension, which selects the format ('.png' unless there is a reason). Defaults to a generated name in the system temp folder."},"maxWidth":{"type":"number","description":"Downscale to at most this width, keeping the aspect ratio. Default 1280. Pass 0 for the full resolution."},"window":{"type":"number","description":"Which window to capture, as its index in app_info's 'windows'. Default 0. Ignored if 'windowName' is given."},"windowName":{"type":"string","description":"Capture the window whose name contains this, instead of picking by index. An editor view is named 'EditorView <document guid>', so the document guid alone selects that document's view."}}})";
  }
}

void ezMcpEngineAppTool::Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  if (sToolName == "app_screenshot")
  {
    ExecuteScreenshot(arguments, out_result);
    return;
  }

  SUPER::Execute(sToolName, arguments, out_result);
}

ezMcpEngineAppTool::~ezMcpEngineAppTool()
{
  OnDeactivate();
}

void ezMcpEngineAppTool::OnDeactivate()
{
  if (m_FrameSubscription != 0)
  {
    if (ezGameApplicationBase* pApp = ezGameApplicationBase::GetGameApplicationBaseInstance())
    {
      pApp->m_ExecutionEvents.RemoveEventHandler(m_FrameSubscription);
    }

    m_FrameSubscription = 0;
  }
}

void ezMcpEngineAppTool::ExecutionEventHandler(const ezGameApplicationExecutionEvent& e)
{
  if (e.m_Type != ezGameApplicationExecutionEvent::Type::BeforePresent)
    return;

  if (m_State != CaptureState::Pending)
    return;

  ezHybridArray<ezRegisteredWndHandle, 4> windows;
  GetWindows(windows);

  ezWindowOutputTargetBase* pOutputTarget = nullptr;

  if (m_iCaptureWindow < static_cast<ezInt32>(windows.GetCount()))
  {
    pOutputTarget = ezWindowManager::GetSingleton()->GetOutputTarget(windows[m_iCaptureWindow]);
  }

  if (pOutputTarget == nullptr)
  {
    // the window went away mid-capture, e.g. the editor ended the play-the-game session
    m_State = CaptureState::Failed;
    m_sCaptureError = "The window disappeared while the screenshot was being captured.";
    return;
  }

  const ezEnum<ezCaptureImageResult> res = pOutputTarget->WaitCaptureImage(m_CapturedImage);

  if (res == ezCaptureImageResult::Ready)
  {
    m_State = CaptureState::Ready;
    return;
  }

  if (res == ezCaptureImageResult::NotStarted)
  {
    // Accepted and then dropped - the swap chain's back buffer was not usable when the renderer got to
    // it. Not a timing problem, so waiting longer would not help.
    m_State = CaptureState::Failed;
    m_sCaptureError = "The capture was discarded before it produced an image. The window's swap chain was not in a "
                      "usable state - a window that is minimised or in the middle of being resized does this. Try "
                      "again once it is visible.";
  }
}

void ezMcpEngineAppTool::ExecuteScreenshot(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  // Re-entered once per frame while the capture is in flight - see ezMcpToolResult::m_bNotFinished. The
  // arguments are the same every time, so they are only read on the first pass.
  if (m_State == CaptureState::Idle)
  {
    if (BeginScreenshot(arguments, out_result).Failed())
      return;

    out_result.m_bNotFinished = true;
    return;
  }

  if (m_State == CaptureState::Ready)
  {
    FinishScreenshot(out_result);
    m_State = CaptureState::Idle;
    return;
  }

  if (m_State == CaptureState::Failed)
  {
    out_result.SetError(m_sCaptureError);
    m_State = CaptureState::Idle;
    return;
  }

  if (ezTime::Now() - m_CaptureStarted < s_CaptureTimeout)
  {
    out_result.m_bNotFinished = true;
    return;
  }

  m_State = CaptureState::Idle;
  out_result.SetError("The screenshot never became ready. A capture only completes once another frame has been "
                      "presented, so this means the game is not rendering: in the editor's engine process that is "
                      "the normal state while play-the-game is not running, and the fix is to start it. A window "
                      "that is minimised does the same thing.");
}

ezResult ezMcpEngineAppTool::BeginScreenshot(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  ezHybridArray<ezRegisteredWndHandle, 4> windows;
  GetWindows(windows);

  if (windows.IsEmpty())
  {
    out_result.SetError("This process has no window registered, so there is nothing to capture. That is normal for a "
                        "process that was started without a graphics device.");
    return EZ_FAILURE;
  }

  ezWindowManager* pWinMan = ezWindowManager::GetSingleton();

  ezInt32 iWindow = 0;

  const ezStringBuilder sWantedName = ezMcpJson::GetString(arguments, "windowName");

  if (!sWantedName.IsEmpty())
  {
    iWindow = -1;

    for (ezUInt32 i = 0; i < windows.GetCount(); ++i)
    {
      // Substring, so that a document guid alone finds 'EditorView { guid }'.
      if (pWinMan->GetName(windows[i]).FindSubString_NoCase(sWantedName) != nullptr)
      {
        iWindow = static_cast<ezInt32>(i);
        break;
      }
    }

    if (iWindow < 0)
    {
      ezStringBuilder sError;
      sError.SetFormat("No window's name contains '{}'. This process has {}; see 'windows' in app_info for their names.",
        sWantedName, windows.GetCount());
      out_result.SetError(sError);
      return EZ_FAILURE;
    }
  }
  else
  {
    iWindow = static_cast<ezInt32>(ezMcpJson::GetInt(arguments, "window", 0));

    if (iWindow < 0 || iWindow >= static_cast<ezInt32>(windows.GetCount()))
    {
      ezStringBuilder sError;
      sError.SetFormat("There is no window {}. This process has {}; see 'windows' in app_info.", iWindow, windows.GetCount());
      out_result.SetError(sError);
      return EZ_FAILURE;
    }
  }

  ezWindowOutputTargetBase* pOutputTarget = pWinMan->GetOutputTarget(windows[iWindow]);

  if (pOutputTarget == nullptr)
  {
    out_result.SetError("That window has no output target, so nothing is rendered into it and there is nothing to capture.");
    return EZ_FAILURE;
  }

  ezStringBuilder sPath = ezMcpJson::GetString(arguments, "path");

  if (sPath.IsEmpty())
  {
    // Into the temp folder, and named by frame, so that repeated calls do not overwrite each other and
    // an agent can compare two frames without having to invent file names.
    sPath = ezOSFile::GetTempDataFolder("Screenshots");
    sPath.AppendFormat("/{}_{}.png", ezProcess::GetCurrentProcessID(), ezMcpEngineHost::GetFrameCount());
  }

  sPath.MakeCleanPath();

  if (!ezPathUtils::IsAbsolutePath(sPath))
  {
    ezStringBuilder sError;
    sError.SetFormat("'{}' is not an absolute path. The file is written straight to disk rather than into a data "
                     "directory, so there is nothing for a relative path to be relative to.",
      sPath);
    out_result.SetError(sError);
    return EZ_FAILURE;
  }

  // Subscribed lazily rather than in OnActivate(): the providers are created when the plugin loads, and
  // there is no ezGameApplicationBase yet at that point.
  if (m_FrameSubscription == 0)
  {
    ezGameApplicationBase* pApp = ezGameApplicationBase::GetGameApplicationBaseInstance();

    if (pApp == nullptr)
    {
      out_result.SetError("There is no game application, so there is no frame loop to complete a capture.");
      return EZ_FAILURE;
    }

    m_FrameSubscription = pApp->m_ExecutionEvents.AddEventHandler(ezMakeDelegate(&ezMcpEngineAppTool::ExecutionEventHandler, this));
  }

  if (pOutputTarget->StartCaptureImage().Failed())
  {
    out_result.SetError("Could not start the capture. Another one may still be in flight - try again in a moment.");
    return EZ_FAILURE;
  }

  m_State = CaptureState::Pending;
  m_sCapturePath = sPath;
  m_iCaptureWindow = iWindow;
  m_sCaptureWindowName = pWinMan->GetName(windows[iWindow]);
  m_uiCaptureMaxWidth = static_cast<ezUInt32>(ezMath::Max<ezInt64>(0, ezMcpJson::GetInt(arguments, "maxWidth", 1280)));
  m_CaptureStarted = ezTime::Now();

  return EZ_SUCCESS;
}

void ezMcpEngineAppTool::FinishScreenshot(ezMcpToolResult& out_result)
{
  ezImage image;
  image.ResetAndMove(std::move(m_CapturedImage));

  // Downscaled before encoding rather than after: a full resolution PNG is several megabytes and the
  // detail is not what the image is being looked at for.
  if (m_uiCaptureMaxWidth > 0 && image.GetWidth() > m_uiCaptureMaxWidth)
  {
    const ezUInt32 uiHeight = ezMath::Max(1u, (image.GetHeight() * m_uiCaptureMaxWidth) / image.GetWidth());

    ezImage scaled;
    if (ezImageUtils::Scale(image, scaled, m_uiCaptureMaxWidth, uiHeight).Succeeded())
    {
      image.ResetAndMove(std::move(scaled));
    }
  }

  // get rid of the alpha channel, which a back buffer carries and a viewer does not want
  if (image.Convert(ezImageFormat::R8G8B8_UNORM_SRGB).Failed())
  {
    out_result.SetError("Could not convert the screenshot to RGB8.");
    return;
  }

  if (WriteImageToAbsolutePath(image, m_sCapturePath).Failed())
  {
    ezStringBuilder sError;
    sError.SetFormat("Could not write the screenshot to '{}'. See log_read for the reason - a bad extension and a "
                     "folder that cannot be created both end up here.",
      m_sCapturePath);
    out_result.SetError(sError);
    return;
  }

  ezMcpJsonWriter writer;
  writer.BeginObject();
  writer.AddVariableString("path", m_sCapturePath);
  writer.AddVariableUInt32("width", image.GetWidth());
  writer.AddVariableUInt32("height", image.GetHeight());
  writer.AddVariableUInt64("frame", ezMcpEngineHost::GetFrameCount());

  writer.AddVariableInt32("window", m_iCaptureWindow);
  writer.AddVariableString("windowName", m_sCaptureWindowName);

  writer.AddVariableString("note", "Read this file to look at the frame.");
  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}
