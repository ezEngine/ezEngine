#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/Util/AssetUtils.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

struct ezPropertyMetaStateEvent;

struct ezMeshPrimitive
{
  typedef ezInt8 StorageType;

  enum Enum
  {
    File,
    Box,
    Rect,
    Cylinder,
    Cone,
    Pyramid,
    Sphere,
    HalfSphere,
    GeodesicSphere,
    Capsule,
    Torus,

    Default = File
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezMeshPrimitive);

struct ezMeshNormalPrecision
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    _10Bit,
    _16Bit,
    _32Bit,

    Default = _10Bit
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezMeshNormalPrecision);

struct ezMeshTexCoordPrecision
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    _16Bit,
    _32Bit,

    Default = _16Bit
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezMeshTexCoordPrecision);

class ezMeshAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshAssetProperties, ezReflectedClass);

public:
  ezMeshAssetProperties();

  static void PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);

  ezString m_sMeshFile;
  ezString m_sSubMeshName;
  float m_fUniformScaling;
  ezVec3 m_vNonUniformScaling;
  float m_fRadius;
  float m_fRadius2;
  float m_fHeight;
  ezAngle m_Angle;
  ezUInt16 m_uiDetail;
  ezUInt16 m_uiDetail2;
  bool m_bCap;
  bool m_bCap2;

  ezEnum<ezBasisAxis> m_ForwardDir;
  ezEnum<ezBasisAxis> m_RightDir;
  ezEnum<ezBasisAxis> m_UpDir;

  ezMeshPrimitive::Enum m_PrimitiveType;

  bool m_bRecalculateNormals;
  bool m_bInvertNormals;

  ezEnum<ezMeshNormalPrecision> m_NormalPrecision;
  ezEnum<ezMeshTexCoordPrecision> m_TexCoordPrecision;

  bool m_bImportMaterials;
  bool m_bUseSubFolderForImportedMaterials;
  ezHybridArray<ezMaterialResourceSlot, 8> m_Slots;

  ezUInt32 m_uiVertices;
  ezUInt32 m_uiTriangles;
};
