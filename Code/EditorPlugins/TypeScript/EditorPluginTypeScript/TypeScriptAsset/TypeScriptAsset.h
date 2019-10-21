#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetObjects.h>

class ezTypeScriptAssetDocument : public ezSimpleAssetDocument<ezTypeScriptAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTypeScriptAssetDocument, ezSimpleAssetDocument<ezTypeScriptAssetProperties>);

public:
  ezTypeScriptAssetDocument(const char* szDocumentPath);

  virtual const char* QueryAssetType() const override;

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;
};
