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
  ezString m_sFileExtension;
  ezString m_sDocumentTypeName;
  bool m_bCanCreate = true;
  ezString m_sIcon;
  const ezRTTI* m_pDocumentType = nullptr;
  ezDocumentManager* m_pManager = nullptr;

  /// This list is used to decide which asset types can be picked from the asset browser for a property.
  /// The strings are arbitrary and don't need to be registered anywhere else.
  /// An asset may be compatible for multiple scenarios, e.g. a skinned mesh may also be used as a static mesh, but not the other way round.
  /// In such a case the skinned mesh is set to be compatible to both "CompatibleAsset_Mesh_Static" and "CompatibleAsset_Mesh_Skinned", but the non-skinned mesh only to "CompatibleAsset_Mesh_Static".
  /// A component then only needs to specify that it takes an "CompatibleAsset_Mesh_Static" as input, and all asset types that are compatible to that will be browseable.
  ezHybridArray<ezString, 1> m_CompatibleTypes;
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
