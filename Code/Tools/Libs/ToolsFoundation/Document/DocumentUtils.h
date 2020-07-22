#pragma once

#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class ezDocumentObject;
struct ezDocumentTypeDescriptor;

class EZ_TOOLSFOUNDATION_DLL ezDocumentUtils
{
public:
  static ezStatus IsValidSaveLocationForDocument(const char* szDocument, const ezDocumentTypeDescriptor** out_pTypeDesc = nullptr);
};
