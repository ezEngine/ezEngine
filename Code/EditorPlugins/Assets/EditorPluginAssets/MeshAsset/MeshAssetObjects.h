#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/Util/AssetUtils.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
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

class ezMeshAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshAssetProperties, ezReflectedClass);

public:
  ezMeshAssetProperties();
  ~ezMeshAssetProperties();

  static void PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);

  ezString m_sMeshFile;
  float m_fUniformScaling = 1.0f;

  float m_fRadius = 0.5f;
  float m_fRadius2 = 0.5f;
  float m_fHeight = 1.0f;
  ezAngle m_Angle = ezAngle::Degree(360.0f);
  ezUInt16 m_uiDetail = 1;
  ezUInt16 m_uiDetail2 = 1;
  bool m_bCap = true;
  bool m_bCap2 = true;

  ezEnum<ezBasisAxis> m_RightDir = ezBasisAxis::PositiveY;
  ezEnum<ezBasisAxis> m_UpDir = ezBasisAxis::PositiveZ;
  bool m_bFlipForwardDir = false;

  ezMeshPrimitive::Enum m_PrimitiveType;

  bool m_bRecalculateNormals = false;
  bool m_bRecalculateTrangents = true;
  bool m_bImportMaterials = true;

  ezEnum<ezMeshNormalPrecision> m_NormalPrecision;
  ezEnum<ezMeshTexCoordPrecision> m_TexCoordPrecision;

  ezHybridArray<ezMaterialResourceSlot, 8> m_Slots;

  ezUInt32 m_uiVertices = 0;
  ezUInt32 m_uiTriangles = 0;
};
