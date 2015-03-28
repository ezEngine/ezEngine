#include <PCH.h>
#include <EditorFramework/Assets/AssetDocument.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetDocumentInfo, ezDocumentInfo, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetDocument, ezDocumentBase, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezAssetDocument::ezAssetDocument(const char* szDocumentPath, ezDocumentObjectManagerBase* pObjectManager) : ezDocumentBase(szDocumentPath, pObjectManager)
{
}

ezAssetDocument::~ezAssetDocument()
{
}

ezDocumentInfo* ezAssetDocument::CreateDocumentInfo()
{
  return EZ_DEFAULT_NEW(ezAssetDocumentInfo);
}