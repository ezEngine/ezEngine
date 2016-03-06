#pragma once

class ezImage;
class ezAssetFileHeader;

struct ezAssetDocumentFlags
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    None = 0,
    AutoTransformOnSave = EZ_BIT(0),      ///< Every time the document is saved, TransformAsset is automatically executed
    TransformRequiresWindow = EZ_BIT(1),  ///< The document window is required to be able to transform this asset type
    DisableTransform = EZ_BIT(2),         ///< If set, TransformAsset will not do anything
    OnlyTransformManually = EZ_BIT(3),    ///< The asset transformation is not done, unless explicitely requested for this asset

    Default = None
  };

  struct Bits
  {
    StorageType AutoTransformOnSave : 1;
    StorageType TransformRequiresWindow : 1;
    StorageType DisableTransform : 1;
    StorageType OnlyTransformManually : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezAssetDocumentFlags)
