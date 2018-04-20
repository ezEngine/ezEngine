#pragma once

#include <RendererCore/Basics.h>

class ezSkeleton;
struct ezSkeletonResourceDescriptor;
class ezEditableSkeletonBone;

struct ezSkeletonBoneGeometryType
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    None = 0,
    Capsule,
    Sphere,
    Box,

    Default = None
  };
};

