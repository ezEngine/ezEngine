#pragma once

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Basics.h>

class ezWorld;
class ezCamera;
class ezGALContext;
class ezRenderPipeline;

/// \brief Encapsulates a view on the given world through the given camera and rendered with the specified RenderPipeline.
class EZ_RENDERERCORE_DLL ezView
{
  EZ_DECLARE_POD_TYPE();

public:
  ezView(const char* szName);

  void SetName(const char* szName);
  const char* GetName() const;
  
  void SetWorld(const ezWorld* pWorld);
  const ezWorld* GetWorld() const;

  void SetRenderPipeline(ezRenderPipeline* pRenderPipeline);
  ezRenderPipeline* GetRenderPipeline() const;

  void SetLogicCamera(const ezCamera* pCamera);
  const ezCamera* GetLogicCamera() const;

  void SetRenderCamera(const ezCamera* pCamera);
  const ezCamera* GetRenderCamera() const;

  /// \brief Returns the start position and direction (in world space) of the picking ray through the screen position in this view.
  ///
  /// fScreenPosX and fScreenPosY are expected to be in [0; 1] range (normalized pixel coordinates).
  /// If no ray can be computed, EZ_FAILURE is returned.
  ezResult ComputePickingRay(float fScreenPosX, float fScreenPosY, ezVec3& out_RayStartPos, ezVec3& out_RayDir);

  bool IsValid() const;
  
  /// \brief Extracts all relevant data from the world to render the view.
  void ExtractData();

  /// \brief Renders the extracted data with the view's pipeline.
  void Render(ezGALContext* pContext);

  /// \brief Returns the current projection matrix.
  const ezMat4& GetProjectionMatrix() const { UpdateCachedMatrices(); return m_ProjectionMatrix; }

  /// \brief Returns the current inverse projection matrix.
  const ezMat4& GetInverseProjectionMatrix() const { UpdateCachedMatrices(); return m_InverseProjectionMatrix; }

  /// \brief Returns the current view matrix (camera orientation).
  const ezMat4& GetViewMatrix() const { UpdateCachedMatrices(); return m_ViewMatrix; }

  /// \brief Returns the current inverse view matrix (inverse camera orientation).
  const ezMat4& GetInverseViewMatrix() const { UpdateCachedMatrices(); return m_InverseViewMatrix; }

  /// \brief Returns the current view-projection matrix.
  const ezMat4& GetViewProjectionMatrix() const { UpdateCachedMatrices(); return m_ViewProjectionMatrix; }

  /// \brief Returns the current inverse view-projection matrix.
  const ezMat4& GetInverseViewProjectionMatrix() const { UpdateCachedMatrices(); return m_InverseViewProjectionMatrix; }

private:
  void UpdateCachedMatrices() const;

  mutable ezUInt32 m_uiLastCameraSettingsModification;
  mutable ezUInt32 m_uiLastCameraOrientationModification;
  mutable float m_fLastViewportAspectRatio;
  mutable ezMat4 m_ViewMatrix;
  mutable ezMat4 m_InverseViewMatrix;
  mutable ezMat4 m_ProjectionMatrix;
  mutable ezMat4 m_InverseProjectionMatrix;
  mutable ezMat4 m_ViewProjectionMatrix;
  mutable ezMat4 m_InverseViewProjectionMatrix;

private:
  ezHashedString m_sName;

  ezProfilingId m_ExtractDataProfilingID;
  ezProfilingId m_RenderProfilingID;

  const ezWorld* m_pWorld; 
  ezRenderPipeline* m_pRenderPipeline;
  const ezCamera* m_pLogicCamera;
  const ezCamera* m_pRenderCamera;
};

#include <RendererCore/Pipeline/Implementation/View_inl.h>
