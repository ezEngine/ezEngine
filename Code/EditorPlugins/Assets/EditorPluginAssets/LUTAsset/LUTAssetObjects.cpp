#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/LUTAsset/LUTAssetObjects.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLUTAssetProperties, 1, ezRTTIDefaultAllocator<ezLUTAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Input", GetInputFile, SetInputFile)->AddAttributes(new ezFileBrowserAttribute("Select CUBE file", "*.cube")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezLUTAssetProperties::PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezLUTAssetProperties>())
  {
    auto& props = *e.m_pPropertyStates;

    props["Input"].m_Visibility = ezPropertyUiState::Default;
    props["Input"].m_sNewLabelText = "ezLUTAssetProperties::CUBEfile";
  }
}

ezString ezLUTAssetProperties::GetAbsoluteInputFilePath() const
{
  ezStringBuilder sPath = m_sInput;
  sPath.MakeCleanPath();

  if (!sPath.IsAbsolutePath())
  {
    ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);
  }

  return sPath;
}
