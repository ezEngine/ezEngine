#pragma once

#include <GameEngine/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <Foundation/Types/SharedPtr.h>

typedef ezTypedResourceHandle<class ezCurve1DResource> ezCurve1DResourceHandle;
typedef ezTypedResourceHandle<class ezColorGradientResource> ezColorGradientResourceHandle;

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
    Color,

    Default = Number,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezPropertyAnimTarget);


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


struct EZ_GAMEENGINE_DLL ezPropertyAnimEntry
{
  ezString m_sPropertyName;
  ezTime m_Duration;
  ezEnum<ezPropertyAnimMode> m_Mode;
  ezEnum<ezPropertyAnimTarget> m_Target;
  ezCurve1DResourceHandle m_hNumberCurve;
  ezColorGradientResourceHandle m_hColorCurve;

  void SetNumberCurveFile(const char* szFile);
  const char* GetNumberCurveFile() const;

  void SetColorCurveFile(const char* szFile);
  const char* GetColorCurveFile() const;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezPropertyAnimEntry);


// this class is actually ref counted and used with ezSharedPtr to allow to work on the same data, even when the resource was reloaded
struct EZ_GAMEENGINE_DLL ezPropertyAnimResourceDescriptor : public ezRefCounted
{
  ezDynamicArray<ezPropertyAnimEntry> m_Animations;

  void Save(ezStreamWriter& stream) const;
  void Load(ezStreamReader& stream);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezPropertyAnimResourceDescriptor);



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


