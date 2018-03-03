#pragma once

#include <GameEngine/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Tracks/Curve1D.h>
#include <Foundation/Tracks/ColorGradient.h>
#include <Foundation/Tracks/EventTrack.h>

struct EZ_GAMEENGINE_DLL ezPropertyAnimTarget
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    Number,
    VectorX,
    VectorY,
    VectorZ,
    VectorW,
    RotationX,
    RotationY,
    RotationZ,
    Color,

    Default = Number,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezPropertyAnimTarget);

//////////////////////////////////////////////////////////////////////////

struct EZ_GAMEENGINE_DLL ezPropertyAnimMode
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    Once,
    Loop,
    BackAndForth,

    Default = Loop,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezPropertyAnimMode);

//////////////////////////////////////////////////////////////////////////

struct EZ_GAMEENGINE_DLL ezPropertyAnimEntry
{
  ezString m_sObjectSearchSequence; ///< Sequence of named objects to search for the target
  ezString m_sComponentType; ///< Empty to reference the game object properties (position etc.)
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

  void Save(ezStreamWriter& stream) const;
  void Load(ezStreamReader& stream);
};

//////////////////////////////////////////////////////////////////////////

typedef ezTypedResourceHandle<class ezPropertyAnimResource> ezPropertyAnimResourceHandle;

class EZ_GAMEENGINE_DLL ezPropertyAnimResource : public ezResource<ezPropertyAnimResource, ezPropertyAnimResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPropertyAnimResource, ezResourceBase);

public:
  ezPropertyAnimResource();

  ezSharedPtr<ezPropertyAnimResourceDescriptor> GetDescriptor() const { return m_pDescriptor; }

private:
  virtual ezResourceLoadDesc CreateResource(const ezPropertyAnimResourceDescriptor& descriptor) override;
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezSharedPtr<ezPropertyAnimResourceDescriptor> m_pDescriptor;
};


