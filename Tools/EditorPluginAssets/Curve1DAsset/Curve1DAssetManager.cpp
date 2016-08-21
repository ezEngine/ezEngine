#include <PCH.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAssetManager.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAsset.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAssetWindow.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCurve1DAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezCurve1DAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezCurve1DAssetDocumentManager::ezCurve1DAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezCurve1DAssetDocumentManager::OnDocumentManagerEvent, this));

  m_AssetDesc.m_bCanCreate = true;
  m_AssetDesc.m_sDocumentTypeName = "Curve1D Asset";
  m_AssetDesc.m_sFileExtension = "ezCurve1DAsset";
  m_AssetDesc.m_sIcon = ":/AssetIcons/Curve1D.png";
  m_AssetDesc.m_pDocumentType = ezGetStaticRTTI<ezCurve1DAssetDocument>();
  m_AssetDesc.m_pManager = this;
}

ezCurve1DAssetDocumentManager::~ezCurve1DAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezCurve1DAssetDocumentManager::OnDocumentManagerEvent, this));
}


ezBitflags<ezAssetDocumentFlags> ezCurve1DAssetDocumentManager::GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const
{
  EZ_ASSERT_DEBUG(pDescriptor->m_pManager == this, "Given type descriptor is not part of this document manager!");
  return ezAssetDocumentFlags::AutoTransformOnSave | ezAssetDocumentFlags::SupportsThumbnail;
}

void ezCurve1DAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezCurve1DAssetDocument>())
      {
        ezCurve1DAssetDocumentWindow* pDocWnd = new ezCurve1DAssetDocumentWindow(e.m_pDocument);
      }
    }
    break;
  }
}

ezStatus ezCurve1DAssetDocumentManager::InternalCanOpenDocument(const char* szDocumentTypeName, const char* szFilePath) const
{
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezCurve1DAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument)
{
  out_pDocument = new ezCurve1DAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezCurve1DAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
}



