#include <PCH.h>
#include <ToolsFoundationTest/Object/TestObjectManager.h>

ezTestDocumentObjectManager::ezTestDocumentObjectManager()
{
}

ezTestDocumentObjectManager::~ezTestDocumentObjectManager()
{
}

void ezTestDocumentObjectManager::GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const
{
}

ezDocumentObject* ezTestDocumentObjectManager::InternalCreateObject(const ezRTTI* pRtti)
{
  auto pObject = EZ_DEFAULT_NEW(ezDocumentStorageObject, pRtti);
  return pObject;
}
