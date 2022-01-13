#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginDLang/DLangAsset/DLangAssetObjects.h>
#include <Foundation/Communication/Event.h>

class ezDLangAssetDocument;

struct ezDLangAssetDocumentEvent
{
  enum class Type
  {
    None,
    ScriptCreated,
    ScriptOpened,
    ScriptTransformed,
  };

  Type m_Type = Type::None;
  ezDLangAssetDocument* m_pDocument = nullptr;
};

class ezDLangAssetDocument : public ezSimpleAssetDocument<ezDLangAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDLangAssetDocument, ezSimpleAssetDocument<ezDLangAssetProperties>);

public:
  ezDLangAssetDocument(const char* szDocumentPath);

  void EditScript();

  const ezEvent<const ezDLangAssetDocumentEvent&>& GetEvent() const { return m_Events; }

protected:
  void CreateComponentFile(const char* szFile);

  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;

  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  ezStatus ValidateDLangCode();
  //ezStatus AutoGenerateVariablesCode();

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;

  ezEvent<const ezDLangAssetDocumentEvent&> m_Events;
};
