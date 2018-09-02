#pragma once

class ezDocument;
class ezDocumentManager;
class ezDocumentObjectManager;
class ezAbstractObjectGraph;

struct ezDocumentFlags
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    None = 0,
    RequestWindow = EZ_BIT(0),
    AddToRecentFilesList = EZ_BIT(1),
    AsyncSave = EZ_BIT(2),
    Default = None,
  };

  struct Bits
  {
    StorageType RequestWindow : 1;
    StorageType AddToRecentFilesList : 1;
    StorageType AsyncSave : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezDocumentFlags);


struct EZ_TOOLSFOUNDATION_DLL ezDocumentTypeDescriptor
{
  ezDocumentTypeDescriptor()
  {
    m_pDocumentType = nullptr;
    m_bCanCreate = false;
    m_pManager = nullptr;
  }

  ezString m_sFileExtension;
  ezString m_sDocumentTypeName;
  bool m_bCanCreate;
  ezString m_sIcon;
  const ezRTTI* m_pDocumentType;
  ezDocumentManager* m_pManager;
};


struct ezDocumentEvent
{
  enum class Type
  {
    ModifiedChanged,
    ReadOnlyChanged,
    EnsureVisible,
    DocumentSaved,
    DocumentStatusMsg,
  };

  Type m_Type;
  const ezDocument* m_pDocument;

  const char* m_szStatusMsg;
};

class EZ_TOOLSFOUNDATION_DLL ezDocumentInfo : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentInfo, ezReflectedClass);

public:
  ezDocumentInfo();

  ezUuid m_DocumentID;
};

