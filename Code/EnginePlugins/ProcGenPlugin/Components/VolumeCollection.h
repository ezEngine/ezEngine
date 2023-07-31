#pragma once

#include <Core/World/World.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Types/TagSet.h>
#include <ProcGenPlugin/Declarations.h>

using ezImageDataResourceHandle = ezTypedResourceHandle<class ezImageDataResource>;

class EZ_PROCGENPLUGIN_DLL ezVolumeCollection : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVolumeCollection, ezReflectedClass);

public:
  struct ShapeType
  {
    using StorageType = ezUInt8;

    enum Enum
    {
      Sphere,
      Box,
      Image,

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
    float m_fFadeOutScale;
    float m_fFadeOutBias;
  };

  struct Box : public Shape
  {
    ezVec3 m_vFadeOutScale;
    ezVec3 m_vFadeOutBias;
  };

  struct Image : public Box
  {
    ezImageDataResourceHandle m_Image;
    const ezColor* m_pPixelData = nullptr;
    ezUInt32 m_uiImageWidth = 0;
    ezUInt32 m_uiImageHeight = 0;
  };

  bool IsEmpty() { return m_Spheres.IsEmpty() && m_Boxes.IsEmpty(); }

  float EvaluateAtGlobalPosition(const ezSimdVec4f& vPosition, float fInitialValue, ezProcVolumeImageMode::Enum imgMode, const ezColor& refColor) const;

  static void ExtractVolumesInBox(const ezWorld& world, const ezBoundingBox& box, ezSpatialData::Category spatialCategory, const ezTagSet& includeTags, ezVolumeCollection& out_collection, const ezRTTI* pComponentBaseType = nullptr);

  void AddSphere(const ezSimdTransform& transform, float fRadius, ezEnum<ezProcGenBlendMode> blendMode, float fSortOrder, float fValue, float fFadeOutStart);

  void AddBox(const ezSimdTransform& transform, const ezVec3& vExtents, ezEnum<ezProcGenBlendMode> blendMode, float fSortOrder, float fValue, const ezVec3& vFadeOutStart);

  void AddImage(const ezSimdTransform& transform, const ezVec3& vExtents, ezEnum<ezProcGenBlendMode> blendMode, float fSortOrder, float fValue, const ezVec3& vFadeOutStart, const ezImageDataResourceHandle& hImage);

private:
  ezDynamicArray<Sphere, ezAlignedAllocatorWrapper> m_Spheres;
  ezDynamicArray<Box, ezAlignedAllocatorWrapper> m_Boxes;
  ezDynamicArray<Image, ezAlignedAllocatorWrapper> m_Images;

  ezDynamicArray<const Shape*> m_SortedShapes;
};

struct EZ_PROCGENPLUGIN_DLL ezMsgExtractVolumes : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgExtractVolumes, ezMessage);

  ezVolumeCollection* m_pCollection = nullptr;
};
