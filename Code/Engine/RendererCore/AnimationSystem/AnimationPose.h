#pragma once

#include <RendererCore/AnimationSystem/Declarations.h>

#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Transform.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

class ezSkeleton;

class EZ_RENDERERCORE_DLL ezSkinningSpaceAnimationPose
{
public:
  ezSkinningSpaceAnimationPose();
  ~ezSkinningSpaceAnimationPose();

  void Clear();

  bool IsEmpty() const { return m_Transforms.IsEmpty(); }

  void Configure(ezUInt32 uiNumTransforms);

  ezUInt32 GetTransformCount() const { return m_Transforms.GetCount(); }

  void MapModelSpacePoseToSkinningSpace(const ezHashTable<ezHashedString, ezMeshResourceDescriptor::BoneData>& bones, const ezSkeleton& skeleton, ezArrayPtr<const ezMat4> modelSpaceTransforms);

  // TODO: would be nicer to use ezTransform or ezShaderTransform for this data
  ezDynamicArray<ezMat4, ezAlignedAllocatorWrapper> m_Transforms;
};
