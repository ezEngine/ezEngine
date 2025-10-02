#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Declarations.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/SimdMath/SimdTransform.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>
#include <ProcGenPlugin/ProcGenPluginDLL.h>

class ezExpressionByteCode;
using ezColorGradientResourceHandle = ezTypedResourceHandle<class ezColorGradientResource>;
using ezPrefabResourceHandle = ezTypedResourceHandle<class ezPrefabResource>;
using ezSurfaceResourceHandle = ezTypedResourceHandle<class ezSurfaceResource>;

struct ezProcGenBinaryOperator
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Add,
    Subtract,
    Multiply,
    Divide,
    Max,
    Min,

    Default = Multiply
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PROCGENPLUGIN_DLL, ezProcGenBinaryOperator);

struct ezProcGenBlendMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Add,
    Subtract,
    Multiply,
    Divide,
    Max,
    Min,
    Set,

    Default = Multiply
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PROCGENPLUGIN_DLL, ezProcGenBlendMode);

struct ezProcVertexColorChannelMapping
{
  using StorageType = ezUInt8;

  enum Enum
  {
    R,
    G,
    B,
    A,
    Black,
    White,

    Default = R
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PROCGENPLUGIN_DLL, ezProcVertexColorChannelMapping);

struct ezProcVertexColorMapping
{
  ezEnum<ezProcVertexColorChannelMapping> m_R = ezProcVertexColorChannelMapping::R;
  ezEnum<ezProcVertexColorChannelMapping> m_G = ezProcVertexColorChannelMapping::G;
  ezEnum<ezProcVertexColorChannelMapping> m_B = ezProcVertexColorChannelMapping::B;
  ezEnum<ezProcVertexColorChannelMapping> m_A = ezProcVertexColorChannelMapping::A;

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PROCGENPLUGIN_DLL, ezProcVertexColorMapping);

struct ezProcPlacementMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Raycast,
    RaycastHighQuality,
    Fixed,

    Default = Raycast
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PROCGENPLUGIN_DLL, ezProcPlacementMode);

struct ezProcPlacementPattern
{
  using StorageType = ezUInt8;

  enum Enum
  {
    RegularGrid,
    HexGrid,
    Natural,

    COUNT,

    Default = Natural
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PROCGENPLUGIN_DLL, ezProcPlacementPattern);

struct ezProcVolumeImageMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    ReferenceColor,
    ChannelR,
    ChannelG,
    ChannelB,
    ChannelA,

    Default = ReferenceColor
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PROCGENPLUGIN_DLL, ezProcVolumeImageMode);

//////////////////////////////////////////////////////////////////////////

namespace ezProcGenInternal
{
  class PlacementTile;
  class FindPlacementTilesTask;
  class PreparePlacementTask;
  class PlacementTask;
  class VertexColorTask;
  struct PlacementData;

  struct InvalidatedArea
  {
    ezBoundingBox m_Box;
    ezWorld* m_pWorld = nullptr;
  };

  struct Pattern
  {
    struct Point
    {
      float x;
      float y;
      float threshold;
    };

    ezArrayPtr<Point> m_Points;
    float m_fSize;
  };

  struct EZ_PROCGENPLUGIN_DLL GraphSharedDataBase : public ezRefCounted
  {
    virtual ~GraphSharedDataBase();
  };

  struct Output : public ezRefCounted
  {
    virtual ~Output();

    ezHashedString m_sName;

    ezHybridArray<ezUInt8, 4> m_VolumeTagSetIndices;
    ezSharedPtr<const GraphSharedDataBase> m_pGraphSharedData;

    ezUniquePtr<ezExpressionByteCode> m_pByteCode;
  };

  struct PlacementOutput : public Output
  {
    float GetTileSize() const { return m_pPattern->m_fSize * m_fFootprint; }

    bool IsValid() const
    {
      return !m_ObjectsToPlace.IsEmpty() && m_pPattern != nullptr && m_fFootprint > 0.0f && m_fCullDistance > 0.0f && m_pByteCode != nullptr;
    }

    ezHybridArray<ezPrefabResourceHandle, 4> m_ObjectsToPlace;

