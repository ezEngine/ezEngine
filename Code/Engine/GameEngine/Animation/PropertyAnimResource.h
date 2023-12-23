#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Tracks/ColorGradient.h>
#include <Foundation/Tracks/Curve1D.h>
#include <Foundation/Tracks/EventTrack.h>
#include <Foundation/Types/SharedPtr.h>
#include <GameEngine/GameEngineDLL.h>

/// \brief What data type an animation modifies.
struct EZ_GAMEENGINE_DLL ezPropertyAnimTarget
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Number,    ///< A single value.
    VectorX,   ///< The x coordinate of a vector.
    VectorY,   ///< The y coordinate of a vector.
    VectorZ,   ///< The z coordinate of a vector.
    VectorW,   ///< The w coordinate of a vector.
    RotationX, ///< The x coordinate of a rotation.
    RotationY, ///< The y coordinate of a rotation.
    RotationZ, ///< The z coordinate of a rotation.
    Color,     ///< A color.

    Default = Number,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezPropertyAnimTarget);

//////////////////////////////////////////////////////////////////////////

/// \brief Describes how an animation should be played back.
struct EZ_GAMEENGINE_DLL ezPropertyAnimMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Once,         ///< Play the animation once from start to end and then stop.
    Loop,         ///< Play the animation from start to end, then loop back to the start and repeat indefinitely.
    BackAndForth, ///< Play the animation from start to end, then reverse direction and play from end to start, then repeat indefinitely.

    Default = Loop,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezPropertyAnimMode);

//////////////////////////////////////////////////////////////////////////

struct EZ_GAMEENGINE_DLL ezPropertyAnimEntry
{
  ezString m_sObjectSearchSequence; ///< Sequence of named objects to search for the target
  ezString m_sComponentType;        ///< Empty to reference the game object properties (position etc.)
  ezString m_sPropertyPath;
  ezEnum<ezPropertyAnimTarget> m_Target;
  const ezRTTI* m_pComponentRtti = nullptr;
};

struct EZ_GAMEENGINE_DLL ezFloatPropertyAnimEntry : public ezPropertyAnimEntry
{
  ezCurve1D m_Curve;
};

struct EZ_GAMEENGINE_DLL ezColorPropertyAnimEntry : public ezPropertyAnimEntry
{
  ezColorGradient m_Gradient;
};

//////////////////////////////////////////////////////////////////////////

// this class is actually ref counted and used with ezSharedPtr to allow to work on the same data, even when the resource was reloaded
struct EZ_GAMEENGINE_DLL ezPropertyAnimResourceDescriptor : public ezRefCounted
{
  ezTime m_AnimationDuration;
  ezDynamicArray<ezFloatPropertyAnimEntry> m_FloatAnimations;
  ezDynamicArray<ezColorPropertyAnimEntry> m_ColorAnimations;
  ezEventTrack m_EventTrack;

  void Save(ezStreamWriter& inout_stream) const;
  void Load(ezStreamReader& inout_stream);
};

//////////////////////////////////////////////////////////////////////////

using ezPropertyAnimResourceHandle = ezTypedResourceHandle<class ezPropertyAnimResource>;

class EZ_GAMEENGINE_DLL ezPropertyAnimResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPropertyAnimResource, ezResource);

  EZ_RESOURCE_DECLARE_COMMON_CODE(ezPropertyAnimResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezPropertyAnimResource, ezPropertyAnimResourceDescriptor);

public:
  ezPropertyAnimResource();

  ezSharedPtr<ezPropertyAnimResourceDescriptor> GetDescriptor() const { return m_pDescriptor; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezSharedPtr<ezPropertyAnimResourceDescriptor> m_pDescriptor;
};
