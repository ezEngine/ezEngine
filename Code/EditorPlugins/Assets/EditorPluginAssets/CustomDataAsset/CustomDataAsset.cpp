#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/ApplyNativePropertyChangesContext.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <EditorPluginAssets/CustomDataAsset/CustomDataAsset.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCustomDataAssetProperties, 1, ezRTTIDefaultAllocator<ezCustomDataAssetProperties>)
EZ_BEGIN_PROPERTIES
{
  EZ_MEMBER_PROPERTY("Type", m_pType)->AddFlags(ezPropertyFlags::PointerOwner),
}
EZ_END_PROPERTIES;
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCustomDataAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezCustomDataAssetDocument::ezCustomDataAssetDocument(ezStringView sDocumentPath)
  : ezSimpleAssetDocument<ezCustomDataAssetProperties>(sDocumentPath, ezAssetDocEngineConnection::None)
{
}

ezTransformStatus ezCustomDataAssetDocument::InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
                                                                    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  ezAbstractObjectGraph abstractObjectGraph;
  ezDocumentObjectConverterWriter objectWriter(&abstractObjectGraph, GetObjectManager());

  ezDocumentObject* pObject = GetPropertyObject();

  ezVariant type = pObject->GetTypeAccessor().GetValue("Type");
  EZ_ASSERT_DEV(type.IsA<ezUuid>(), "Implementation error");

  if (ezDocumentObject* pDataObject = pObject->GetChild(type.Get<ezUuid>()))
  {
    ezAbstractObjectNode* pAbstractNode = objectWriter.AddObjectToGraph(pDataObject, "root");
    ezAbstractGraphBinarySerializer::Write(stream, &abstractObjectGraph);
    
    return ezStatus(EZ_SUCCESS);
  }

  return ezStatus(EZ_FAILURE);
}
