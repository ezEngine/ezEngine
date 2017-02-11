#pragma once

#include <ToolsFoundation/Document/Document.h>

class EZ_EDITORFRAMEWORK_DLL ezAssetDocumentInfo : public ezDocumentInfo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetDocumentInfo, ezDocumentInfo);

public:
  ezAssetDocumentInfo();

  ezUInt64 m_uiSettingsHash; ///< Current hash over all settings in the document, used to check resulting resource for being up-to-date in combination with dependency hashes.
  ezSet<ezString> m_FileDependencies;   ///< Files that are required to generate the asset, ie. if one changes, the asset needs to be recreated
  ezSet<ezString> m_FileReferences;     ///< Other files that are used at runtime together with this asset, e.g. materials for a mesh, needed for thumbnails and packaging.
  ezSet<ezString> m_Outputs; ///< Additional output this asset produces besides the default one. These are tags like VISUAL_SHADER that are resolved by the ezAssetDocumentManager into paths.
  ezString m_sAssetTypeName;
};


