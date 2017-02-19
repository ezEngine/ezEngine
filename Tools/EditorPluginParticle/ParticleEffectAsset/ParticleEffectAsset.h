#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>
#include <Foundation/Communication/Event.h>

class ezParticleEffectAssetDocument;

struct ezParticleEffectAssetEvent
{
  enum Type
  {
    RestartEffect,
    AutoRestartChanged,
    SimulationSpeedChanged,
  };

  ezParticleEffectAssetDocument* m_pDocument;
  Type m_Type;
};

class ezParticleEffectAssetDocument : public ezSimpleAssetDocument<ezParticleEffectDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEffectAssetDocument, ezSimpleAssetDocument<ezParticleEffectDescriptor>);

public:
  ezParticleEffectAssetDocument(const char* szDocumentPath);

  virtual const char* QueryAssetType() const override { return "Particle Effect"; }

  ezStatus WriteParticleEffectAsset(ezStreamWriter& stream, const char* szPlatform) const;

  void TriggerRestartEffect();

  ezEvent<const ezParticleEffectAssetEvent&> m_Events;

  void SetAutoRestart(bool enable);
  bool GetAutoRestart() const { return m_bAutoRestart; }

  void SetSimulationPaused(bool bPaused);
  bool GetSimulationPaused() const { return m_bSimulationPaused; }

  void SetSimulationSpeed(float speed);
  float GetSimulationSpeed() const { return m_fSimulationSpeed; }

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override;
  virtual ezStatus InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader) override;

private:
  bool m_bSimulationPaused = false;
  bool m_bAutoRestart = true;
  float m_fSimulationSpeed = 1.0f;
};
