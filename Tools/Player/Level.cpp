#include <Core/ResourceManager/ResourceManager.h>
#include <CoreUtils/Graphics/Camera.h>
#include <Core/WorldSerializer/WorldReader.h>

#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/SimpleRenderPass.h>
#include <RendererCore/RenderLoop/RenderLoop.h>
#include <RendererCore/Textures/TextureResource.h>
#include <RendererCore/Material/MaterialResource.h>

#include <GameUtils/Components/RotorComponent.h>

#include "GameState.h"
#include "Window.h"
#include <Foundation/IO/FileSystem/FileReader.h>


void GameState::CreateGameLevelAndRenderPipeline(ezGALRenderTargetViewHandle hBackBuffer, ezGALRenderTargetViewHandle hDSV, const char* szLevelFile)
{
  EZ_LOG_BLOCK("CreateGameLevelAndRenderPipeline", szLevelFile);

  m_pScene = EZ_DEFAULT_NEW(ezScene);
  m_pScene->Initialize();

  m_pWorld = EZ_DEFAULT_NEW(ezWorld, "Level");
  EZ_LOCK(m_pWorld->GetWriteMarker());

  ezMeshComponentManager* pMeshCompMan = m_pWorld->CreateComponentManager<ezMeshComponentManager>();
  ezRotorComponentManager* pRotorCompMan = m_pWorld->CreateComponentManager<ezRotorComponentManager>();

  {
    ezFileReader file;
    if (file.Open(szLevelFile).Succeeded())
    {
      // File Header
      {
        char szSceneTag[16];
        file.ReadBytes(szSceneTag, sizeof(char) * 16);

        EZ_ASSERT_RELEASE(ezStringUtils::IsEqualN(szSceneTag, "[ezBinaryScene]", 16), "The given file is not a valid scene file");

        ezUInt8 uiVersion = 0;
        file >> uiVersion;

        EZ_ASSERT_RELEASE(uiVersion == 1, "The given scene file has an invalid version: %u", uiVersion);

        ezUInt64 uiHash = 0;
        file >> uiHash;
      }

      ezQuat qRot;
      qRot.SetIdentity();
      //qRot.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(45));

      ezWorldReader reader;
      reader.Read(file, *m_pWorld, ezVec3(0), qRot, ezVec3(1.0f));
    }
    else
    {
      ezLog::Error("Could not read level '%s'", szLevelFile);
    }
  }

  ezVec3 vCameraPos = ezVec3(0.0f, 0.0f, 10.0f);

  ezCoordinateSystem coordSys;
  m_pWorld->GetCoordinateSystem(vCameraPos, coordSys);

  m_Camera.LookAt(vCameraPos, vCameraPos + coordSys.m_vForwardDir, coordSys.m_vUpDir);
  m_Camera.SetCameraMode(ezCamera::PerspectiveFixedFovY, 60.0f, 1.0f, 5000.0f);

  m_pView = ezRenderLoop::CreateView("View");
  ezRenderLoop::AddMainView(m_pView);

  ezGALRenderTagetSetup RTS;
  RTS.SetRenderTarget(0, hBackBuffer)
     .SetDepthStencilTarget(hDSV);

  ezUniquePtr<ezRenderPipeline> pRenderPipeline = EZ_DEFAULT_NEW(ezRenderPipeline);
  pRenderPipeline->AddPass(EZ_DEFAULT_NEW(ezSimpleRenderPass, RTS));
  pRenderPipeline->AddExtractor(EZ_DEFAULT_NEW(ezVisibleObjectsExtractor));
  m_pView->SetRenderPipeline(std::move(pRenderPipeline));

  ezSizeU32 size = m_pWindow->GetClientAreaSize();
  m_pView->SetViewport(ezRectFloat(0.0f, 0.0f, (float)size.width, (float)size.height));

  m_pView->SetWorld(m_pWorld);
  m_pView->SetLogicCamera(&m_Camera);
}

void GameState::DestroyGameLevel()
{
  m_pScene->Deinitialize();

  EZ_DEFAULT_DELETE(m_pWorld);
  EZ_DEFAULT_DELETE(m_pScene);
}


