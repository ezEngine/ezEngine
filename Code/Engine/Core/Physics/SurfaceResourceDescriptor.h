#pragma once

#include <Core/CoreDLL.h>

#include <Core/Prefabs/PrefabResource.h>
#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/RangeView.h>
#include <Foundation/Types/Variant.h>

using ezSurfaceResourceHandle = ezTypedResourceHandle<class ezSurfaceResource>;
using ezPrefabResourceHandle = ezTypedResourceHandle<class ezPrefabResource>;


/// \brief Defines how prefabs are aligned when spawned during surface interactions.
struct ezSurfaceInteractionAlignment
{
  using StorageType = ezUInt8;

  enum Enum
  {
    SurfaceNormal,
    IncidentDirection,
    ReflectedDirection,
    ReverseSurfaceNormal,
    ReverseIncidentDirection,
    ReverseReflectedDirection,

    Default = SurfaceNormal
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezSurfaceInteractionAlignment);


/// \brief Describes how a surface responds to a specific type of interaction.
///
/// Configures the prefab to spawn, its alignment, impact thresholds, and custom parameters
/// when objects interact with a surface in a particular way (collision, slide, roll, etc.).
struct EZ_CORE_DLL ezSurfaceInteraction
{
  ezString m_sInteractionType;

  ezPrefabResourceHandle m_hPrefab;
  ezEnum<ezSurfaceInteractionAlignment> m_Alignment;
  ezAngle m_Deviation;
  float m_fImpulseThreshold = 0.0f;
  float m_fImpulseScale = 1.0f;

  const ezRangeView<const char*, ezUInt32> GetParameters() const;   // [ property ] (exposed parameter)
  void SetParameter(const char* szKey, const ezVariant& value);     // [ property ] (exposed parameter)
  void RemoveParameter(const char* szKey);                          // [ property ] (exposed parameter)
  bool GetParameter(const char* szKey, ezVariant& out_value) const; // [ property ] (exposed parameter)

  ezArrayMap<ezHashedString, ezVariant> m_Parameters;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezSurfaceInteraction);

/// \brief Descriptor containing all configuration data for a surface resource.
///
/// Defines physics properties (restitution, friction), interaction behaviors,
/// base surface inheritance, and navigation ground type information.
struct EZ_CORE_DLL ezSurfaceResourceDescriptor : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSurfaceResourceDescriptor, ezReflectedClass);

public:
  void Load(ezStreamReader& inout_stream);
  void Save(ezStreamWriter& inout_stream) const;

  void SetCollisionInteraction(const char* szName);
  const char* GetCollisionInteraction() const;

  void SetSlideReactionPrefabFile(const char* szFile);
  const char* GetSlideReactionPrefabFile() const;

  void SetRollReactionPrefabFile(const char* szFile);
  const char* GetRollReactionPrefabFile() const;

  ezSurfaceResourceHandle m_hBaseSurface;
  float m_fPhysicsRestitution;
  float m_fPhysicsFrictionStatic;
  float m_fPhysicsFrictionDynamic;
  ezHashedString m_sOnCollideInteraction;
  ezHashedString m_sSlideInteractionPrefab;
  ezHashedString m_sRollInteractionPrefab;
  ezInt8 m_iGroundType = -1; ///< What kind of ground this is for navigation purposes. Ground type properties need to be specified elsewhere, this is just a number.

  ezHybridArray<ezSurfaceInteraction, 16> m_Interactions;
};
