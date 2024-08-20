#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/LUTAsset/LUTAsset.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetManager.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetWindow.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLUTAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezLUTAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezLUTAssetDocumentManager::ezLUTAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezLUTAssetDocumentManager::OnDocumentManagerEvent, this));

  // LUT asset source files
  ezAssetFileExtensionWhitelist::AddAssetFileExtension("LUT", "cube");

  m_DocTypeDesc.m_sDocumentTypeName = "LUT";
  m_DocTypeDesc.m_sFileExtension = "ezLUTAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/LUT.svg";
  m_DocTypeDesc.m_sAssetCategory = "Rendering";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezLUTAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_sResourceFileExtension = "ezBinLUT";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::None;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Texture_3D");

  ezQtImageCache::GetSingleton()->RegisterTypeImage("LUT", QPixmap(":/AssetIcons/LUT.svg"));

  // ezQtImageCache::GetSingleton()->RegisterTypeImage("LUT", QPixmap(":/AssetIcons/Render_Target.svg"));
}

ezLUTAssetDocumentManager::~ezLUTAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezLUTAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezLUTAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezLUTAssetDocument>())
      {
        new ezQtLUTAssetDocumentWindow(static_cast<ezLUTAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void ezLUTAssetDocumentManager::InternalCreateDocument(
  ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  ezLUTAssetDocument* pDoc = new ezLUTAssetDocument(sPath);
  out_pDocument = pDoc;
}

void ezLUTAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
