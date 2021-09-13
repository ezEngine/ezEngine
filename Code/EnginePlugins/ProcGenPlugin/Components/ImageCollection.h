#pragma once

#include <Core/World/World.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Types/TagSet.h>
#include <ProcGenPlugin/Declarations.h>

class EZ_PROCGENPLUGIN_DLL ezImageCollection : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezImageCollection, ezReflectedClass);

public:
  struct Shape
  {
    ezVec4 m_GlobalToLocalTransform0;
    ezVec4 m_GlobalToLocalTransform1;
    ezVec4 m_GlobalToLocalTransform2;
    ezFloat16 m_fValue;
    ezUInt32 m_uiSortingKey;

    EZ_ALWAYS_INLINE bool operator<(const Shape& other) const { return m_uiSortingKey < other.m_uiSortingKey; }

    void SetGlobalToLocalTransform(const ezSimdMat4f& t);
    ezSimdMat4f GetGlobalToLocalTransform() const;
  };

  bool IsEmpty() { return m_Shapes.IsEmpty(); }

  static ezUInt32 ComputeSortingKey(float fSortOrder, float fMaxScale);

  float EvaluateAtGlobalPosition(const ezVec3& vPosition, float fInitialValue = 0.0f) const;

  static void ExtractImagesInBox(const ezWorld& world, const ezBoundingBox& box, ezSpatialData::Category spatialCategory,
    const ezTagSet& includeTags, ezImageCollection& out_Collection, const ezRTTI* pComponentBaseType = nullptr);

  void AddShape(const ezSimdTransform& transform, const ezVec3& vExtents, float fSortOrder, float fValue);

private:
  ezDynamicArray<Shape, ezAlignedAllocatorWrapper> m_Shapes;
};

struct EZ_PROCGENPLUGIN_DLL ezMsgExtractProcImages : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgExtractProcImages, ezMessage);

  ezImageCollection* m_pCollection = nullptr;
};
