#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <Animation/Resources/PropertyAnimResource.h>

class ezPropertyAnimAssetDocument : public ezSimpleAssetDocument<ezPropertyAnimResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPropertyAnimAssetDocument, ezSimpleAssetDocument<ezPropertyAnimResourceDescriptor>);

public:
  ezPropertyAnimAssetDocument(const char* szDocumentPath);

  virtual const char* QueryAssetType() const override { return "PropertyAnim"; }

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform, const ezAssetFileHeader& AssetHeader) override;
};
