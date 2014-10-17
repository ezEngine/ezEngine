#pragma once

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Basics.h>

class ezWorld;
class ezCamera;
class ezRenderPipeline;

/// \brief Encapsulates a view on the given world through the given camera and rendered with the specified RenderPipeline.
class EZ_RENDERERCORE_DLL ezView
{
  EZ_DECLARE_POD_TYPE();

public:
  ezView(const char* szName);

  void SetName(const char* szName);
  const char* GetName() const;
  
  void SetWorld(ezWorld* pWorld);
  ezWorld* GetWorld() const;

  void SetRenderPipeline(ezRenderPipeline* pRenderPipeline);
  ezRenderPipeline* GetRenderPipeline() const;

  void SetLogicCamera(const ezCamera* pCamera);
  const ezCamera* GetLogicCamera() const;

  void SetRenderCamera(const ezCamera* pCamera);
  const ezCamera* GetRenderCamera() const;

  bool IsValid() const;
  
  /// \brief Extracts all relevant data from the world to render the view.
  void ExtractData();

  /// \brief Renders the extracted data with the view's pipeline.
  void Render();

private:
  ezHashedString m_sName;

  ezProfilingId m_ExtractDataProfilingID;
  ezProfilingId m_RenderProfilingID;

  /// \todo the pointer to the world should be const
  ezWorld* m_pWorld; 
  ezRenderPipeline* m_pRenderPipeline;
  const ezCamera* m_pLogicCamera;
  const ezCamera* m_pRenderCamera;
};

#include <RendererCore/Pipeline/Implementation/View_inl.h>
