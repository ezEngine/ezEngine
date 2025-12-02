#include <EditorFramework/EditorFrameworkPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

#  include <EditorFramework/EditorApp/WindowsJumpList.h>
#  include <Foundation/Logging/Log.h>
#  include <ToolsFoundation/Utilities/RecentFilesList.h>

#  include <objbase.h>
#  include <propkey.h>
#  include <propvarutil.h>
#  include <shobjidl.h>

// Helper to create a shell link
static HRESULT CreateShellLink(const wchar_t* pPath, const wchar_t* pArguments, const wchar_t* pTitle, const wchar_t* pDescription, IShellLink*& out_pShellLink)
{
  HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&out_pShellLink));
  if (SUCCEEDED(hr))
  {
    // Set the path and arguments
    hr = out_pShellLink->SetPath(pPath);
    if (SUCCEEDED(hr))
    {
      hr = out_pShellLink->SetArguments(pArguments);
      if (SUCCEEDED(hr))
      {
        // Set the description (tooltip)
        if (pDescription != nullptr)
        {
          out_pShellLink->SetDescription(pDescription);
        }

        // Set the title using the property store
        IPropertyStore* pPropertyStore = nullptr;
        hr = out_pShellLink->QueryInterface(IID_PPV_ARGS(&pPropertyStore));
        if (SUCCEEDED(hr))
        {
          PROPVARIANT propVariant;
          hr = InitPropVariantFromString(pTitle, &propVariant);
          if (SUCCEEDED(hr))
          {
            hr = pPropertyStore->SetValue(PKEY_Title, propVariant);
            if (SUCCEEDED(hr))
            {
              hr = pPropertyStore->Commit();
            }
            PropVariantClear(&propVariant);
          }
          pPropertyStore->Release();
        }
      }
    }
  }

  if (FAILED(hr))
  {
    out_pShellLink->Release();
    out_pShellLink = nullptr;
  }

  return hr;
}

void ezWindowsJumpList::UpdateJumpList(const ezRecentFilesList& recentProjects)
{
  // Initialize COM if not already initialized
  HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  const bool bComInitialized = SUCCEEDED(hr);

  // Get the custom destination list interface
  ICustomDestinationList* pDestList = nullptr;
  hr = CoCreateInstance(CLSID_DestinationList, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDestList));
  if (FAILED(hr))
  {
    ezLog::Warning("Failed to create destination list: 0x{0:X}", static_cast<ezUInt32>(hr));
    if (bComInitialized)
      CoUninitialize();
    return;
  }

  // Begin building the list
  UINT cMinSlots = 0;
  IObjectArray* pRemovedItems = nullptr;
  hr = pDestList->BeginList(&cMinSlots, IID_PPV_ARGS(&pRemovedItems));
  if (FAILED(hr))
  {
    ezLog::Warning("Failed to begin destination list: 0x{0:X}", static_cast<ezUInt32>(hr));
    pDestList->Release();
    if (bComInitialized)
      CoUninitialize();
    return;
  }

  if (pRemovedItems)
    pRemovedItems->Release();

  // Get the current executable path
  wchar_t szExePath[MAX_PATH];
  if (GetModuleFileNameW(nullptr, szExePath, MAX_PATH) == 0)
  {
    ezLog::Warning("Failed to get module file name");
    pDestList->AbortList();
    pDestList->Release();
    if (bComInitialized)
      CoUninitialize();
    return;
  }

  // Create a collection for recent projects
  IObjectCollection* pObjectCollection = nullptr;
  hr = CoCreateInstance(CLSID_EnumerableObjectCollection, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pObjectCollection));
  if (FAILED(hr))
  {
    ezLog::Warning("Failed to create object collection: 0x{0:X}", static_cast<ezUInt32>(hr));
    pDestList->AbortList();
    pDestList->Release();
    if (bComInitialized)
      CoUninitialize();
    return;
  }

  // Add recent projects
  const auto& fileList = recentProjects.GetFileList();
  const ezUInt32 uiMaxItems = ezMath::Min<ezUInt32>(5, fileList.GetCount());

  for (ezUInt32 i = 0; i < uiMaxItems; ++i)
  {
    const auto& recentFile = fileList[i];
    ezStringBuilder sProjectPath = recentFile.m_File;

    // Get project directory (remove /ezProject suffix)
    if (sProjectPath.EndsWith_NoCase("/ezProject"))
    {
      sProjectPath.Shrink(0, 10); // Remove "/ezProject"
    }

    // Extract project name from path
    ezStringBuilder sProjectName = sProjectPath.GetFileName();

    // Convert to wide strings
    ezStringBuilder sArgumentsUtf8;
    sArgumentsUtf8.SetFormat("-project \"{}\"", recentFile.m_File);

    ezStringWChar szProjectPath(sProjectPath.GetData());
    ezStringWChar szArguments(sArgumentsUtf8.GetData());
    ezStringWChar szProjectName(sProjectName.GetData());

    // Create shell link
    IShellLink* pShellLink = nullptr;
    hr = CreateShellLink(szExePath, szArguments.GetData(), szProjectName.GetData(), szProjectPath.GetData(), pShellLink);
    if (SUCCEEDED(hr))
    {
      // Set the working directory to the project directory
      pShellLink->SetWorkingDirectory(szProjectPath.GetData());

      // Add to collection
      pObjectCollection->AddObject(pShellLink);
      pShellLink->Release();
    }
  }

  // Add the collection to the destination list as a custom category
  IObjectArray* pObjectArray = nullptr;
  hr = pObjectCollection->QueryInterface(IID_PPV_ARGS(&pObjectArray));
  if (SUCCEEDED(hr))
  {
    pDestList->AppendCategory(L"Recent Projects", pObjectArray);
    pObjectArray->Release();
  }

  pObjectCollection->Release();

  // Create a "Tasks" category with task entries
  IObjectCollection* pTasksCollection = nullptr;
  hr = CoCreateInstance(CLSID_EnumerableObjectCollection, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pTasksCollection));
  if (SUCCEEDED(hr))
  {
    // Create "New Window" task
    IShellLink* pNewWindowLink = nullptr;
    hr = CreateShellLink(szExePath, L"-noRecent", L"New Window", L"Open editor without loading a project", pNewWindowLink);
    if (SUCCEEDED(hr))
    {
      pTasksCollection->AddObject(pNewWindowLink);
      pNewWindowLink->Release();
    }

    // Create "Start in Safe Mode" task
    IShellLink* pSafeModeLink = nullptr;
    hr = CreateShellLink(szExePath, L"-safe", L"Start in Safe Mode", L"Start editor in safe mode (no automatic project/scene loading)", pSafeModeLink);
    if (SUCCEEDED(hr))
    {
      pTasksCollection->AddObject(pSafeModeLink);
      pSafeModeLink->Release();
    }

    // Add tasks category to the destination list
    IObjectArray* pTasksArray = nullptr;
    hr = pTasksCollection->QueryInterface(IID_PPV_ARGS(&pTasksArray));
    if (SUCCEEDED(hr))
    {
      pDestList->AppendCategory(L"Tasks", pTasksArray);
      pTasksArray->Release();
    }

    pTasksCollection->Release();
  }

  // Commit the list
  hr = pDestList->CommitList();
  if (FAILED(hr))
  {
    ezLog::Warning("Failed to commit destination list: 0x{0:X}", static_cast<ezUInt32>(hr));
  }

  pDestList->Release();

  if (bComInitialized)
    CoUninitialize();
}

#endif
