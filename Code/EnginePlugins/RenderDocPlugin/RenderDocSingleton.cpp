#include <RenderDocPluginPCH.h>

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <RenderDocPlugin/RenderDocSingleton.h>
#include <RenderDocPlugin/ThirdParty/renderdoc_app.h>

EZ_IMPLEMENT_SINGLETON(ezRenderDoc);

static ezRenderDoc g_RenderDocSingleton;

static ezCommandLineOptionBool opt_NoCaptures("RenderDoc", "-NoCaptures", "Disables RenderDoc capture support.", false);

ezRenderDoc::ezRenderDoc()
  : m_SingletonRegistrar(this)
{
  if (opt_NoCaptures.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified))
  {
    ezLog::Info("RenderDoc plugin: Initialization suppressed via command-line.");
    return;
  }

  HMODULE dllHandle = GetModuleHandleW(L"renderdoc.dll");
  if (!dllHandle)
  {
    dllHandle = LoadLibraryW(L"renderdoc.dll");
    m_HandleToFree = ezMinWindows::FromNative(dllHandle);
  }

  if (!dllHandle)
  {
    ezLog::Info("RenderDoc plugin: 'renderdoc.dll' could not be found. Frame captures aren't possible.");
    return;
  }

  if (pRENDERDOC_GetAPI RenderDoc_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(dllHandle, "RENDERDOC_GetAPI"))
  {
    void* pApi = nullptr;
    RenderDoc_GetAPI(eRENDERDOC_API_Version_1_4_0, &pApi);
    m_pRenderDocAPI = (RENDERDOC_API_1_4_0*)pApi;
  }

  if (m_pRenderDocAPI)
  {
    m_pRenderDocAPI->SetCaptureKeys(nullptr, 0);
    m_pRenderDocAPI->SetFocusToggleKeys(nullptr, 0);
    m_pRenderDocAPI->MaskOverlayBits(0, 0);
  }
  else
  {
    ezLog::Warning("RenderDoc plugin: Unable to retrieve API pointer from DLL. Potentially outdated version. Frame captures aren't possible.");
  }
}

ezRenderDoc::~ezRenderDoc()
{
  m_pRenderDocAPI = nullptr;

  if (m_HandleToFree)
  {
    FreeLibrary(ezMinWindows::ToNative(m_HandleToFree));
    m_HandleToFree = nullptr;
  }
}

bool ezRenderDoc::IsInitialized() const
{
  return m_pRenderDocAPI != nullptr;
}

void ezRenderDoc::SetAbsCaptureFilePathTemplate(const char* szFilePathTemplate)
{
  if (m_pRenderDocAPI)
  {
    m_pRenderDocAPI->SetCaptureFilePathTemplate(szFilePathTemplate);
  }
}

const char* ezRenderDoc::GetAbsCaptureFilePathTemplate() const
{
  if (m_pRenderDocAPI)
  {
    return m_pRenderDocAPI->GetCaptureFilePathTemplate();
  }
  return nullptr;
}

void ezRenderDoc::StartFrameCapture(ezWindowHandle hWnd)
{
  if (m_pRenderDocAPI)
  {
    m_pRenderDocAPI->StartFrameCapture(nullptr, hWnd);
  }
}

bool ezRenderDoc::IsFrameCapturing() const
{
  return m_pRenderDocAPI ? m_pRenderDocAPI->IsFrameCapturing() : false;
}

void ezRenderDoc::EndFrameCaptureAndWriteOutput(ezWindowHandle hWnd)
{
  if (m_pRenderDocAPI)
  {
    m_pRenderDocAPI->EndFrameCapture(nullptr, hWnd);
  }
}

void ezRenderDoc::EndFrameCaptureAndDiscardResult(ezWindowHandle hWnd)
{
  if (m_pRenderDocAPI)
  {
    m_pRenderDocAPI->DiscardFrameCapture(nullptr, hWnd);
  }
}

ezResult ezRenderDoc::GetLastAbsCaptureFileName(ezStringBuilder& out_sFileName) const
{
  if (m_pRenderDocAPI && m_pRenderDocAPI->GetNumCaptures() > 0)
  {
    ezUInt32 uiNumCaptures = m_pRenderDocAPI->GetNumCaptures();
    ezUInt32 uiFilePathLength = 0;
    if (m_pRenderDocAPI->GetCapture(uiNumCaptures - 1, nullptr, &uiFilePathLength, nullptr))
    {
      ezHybridArray<char, 128> filePathBuffer;
      filePathBuffer.SetCount(uiFilePathLength);
      m_pRenderDocAPI->GetCapture(uiNumCaptures - 1, filePathBuffer.GetArrayPtr().GetPtr(), nullptr, nullptr);
      out_sFileName = filePathBuffer.GetArrayPtr().GetPtr();

      return EZ_SUCCESS;
    }
  }

  return EZ_FAILURE;
}

EZ_STATICLINK_FILE(RenderDocPlugin, RenderDocPlugin_RenderDocSingleton);
