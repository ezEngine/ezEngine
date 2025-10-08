#pragma once

#include <Core/Graphics/Spline.h>
#include <Core/World/World.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Types/TagSet.h>
#include <ProcGenPlugin/Declarations.h>

using ezImageDataResourceHandle = ezTypedResourceHandle<class ezImageDataResource>;

class EZ_PROCGENPLUGIN_DLL ezVolumeCollection : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVolumeCollection, ezReflectedClass);

public:
  ezVolumeCollection();
  ~ezVolumeCollection();

  struct ShapeType
  {
    using StorageType = ezUInt8;

    enum Enum
    {
      Sphere,
      Box,
      Image,
      Spline,

      Default = Sphere
    };
  };

  struct Shape
  {
    ezVec4 m_GlobalToLocalTransform0;
    ezVec4 m_GlobalToLocalTransform1;
    ezVec4 m_GlobalToLocalTransform2;
    ezEnum<ShapeType> m_Type;
    ezEnum<ezProcGenBlendMode> m_BlendMode;
    ezFloat16 m_fValue;
    ezUInt32 m_uiSortingKey;

    EZ_ALWAYS_INLINE bool operator<(const Shape& other) const { return m_uiSortingKey < other.m_uiSortingKey; }

    void SetGlobalToLocalTransform(const ezSimdMat4f& t);
    ezSimdMat4f GetGlobalToLocalTransform() const;
  };

  struct Sphere : public Shape
  {
    EZ_DECLARE_POD_TYPE();

    float m_fFadeOut;
  };

  struct Box : public Shape
  {
    EZ_DECLARE_POD_TYPE();

    ezVec3 m_vPositiveFadeOut;
    ezVec3 m_vNegativeFadeOut;
  };

  struct Image : public Box
  {
    ezImageDataResourceHandle m_hImage;
    const ezColor* m_pPixelData = nullptr;
    ezUInt32 m_uiImageWidth = 0;
    ezUInt32 m_uiImageHeight = 0;
  };

  struct Spline : public Shape
  {
    ezSpline m_Spline;
    ezBoundingBox m_BoundingBox;
    float m_fInvRadius;
    float m_fFadeOut;
    float m_fMaxError;
  };

  bool IsEmpty() { return m_SortedShapes.IsEmpty(); }

  float EvaluateAtGlobalPosition(const ezSimdVec4f& vPosition, float fInitialValue, ezProcVolumeImageMode::Enum imgMode, const ezColor& refColor) const;

  static void ExtractVolumesInBox(const ezWorld& world, const ezBoundingBox& box, ezSpatialData::Category spatialCategory, const ezTagSet& includeTags, ezVolumeCollection& out_collection, const ezRTTI* pComponentBaseType = nullptr);

  void AddSphere(const ezSimdTransform& transform, float fRadius, ezEnum<ezProcGenBlendMode> blendMode, float fSortOrder, float fValue, float fFalloff);

  void AddBox(const ezSimdTransform& transform, const ezVec3& vExtents, ezEnum<ezProcGenBlendMode> blendMode, float fSortOrder, float fValue, const ezVec3& vPositiveFalloff, const ezVec3& vNegativeFalloff, const ezImageDataResourceHandle& hImage = {});

  void AddSpline(const ezSimdTransform& transform, const ezSpline& spline, float fRadius, ezEnum<ezProcGenBlendMode> blendMode, float fSortOrder, float fValue, float fFalloff);

private:
  ezLinearAllocator<ezAllocatorTrackingMode::Basics> m_Allocator;

  ezDynamicArray<const Shape*> m_SortedShapes;
};

struct EZ_PROCGENPLUGIN_DLL ezMsgExtractVolumes : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgExtractVolumes, ezMessage);

  ezVolumeCollection* m_pCollection = nullptr;
};
