#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/ApplyNativePropertyChangesContext.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <EditorPluginAssets/CustomDataAsset/CustomDataAsset.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCustomDataAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezCustomDataAssetDocument::ezCustomDataAssetDocument(ezStringView sDocumentPath)
: ezSimpleAssetDocument<ezCustomDataAssetProperties>(sDocumentPath, ezAssetDocEngineConnection::None)
{
}

ezTransformStatus ezCustomDataAssetDocument::InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
                                                                    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  const ezCustomDataAssetProperties* pProp = GetProperties();

  pProp->m_pType->Save(stream);

  return ezStatus(EZ_SUCCESS);
}
