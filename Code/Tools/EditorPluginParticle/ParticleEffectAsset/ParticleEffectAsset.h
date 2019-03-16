#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <Foundation/Communication/Event.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>

class ezParticleEffectAssetDocument;
struct ezPropertyMetaStateEvent;

struct ezParticleEffectAssetEvent
{
  enum Type
  {
    RestartEffect,
    AutoRestartChanged,
    SimulationSpeedChanged,
    RenderVisualizersChanged,
  };

  ezParticleEffectAssetDocument* m_pDocument;
  Type m_Type;
};

class ezParticleEffectAssetDocument : public ezSimpleAssetDocument<ezParticleEffectDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEffectAssetDocument, ezSimpleAssetDocument<ezParticleEffectDescriptor>);

public:
  ezParticleEffectAssetDocument(const char* szDocumentPath);

  static void PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);

  virtual const char* QueryAssetType() const override { return "Particle Effect"; }

  ezStatus WriteParticleEffectAsset(ezStreamWriter& stream, const ezPlatformProfile* pAssetProfile) const;

  void TriggerRestartEffect();

  ezEvent<const ezParticleEffectAssetEvent&> m_Events;

  void SetAutoRestart(bool enable);
  bool GetAutoRestart() const { return m_bAutoRestart; }

  void SetSimulationPaused(bool bPaused);
  bool GetSimulationPaused() const { return m_bSimulationPaused; }

  void SetSimulationSpeed(float speed);
  float GetSimulationSpeed() const { return m_fSimulationSpeed; }

  bool GetRenderVisualizers() const { return m_bRenderVisualizers; }
  void SetRenderVisualizers(bool b);

  // Overridden to enable support for visualizers/manipulators
  virtual ezResult ComputeObjectTransformation(const ezDocumentObject* pObject, ezTransform& out_Result) const override;

protected:
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
                                          const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override;
  virtual ezStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

private:
  bool m_bSimulationPaused = false;
  bool m_bAutoRestart = true;
  bool m_bRenderVisualizers = false;
  float m_fSimulationSpeed = 1.0f;
};
