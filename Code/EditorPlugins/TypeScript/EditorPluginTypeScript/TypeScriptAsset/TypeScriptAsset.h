#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetObjects.h>
#include <Foundation/Communication/Event.h>

class ezTypeScriptAssetDocument;

struct ezTypeScriptAssetDocumentEvent
{
  enum class Type
  {
    None,
    ScriptCreated,
    ScriptOpened,
    ScriptTransformed,
  };

  Type m_Type = Type::None;
  ezTypeScriptAssetDocument* m_pDocument = nullptr;
};

class ezTypeScriptAssetDocument : public ezSimpleAssetDocument<ezTypeScriptAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTypeScriptAssetDocument, ezSimpleAssetDocument<ezTypeScriptAssetProperties>);

public:
  ezTypeScriptAssetDocument(const char* szDocumentPath);

  virtual const char* QueryAssetType() const override;

  void EditScript();

  const ezEvent<const ezTypeScriptAssetDocumentEvent&>& GetEvent() const { return m_Events; }

protected:
  void CreateComponentFile(const char* szFile);
  void CreateTsConfigFiles();
  ezResult CreateTsConfigFile(const char* szDirectory);

  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;

  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  ezStatus AutoGenerateVariablesCode();

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;

  ezEvent<const ezTypeScriptAssetDocumentEvent&> m_Events;
};
