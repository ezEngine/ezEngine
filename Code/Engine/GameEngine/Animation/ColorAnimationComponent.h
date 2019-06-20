#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <GameEngine/Curves/ColorGradientResource.h>

typedef ezComponentManagerSimple<class ezColorAnimationComponent, ezComponentUpdateType::WhenSimulating> ezColorAnimationComponentManager;

class EZ_GAMEENGINE_DLL ezColorAnimationComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezColorAnimationComponent, ezComponent, ezColorAnimationComponentManager);

public:
  ezColorAnimationComponent();

  void Update();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************

  void SetColorGradientFile(const char* szFile);
  const char* GetColorGradientFile() const;

  void SetColorGradient(const ezColorGradientResourceHandle& hResource);
  EZ_ALWAYS_INLINE const ezColorGradientResourceHandle& GetColorGradient() const { return m_hGradient; }

protected:
  ezTime m_CurAnimTime;
  ezTime m_Duration;

  ezColorGradientResourceHandle m_hGradient;
};

