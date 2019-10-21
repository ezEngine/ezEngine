#include <EditorPluginTypeScriptPCH.h>

#include <Core/Graphics/Geometry.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAsset.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetManager.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetObjects.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Utilities/Progress.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTypeScriptAssetDocument, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezTypeScriptAssetDocument::ezTypeScriptAssetDocument(const char* szDocumentPath)
    : ezSimpleAssetDocument<ezTypeScriptAssetProperties>(szDocumentPath, ezAssetDocEngineConnection::Simple)
{
}

const char* ezTypeScriptAssetDocument::QueryAssetType() const
{
  return "TypeScript";
}

//////////////////////////////////////////////////////////////////////////

ezStatus ezTypeScriptAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag,
                                                          const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader,
                                                          ezBitflags<ezTransformFlags> transformFlags)
{

  return ezStatus(EZ_SUCCESS);
}

