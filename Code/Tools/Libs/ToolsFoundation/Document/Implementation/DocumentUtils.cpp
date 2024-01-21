#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Document/DocumentUtils.h>

ezStatus ezDocumentUtils::IsValidSaveLocationForDocument(ezStringView sDocument, const ezDocumentTypeDescriptor** out_pTypeDesc)
{
  const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (ezDocumentManager::FindDocumentTypeFromPath(sDocument, true, pTypeDesc).Failed())
  {
    ezStringBuilder sTemp;
    sTemp.SetFormat("The selected file extension '{0}' is not registered with any known type.\nCannot create file '{1}'",
      ezPathUtils::GetFileExtension(sDocument), sDocument);
    return ezStatus(sTemp.GetData());
  }

  if (ezDocument* pDocument = pTypeDesc->m_pManager->GetDocumentByPath(sDocument))
  {
    return ezStatus("The selected document is already open. You need to close the document before you can re-create it.");
  }

  if (out_pTypeDesc)
  {
    *out_pTypeDesc = pTypeDesc;
  }
  return ezStatus(EZ_SUCCESS);
}
