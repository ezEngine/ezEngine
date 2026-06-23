#pragma once

#include <Core/Collection/CollectionResource.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <RendererCore/Components/BlackboardComponent.h>
#include <RendererCore/Utils/BlackboardTemplateResource.h>

struct ezBlackboardTemplateAssetObject : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezBlackboardTemplateAssetObject, ezReflectedClass);

  ezDynamicArray<ezString> m_BaseTemplates;
  ezDynamicArray<ezBlackboardEntry> m_Entries;
};

class ezBlackboardTemplateAssetDocument : public ezSimpleAssetDocument<ezBlackboardTemplateAssetObject>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezBlackboardTemplateAssetDocument, ezSimpleAssetDocument<ezBlackboardTemplateAssetObject>);

public:
  ezBlackboardTemplateAssetDocument(ezStringView sDocumentPath);

  ezStatus WriteAsset(ezStreamWriter& inout_stream, const ezPlatformProfile* pAssetProfile) const;

protected:
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& inout_stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  ezStatus RetrieveState(const ezBlackboardTemplateAssetObject* pProp, ezBlackboardTemplateResourceDescriptor& inout_Desc) const;
};