    const Pattern* m_pPattern = nullptr;
    float m_fFootprint = 1.0f;

    ezVec3 m_vMinOffset = ezVec3::MakeZero();
    ezVec3 m_vMaxOffset = ezVec3::MakeZero();

    ezAngle m_YawRotationSnap = ezAngle::MakeFromRadian(0.0f);
    float m_fAlignToNormal = 1.0f;

    ezVec3 m_vMinScale = ezVec3(1.0f);
    ezVec3 m_vMaxScale = ezVec3(1.0f);

    float m_fCullDistance = 30.0f;

    ezUInt32 m_uiCollisionLayer = 0;

    ezColorGradientResourceHandle m_hColorGradient;

    ezSurfaceResourceHandle m_hSurface;

    ezEnum<ezProcPlacementMode> m_Mode;
    ezUInt8 m_uiNumAdditionalRays = 4;
    float m_fRaySpread = 1.0f;
  };

  struct VertexColorOutput : public Output
  {
  };

  struct EZ_PROCGENPLUGIN_DLL ExpressionInputs
  {
    static ezHashedString s_sPosition;
    static ezHashedString s_sPositionX;
    static ezHashedString s_sPositionY;
    static ezHashedString s_sPositionZ;
    static ezHashedString s_sNormal;
    static ezHashedString s_sNormalX;
    static ezHashedString s_sNormalY;
    static ezHashedString s_sNormalZ;
    static ezHashedString s_sColor;
    static ezHashedString s_sColorR;
    static ezHashedString s_sColorG;
    static ezHashedString s_sColorB;
    static ezHashedString s_sColorA;
    static ezHashedString s_sPointIndex;
  };

  struct EZ_PROCGENPLUGIN_DLL ExpressionOutputs
  {
    static ezHashedString s_sOutDensity;
    static ezHashedString s_sOutScale;
    static ezHashedString s_sOutColorIndex;
    static ezHashedString s_sOutObjectIndex;

    static ezHashedString s_sOutColor;
    static ezHashedString s_sOutColorR;
    static ezHashedString s_sOutColorG;
    static ezHashedString s_sOutColorB;
    static ezHashedString s_sOutColorA;
  };

  struct PlacementPoint
  {
    EZ_DECLARE_POD_TYPE();

    ezVec3 m_vPosition;
    float m_fScale;
    ezVec3 m_vNormal;
    ezUInt8 m_uiColorIndex;
    ezUInt8 m_uiObjectIndex;
    ezUInt16 m_uiPointIndex;
  };

  struct PlacementTransform
  {
    EZ_DECLARE_POD_TYPE();

    ezSimdTransform m_Transform;
    ezColorLinear16f m_ObjectColor;
    ezUInt16 m_uiPointIndex;
    ezUInt8 m_uiObjectIndex;
    bool m_bHasValidColor;
    ezUInt32 m_uiPadding;
  };

  struct PlacementTileDesc
  {
    ezComponentHandle m_hComponent;
    ezUInt32 m_uiOutputIndex;
    ezInt32 m_iPosX;
    ezInt32 m_iPosY;
    float m_fMinZ;
    float m_fMaxZ;
    float m_fTileSize;
    float m_fDistanceToCamera;

    bool operator==(const PlacementTileDesc& other) const
    {
      return m_hComponent == other.m_hComponent && m_uiOutputIndex == other.m_uiOutputIndex && m_iPosX == other.m_iPosX && m_iPosY == other.m_iPosY;
    }

    ezBoundingBox GetBoundingBox() const
    {
      ezVec2 vCenter = ezVec2(m_iPosX * m_fTileSize, m_iPosY * m_fTileSize);
      ezVec3 vMin = (vCenter - ezVec2(m_fTileSize * 0.5f)).GetAsVec3(m_fMinZ);
      ezVec3 vMax = (vCenter + ezVec2(m_fTileSize * 0.5f)).GetAsVec3(m_fMaxZ);

      return ezBoundingBox::MakeFromMinMax(vMin, vMax);
    }

    ezHybridArray<ezSimdMat4f, 8, ezAlignedAllocatorWrapper> m_GlobalToLocalBoxTransforms;
  };
} // namespace ezProcGenInternal
