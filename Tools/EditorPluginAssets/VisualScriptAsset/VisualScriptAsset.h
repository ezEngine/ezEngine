#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class ezVisualScriptAssetDocument : public ezAssetDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptAssetDocument, ezAssetDocument);

public:
  ezVisualScriptAssetDocument(const char* szDocumentPath);

  virtual const char* QueryAssetType() const override { return "Visual Script"; }

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override;

  virtual void InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const override;
  virtual void AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph) override;
};
