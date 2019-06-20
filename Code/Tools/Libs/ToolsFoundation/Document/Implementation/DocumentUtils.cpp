#include <ToolsFoundationPCH.h>

#include <ToolsFoundation/Document/DocumentUtils.h>
#include <ToolsFoundation/Document/DocumentManager.h>

ezStatus ezDocumentUtils::IsValidSaveLocationForDocument(const char* szDocument, const ezDocumentTypeDescriptor** out_pTypeDesc)
{
  const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (ezDocumentManager::FindDocumentTypeFromPath(szDocument, true, pTypeDesc).Failed())
  {
    ezStringBuilder sTemp;
    sTemp.Format("The selected file extension '{0}' is not registered with any known type.\nCannot create file '{1}'",
      ezPathUtils::GetFileExtension(szDocument), szDocument);
    return ezStatus(sTemp.GetData());
  }

  if (ezDocument* pDocument = pTypeDesc->m_pManager->GetDocumentByPath(szDocument))
  {
    return ezStatus("The selected document is already open. You need to close the document before you can re-create it.");
  }

  if (out_pTypeDesc)
  {
    *out_pTypeDesc = pTypeDesc;
  }
  return ezStatus(EZ_SUCCESS);
}
