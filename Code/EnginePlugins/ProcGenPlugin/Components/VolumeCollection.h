#pragma once

#include <Core/World/World.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Types/TagSet.h>
#include <ProcGenPlugin/Declarations.h>

class EZ_PROCGENPLUGIN_DLL ezVolumeCollection : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVolumeCollection, ezReflectedClass);

public:
  struct ShapeType
  {
    typedef ezUInt8 StorageType;

    enum Enum
    {
      Sphere,
      Box,

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

  bool IsEmpty() { return m_Spheres.IsEmpty() && m_Boxes.IsEmpty(); }

  static ezUInt32 ComputeSortingKey(float fSortOrder, float fMaxScale);

  float EvaluateAtGlobalPosition(const ezVec3& vPosition, float fInitialValue = 0.0f) const;

  static void ExtractVolumesInBox(const ezWorld& world, const ezBoundingBox& box, ezSpatialData::Category spatialCategory,
    const ezTagSet& includeTags, ezVolumeCollection& out_Collection, const ezRTTI* pComponentBaseType = nullptr);

  void AddSphere(const ezSimdTransform& transform, float fRadius, ezEnum<ezProcGenBlendMode> blendMode, float fSortOrder,
    float fValue, float fFadeOutStart);

  void AddBox(const ezSimdTransform& transform, const ezVec3& vExtents, ezEnum<ezProcGenBlendMode> blendMode, float fSortOrder,
    float fValue, const ezVec3& vFadeOutStart);

private:
  ezDynamicArray<Sphere, ezAlignedAllocatorWrapper> m_Spheres;
  ezDynamicArray<Box, ezAlignedAllocatorWrapper> m_Boxes;

  ezDynamicArray<const Shape*> m_SortedShapes;
};

struct EZ_PROCGENPLUGIN_DLL ezMsgExtractVolumes : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgExtractVolumes, ezMessage);

  ezVolumeCollection* m_pCollection = nullptr;
};
