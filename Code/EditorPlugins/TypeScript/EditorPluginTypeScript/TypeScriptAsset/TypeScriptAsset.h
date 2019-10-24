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

  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;
  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;

  ezEvent<const ezTypeScriptAssetDocumentEvent&> m_Events;
};
