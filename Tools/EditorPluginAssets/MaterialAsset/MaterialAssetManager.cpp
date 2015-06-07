#include <PCH.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetManager.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetWindow.moc.h>
#include <GuiFoundation/DockWindow/DockWindow.moc.h>
#include <EditorFramework/GUI/RawPropertyGridWidget.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMaterialAssetDocumentManager, ezAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezMaterialAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezMaterialAssetDocumentManager::ezMaterialAssetDocumentManager()
{
  ezDocumentManagerBase::s_Events.AddEventHandler(ezMakeDelegate(&ezMaterialAssetDocumentManager::OnDocumentManagerEvent, this));
}

ezMaterialAssetDocumentManager::~ezMaterialAssetDocumentManager()
{
  ezDocumentManagerBase::s_Events.RemoveEventHandler(ezMakeDelegate(&ezMaterialAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezMaterialAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManagerBase::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentManagerBase::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezMaterialAssetDocument>())
      {
        ezMaterialAssetDocumentWindow* pDocWnd = new ezMaterialAssetDocumentWindow(e.m_pDocument);

        {
          ezDockWindow* pPropertyPanel = new ezDockWindow(pDocWnd);
          pPropertyPanel->setObjectName("MaterialAssetDockWidget");
          pPropertyPanel->setWindowTitle("Material Properties");
          pPropertyPanel->show();

          ezRawPropertyGridWidget* pPropertyGrid = new ezRawPropertyGridWidget(e.m_pDocument, pPropertyPanel);
          pPropertyPanel->setWidget(pPropertyGrid);

          pDocWnd->addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

          e.m_pDocument->GetSelectionManager()->SetSelection(e.m_pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);

        }
      }
    }
    break;
  }
}

ezStatus ezMaterialAssetDocumentManager::InternalCanOpenDocument(const char* szDocumentTypeName, const char* szFilePath) const
{
  return EZ_SUCCESS;
}

ezStatus ezMaterialAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocumentBase*& out_pDocument)
{
  out_pDocument = new ezMaterialAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezMaterialAssetDocumentManager::InternalGetSupportedDocumentTypes(ezHybridArray<ezDocumentTypeDescriptor, 4>& out_DocumentTypes) const
{
  {
    ezDocumentTypeDescriptor td;
    td.m_bCanCreate = true;
    td.m_sDocumentTypeName = "Material Asset";
    td.m_sFileExtensions.PushBack("ezMaterialAsset");

    out_DocumentTypes.PushBack(td);
  }
}



