#include <PCH.h>
#include "Basics.h"
#include <CoreUtils/Graphics/Camera.h>

ezResult ezRendererTestBasics::InitializeSubTest(ezInt32 iIdentifier)
{
  m_iFrame = -1;

  if (ezGraphicsTest::InitializeSubTest(iIdentifier).Failed())
    return EZ_FAILURE;

  if (iIdentifier == SubTests::ST_ClearScreen)
  {
    return SetupRenderer(320, 240);
  }
  else
  {
    if (SetupRenderer().Failed())
      return EZ_FAILURE;

    m_hSphere = CreateSphere(3, 1.0f);
    m_hSphere2 = CreateSphere(1, 0.75f);
    m_hTorus = CreateTorus(16, 0.5f, 0.75f);
    m_hLongBox = CreateBox(0.4f, 0.2f, 2.0f);

    return EZ_SUCCESS;
  }

  return EZ_SUCCESS;
}

ezResult ezRendererTestBasics::DeInitializeSubTest(ezInt32 iIdentifier)
{
  m_hSphere.Invalidate();
  m_hSphere2.Invalidate();
  m_hTorus.Invalidate();
  m_hLongBox.Invalidate();
  m_hTexture.Invalidate();

  ShutdownRenderer();

  if (ezGraphicsTest::DeInitializeSubTest(iIdentifier).Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}


ezTestAppRun ezRendererTestBasics::SubtestClearScreen()
{
  BeginFrame();

  switch (m_iFrame)
  {
  case 0:
    ClearScreen(ezColor(1,0,0));
    break;
  case 1:
    ClearScreen(ezColor(0,1,0));
    break;
  case 2:
    ClearScreen(ezColor(0,0,1));
    break;
  case 3:
    ClearScreen(ezColor(0.5f, 0.5f, 0.5f, 0.5f));
    break;
  }

  EZ_TEST_IMAGE(1);

  EndFrame();

  return m_iFrame < 3 ? ezTestAppRun::Continue : ezTestAppRun::Quit;
}

void ezRendererTestBasics::RenderObjects(ezBitflags<ezShaderBindFlags> ShaderBindFlags)
{
  ezCamera cam;
  cam.SetCameraMode(ezCamera::PerspectiveFixedFovX, 90, 0.5f, 1000.0f);
  cam.LookAt(ezVec3(0, 0, 0), ezVec3(0, 0, -1), ezVec3(0, 1, 0));
  ezMat4 mProj, mView;
  cam.GetProjectionMatrix((float) GetResolution().width / (float) GetResolution().height, mProj);
  cam.GetViewMatrix(mView);

  ezMat4 mTransform, mOther;

  mOther.SetScalingMatrix(ezVec3(1.0f, 1.0f, 1.0f));
  mTransform.SetTranslationMatrix(ezVec3( -0.3f, -0.3f, 0.0f));
  RenderObject(m_hLongBox, mProj * mView * mTransform * mOther, ezColor(1, 0, 1, 0.25f), ShaderBindFlags);

  mOther.SetRotationMatrixX(ezAngle::Degree(80.0f));
  mTransform.SetTranslationMatrix(ezVec3(0.75f, 0, -1.8f));
  RenderObject(m_hTorus, mProj * mView * mTransform * mOther, ezColor(1, 0, 0, 0.5f), ShaderBindFlags);

  mOther.SetIdentity();
  mTransform.SetTranslationMatrix(ezVec3( 0, 0.1f, -2.0f));
  RenderObject(m_hSphere, mProj * mView * mTransform * mOther, ezColor(0, 1, 0, 0.75f), ShaderBindFlags);

  mOther.SetScalingMatrix(ezVec3(1.5f, 1.0f, 1.0f));
  mTransform.SetTranslationMatrix(ezVec3( -0.6f, -0.2f, -2.2f));
  RenderObject(m_hSphere2, mProj * mView * mTransform * mOther, ezColor(0, 0, 1, 1), ShaderBindFlags);
}

static ezRendererTestBasics g_Test;

