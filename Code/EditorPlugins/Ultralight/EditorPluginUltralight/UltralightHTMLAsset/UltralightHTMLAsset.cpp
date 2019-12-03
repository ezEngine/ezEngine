#include <PCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginUltralight/UltralightHTMLAsset/UltralightHTMLAsset.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Strings/PathUtils.h>
#include <UltralightPlugin/Resources/UltralightHTMLResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezUltralightHTMLAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezUltralightHTMLAssetProperties, 1, ezRTTIDefaultAllocator<ezUltralightHTMLAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("HTMLContent", m_sHTMLContent),
    EZ_MEMBER_PROPERTY("HTMLFile", m_sHTMLFile)->AddAttributes(new ezFileBrowserAttribute("Select HTML File", "*.html")),

    EZ_MEMBER_PROPERTY("Width", m_uiWidth)->AddAttributes(new ezDefaultValueAttribute(512)),
    EZ_MEMBER_PROPERTY("Height", m_uiHeight)->AddAttributes(new ezDefaultValueAttribute(512)),

    EZ_MEMBER_PROPERTY("TransparentBackground", m_bTransparentBackground)->AddAttributes(new ezDefaultValueAttribute(false)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezUltralightHTMLAssetDocument::ezUltralightHTMLAssetDocument(const char* szDocumentPath)
    : ezSimpleAssetDocument<ezUltralightHTMLAssetProperties>(szDocumentPath, ezAssetDocEngineConnection::None)
{
}

void ezUltralightHTMLAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  const ezUltralightHTMLAssetProperties* pProp = GetProperties();

  // TODO: This doesn't cover dependencies which the HTML file itself has. We would need to load the file
  // with a dependency tracking file system to get at least the initial set of files required.
  if (!pProp->m_sHTMLFile.IsEmpty())
  {
    pInfo->m_AssetTransformDependencies.Insert(pProp->m_sHTMLFile);
  }
}

ezStatus ezUltralightHTMLAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
  const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  const ezUltralightHTMLAssetProperties* pProp = GetProperties();

  if (!pProp->m_sHTMLContent.IsEmpty() && !pProp->m_sHTMLFile.IsEmpty())
    return ezStatus("Both HTML file path and content are set. Pick either.");

  ezUltralightHTMLResourceDescriptor descriptor;
  descriptor.m_sHTMLContent = pProp->m_sHTMLContent;
  descriptor.m_sHTMLFileName = pProp->m_sHTMLFile;

  descriptor.m_uiWidth = pProp->m_uiWidth;
  descriptor.m_uiHeight = pProp->m_uiHeight;

  descriptor.m_bTransparentBackground = pProp->m_bTransparentBackground;

  descriptor.Save(stream);

  return ezStatus(EZ_SUCCESS);
}
