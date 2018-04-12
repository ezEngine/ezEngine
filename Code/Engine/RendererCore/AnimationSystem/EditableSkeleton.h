
#pragma once

#include <RendererCore/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>

class ezEditableSkeletonBone : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditableSkeletonBone, ezReflectedClass);

public:
  ezEditableSkeletonBone();
  ~ezEditableSkeletonBone();

  const char* GetName() const;
  void SetName(const char* sz);

  ezHashedString m_sName;

  ezHybridArray<ezEditableSkeletonBone*, 4> m_Children;
};

class EZ_RENDERERCORE_DLL ezEditableSkeleton : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditableSkeleton, ezReflectedClass );

public:
  ezEditableSkeleton();
  ~ezEditableSkeleton();

  ezString m_sAnimationFile;

  ezEnum<ezBasisAxis> m_ForwardDir;
  ezEnum<ezBasisAxis> m_RightDir;
  ezEnum<ezBasisAxis> m_UpDir;

  ezHybridArray<ezEditableSkeletonBone*, 4> m_Children;

};

