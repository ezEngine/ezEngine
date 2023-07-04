#pragma once

#include <Core/Collection/CollectionResource.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <GameEngine/Utils/BlackboardTemplateResource.h>

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
  ezBlackboardTemplateAssetDocument(const char* szDocumentPath);

protected:
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  ezStatus RetrieveState(const ezBlackboardTemplateAssetObject* pProp, ezBlackboardTemplateResourceDescriptor& inout_Desc);
};
