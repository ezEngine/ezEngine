#include <GameEngineTest/GameEngineTestPCH.h>

#include <GameEngineTest/DebugRenderingTest/DebugRenderingTest.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>

static ezGameEngineTestDebugRendering s_GameEngineTestDebugRendering;

const char* ezGameEngineTestDebugRendering::GetTestName() const
{
  return "Debug Rendering";
}

ezGameEngineTestApplication* ezGameEngineTestDebugRendering::CreateApplication()
{
  m_pOwnApplication = EZ_DEFAULT_NEW(ezGameEngineTestApplication_DebugRendering);
  return m_pOwnApplication;
}

void ezGameEngineTestDebugRendering::SetupSubTests()
{
  AddSubTest("Basics", SubTests::Basics);
  AddSubTest("Text", SubTests::Text);
  AddSubTest("Thick Lines", SubTests::ThickLines);
  AddSubTest("Shapes", SubTests::Shapes);
}

ezResult ezGameEngineTestDebugRendering::InitializeSubTest(ezInt32 iIdentifier)
{
  EZ_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;
  m_pOwnApplication->SubTestSetup();
  return EZ_SUCCESS;
}

ezTestAppRun ezGameEngineTestDebugRendering::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  ++m_iFrame;

  switch (iIdentifier)
  {
    case SubTests::Basics:
      return m_pOwnApplication->SubTestBasicsExec(m_iFrame);
    case SubTests::Text:
      return m_pOwnApplication->SubTestTextExec(m_iFrame);
    case SubTests::ThickLines:
      return m_pOwnApplication->SubTestThickLinesExec(m_iFrame);
    case SubTests::Shapes:
      return m_pOwnApplication->SubTestShapesExec(m_iFrame);
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
  return ezTestAppRun::Quit;
}

//////////////////////////////////////////////////////////////////////////

ezGameEngineTestApplication_DebugRendering::ezGameEngineTestApplication_DebugRendering()
  : ezGameEngineTestApplication("Basics")
{
}

void ezGameEngineTestApplication_DebugRendering::SubTestSetup()
{
  EZ_LOCK(m_pWorld->GetWriteMarker());

  m_pWorld->Clear();

  ezRenderWorld::ResetFrameCounter();
}

ezTestAppRun ezGameEngineTestApplication_DebugRendering::SubTestBasicsExec(ezInt32 iCurFrame)
{
  {
    auto pCamera = ezDynamicCast<ezGameState*>(GetActiveGameState())->GetMainCamera();
    pCamera->SetCameraMode(ezCameraMode::PerspectiveFixedFovY, 100.0f, 0.1f, 1000.0f);
    ezVec3 pos;
    pos.SetZero();
    pCamera->LookAt(pos, pos + ezVec3(1, 0, 0), ezVec3(0, 0, 1));
  }

  // line box
  {
    ezBoundingBox bbox = ezBoundingBox::MakeFromCenterAndHalfExtents(ezVec3(10, -5, 1), ezVec3(1, 2, 3));

    ezTransform t;
    t.SetIdentity();
    t.m_qRotation = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::MakeFromDegree(25));
    ezDebugRenderer::DrawLineBox(m_pWorld.Borrow(), bbox, ezColor::HotPink, 0.0f, t);
  }

  // line box
  {
    ezBoundingBox bbox = ezBoundingBox::MakeFromCenterAndHalfExtents(ezVec3(10, -3, 1), ezVec3(1, 2, 3));

    ezTransform t;
    t.SetIdentity();
    t.m_vPosition.Set(0, 5, -2);
    t.m_qRotation = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::MakeFromDegree(25));
    ezDebugRenderer::DrawLineBoxCorners(m_pWorld.Borrow(), bbox, 0.5f, ezColor::DeepPink, 0.0f, t);
  }

  // 2D Rect
  {
    ezDebugRenderer::Draw2DRectangle(m_pWorld.Borrow(), ezRectFloat(10, 50, 35, 15), 0.1f, ezColor::LawnGreen);
  }

  // Sphere
  {
    ezBoundingSphere sphere = ezBoundingSphere::MakeFromCenterAndRadius(ezVec3(8, -5, -4), 2);
    ezDebugRenderer::DrawLineSphere(m_pWorld.Borrow(), sphere, ezColor::Tomato);
  }

  // Solid box
  {
    ezBoundingBox bbox = ezBoundingBox::MakeFromCenterAndHalfExtents(ezVec3(10, -5, 1), ezVec3(1, 2, 3));

    ezDebugRenderer::DrawSolidBox(m_pWorld.Borrow(), bbox, ezColor::BurlyWood);
  }

  // Text
  {
    ezDebugRenderer::Draw2DText(m_pWorld.Borrow(), "Not 'a test\"", ezVec2I32(30, 10), ezColor::AntiqueWhite, 24);
    ezDebugRenderer::Draw2DText(m_pWorld.Borrow(), "!@#$%^&*()_[]{}|", ezVec2I32(20, 200), ezColor::AntiqueWhite, 24);
  }

  // Frustum
  {
    ezFrustum f = ezFrustum::MakeFromFOV(ezVec3(5, 7, 3), ezVec3(0, -1, 0), ezVec3(0, 0, 1), ezAngle::MakeFromDegree(30), ezAngle::MakeFromDegree(20), 0.1f, 5.0f);
    ezDebugRenderer::DrawLineFrustum(m_pWorld.Borrow(), f, ezColor::Cornsilk);
  }

  // Lines
  {
    ezTempHybridArray<ezDebugRendererLine, 4> lines;
    lines.PushBack(ezDebugRendererLine(ezVec3(3, -4, -4), ezVec3(4, -2, -3)));
    lines.PushBack(ezDebugRendererLine(ezVec3(4, -2, -3), ezVec3(2, 2, -2)));
    ezDebugRenderer::DrawLines(m_pWorld.Borrow(), lines, ezColor::SkyBlue);
  }

  // Triangles
  {
    ezTempHybridArray<ezDebugRendererTriangle, 4> tris;
    tris.PushBack(ezDebugRendererTriangle(ezVec3(7, 0, 0), ezVec3(7, 2, 0), ezVec3(7, 2, 1)));
    tris.PushBack(ezDebugRendererTriangle(ezVec3(7, 3, 0), ezVec3(7, 1, 0), ezVec3(7, 3, 1)));
    ezDebugRenderer::DrawSolidTriangles(m_pWorld.Borrow(), tris, ezColor::Gainsboro);
  }

  Run();
  if (ShouldApplicationQuit())
    return ezTestAppRun::Quit;

  // first frame no image is captured yet
  if (iCurFrame < 1)
    return ezTestAppRun::Continue;

  ezStringView sRendererName = ezGALDevice::GetDefaultDevice()->GetRenderer();
  const bool bRandomlyChangesLineThicknessOnDriverUpdate = sRendererName.IsEqual_NoCase("DX11") && ezGALDevice::GetDefaultDevice()->GetCapabilities().m_sAdapterName.FindSubString_NoCase("Nvidia");

  EZ_TEST_LINE_IMAGE(0, bRandomlyChangesLineThicknessOnDriverUpdate ? 700 : 150);

  return ezTestAppRun::Quit;
}

ezTestAppRun ezGameEngineTestApplication_DebugRendering::SubTestTextExec(ezInt32 iCurFrame)
{
  {
    auto pCamera = ezDynamicCast<ezGameState*>(GetActiveGameState())->GetMainCamera();
    pCamera->SetCameraMode(ezCameraMode::PerspectiveFixedFovY, 100.0f, 0.1f, 1000.0f);
    ezVec3 pos;
    pos.SetZero();
    pCamera->LookAt(pos, pos + ezVec3(1, 0, 0), ezVec3(0, 0, 1));
  }

  // Text
  {
    ezDebugRenderer::Draw2DText(m_pWorld.Borrow(), ezFmt("Frame# {}", ezRenderWorld::GetFrameCounter()), ezVec2I32(10, 10), ezColor::AntiqueWhite, 24);
    ezDebugRenderer::DrawInfoText(m_pWorld.Borrow(), ezDebugTextPlacement::BottomLeft, "test", ezFmt("Frame# {}", ezRenderWorld::GetFrameCounter()));
    ezDebugRenderer::DrawInfoText(m_pWorld.Borrow(), ezDebugTextPlacement::BottomRight, "test", "| Col 1\t| Col 2\t| Col 3\t|\n| abc\t| 42\t| 11.23\t|");
  }

  Run();
  if (ShouldApplicationQuit())
    return ezTestAppRun::Quit;

  // first frame no image is captured yet
  if (iCurFrame < 1)
    return ezTestAppRun::Continue;

  EZ_TEST_IMAGE(0, 150);

  return ezTestAppRun::Quit;
}

ezTestAppRun ezGameEngineTestApplication_DebugRendering::SubTestThickLinesExec(ezInt32 iCurFrame)
{
  {
    auto pCamera = ezDynamicCast<ezGameState*>(GetActiveGameState())->GetMainCamera();
    pCamera->SetCameraMode(ezCameraMode::PerspectiveFixedFovY, 100.0f, 0.1f, 1000.0f);
    pCamera->LookAt(ezVec3::MakeZero(), ezVec3(1, 0, 0), ezVec3(0, 0, 1));
  }

  // Line strip with sharp corners: the joints must have no gaps and no visible overlap.
  {
    ezTempHybridArray<ezDebugRendererLine, 16> lines;
    ezVec3 vPrev(10, 9, 6);
    for (ezUInt32 i = 1; i <= 8; ++i)
    {
      const ezVec3 vNext(10, 9.0f - i * 2.0f, (i % 2 == 0) ? 6.0f : 8.0f);
      lines.PushBack(ezDebugRendererLine(vPrev, vNext));
      vPrev = vNext;
    }

    ezDebugRenderer::DrawLines(m_pWorld.Borrow(), lines, ezColor::Orange, 3.0f);
  }

  // Different thicknesses, including one below a pixel.
  {
    const float thicknesses[] = {0.5f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 6.0f};

    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(thicknesses); ++i)
    {
      const float z = 4.0f - i * 1.5f;
      ezDebugRendererLine line(ezVec3(10, 9, z), ezVec3(10, 1, z - 1.0f));
      ezDebugRenderer::DrawLines(m_pWorld.Borrow(), ezMakeArrayPtr(&line, 1), ezColor::SkyBlue, thicknesses[i]);
    }
  }

  // Color gradient along the line.
  {
    ezDebugRendererLine line(ezVec3(10, -1, 4), ezVec3(10, -8, 3));
    line.m_startColor = ezColor::Red;
    line.m_endColor = ezColor::Yellow;
    ezDebugRenderer::DrawLines(m_pWorld.Borrow(), ezMakeArrayPtr(&line, 1), ezColor::White, 5.0f);
  }

  // Depth test: the line goes through a solid box, the middle part must be hidden.
  {
    ezBoundingBox bbox = ezBoundingBox::MakeFromCenterAndHalfExtents(ezVec3(10, -5, 0), ezVec3(1, 1, 1));
    ezDebugRenderer::DrawSolidBox(m_pWorld.Borrow(), bbox, ezColor::BurlyWood);

    ezDebugRendererLine line(ezVec3(10, -1.5f, 0), ezVec3(10, -8.5f, 0.5f));
    ezDebugRenderer::DrawLines(m_pWorld.Borrow(), ezMakeArrayPtr(&line, 1), ezColor::LawnGreen, 3.0f);
  }

  // Line that starts behind the camera: must be clipped at the near plane, not flip or explode.
  {
    ezDebugRendererLine line(ezVec3(-5, -3, -4), ezVec3(15, -8, -6));
    ezDebugRenderer::DrawLines(m_pWorld.Borrow(), ezMakeArrayPtr(&line, 1), ezColor::HotPink, 2.0f);
  }

  // Line that is completely behind the camera: must not be visible.
  {
    ezDebugRendererLine line(ezVec3(-5, -5, 0), ezVec3(-5, 5, 0));
    ezDebugRenderer::DrawLines(m_pWorld.Borrow(), ezMakeArrayPtr(&line, 1), ezColor::Red, 12.0f);
  }

  // Zero length line: renders as a dot.
  {
    ezDebugRendererLine line(ezVec3(10, 6, -6), ezVec3(10, 6, -6));
    ezDebugRenderer::DrawLines(m_pWorld.Borrow(), ezMakeArrayPtr(&line, 1), ezColor::Cornsilk, 8.0f);
  }

  // Transform: a square, rotated and moved.
  {
    ezDebugRendererLine lines[4] = {
      ezDebugRendererLine(ezVec3(0, -1, -1), ezVec3(0, 1, -1)),
      ezDebugRendererLine(ezVec3(0, 1, -1), ezVec3(0, 1, 1)),
      ezDebugRendererLine(ezVec3(0, 1, 1), ezVec3(0, -1, 1)),
      ezDebugRendererLine(ezVec3(0, -1, 1), ezVec3(0, -1, -1)),
    };

    ezTransform t = ezTransform::MakeIdentity();
    t.m_vPosition.Set(10, 4, -6);
    t.m_qRotation = ezQuat::MakeFromAxisAndAngle(ezVec3(1, 0, 0), ezAngle::MakeFromDegree(30));
    ezDebugRenderer::DrawLines(m_pWorld.Borrow(), ezMakeArrayPtr(lines), ezColor::MediumPurple, 2.0f, t);
  }

  // Curve: a closed circle from many short segments, the joints must not be visible.
  {
    constexpr ezUInt32 uiNumSegments = 40;
    ezTempHybridArray<ezDebugRendererLine, uiNumSegments> lines;

    for (ezUInt32 i = 0; i < uiNumSegments; ++i)
    {
      const ezAngle a0 = ezAngle::MakeFromDegree(360.0f * i / uiNumSegments);
      const ezAngle a1 = ezAngle::MakeFromDegree(360.0f * (i + 1) / uiNumSegments);
      lines.PushBack(ezDebugRendererLine(ezVec3(10, 12.5f + 2.5f * ezMath::Cos(a0), 5 + 2.5f * ezMath::Sin(a0)), ezVec3(10, 12.5f + 2.5f * ezMath::Cos(a1), 5 + 2.5f * ezMath::Sin(a1))));
    }

    ezDebugRenderer::DrawLines(m_pWorld.Borrow(), lines, ezColor::Aquamarine, 2.0f);
  }

  // Color gradient across multiple joints: each segment continues the color of the previous one.
  {
    constexpr ezUInt32 uiNumSegments = 12;
    ezTempHybridArray<ezDebugRendererLine, uiNumSegments> lines;

    auto GetPoint = [](ezUInt32 i)
    { return ezVec3(10, 12.5f + 1.5f * ezMath::Sin(ezAngle::MakeFromDegree(90.0f * i)), 0.5f - i * 0.8f); };

    auto GetColor = [](ezUInt32 i)
    { return ezColor::MakeHSV(300.0f * i / uiNumSegments, 1.0f, 1.0f); };

    for (ezUInt32 i = 0; i < uiNumSegments; ++i)
    {
      ezDebugRendererLine& line = lines.ExpandAndGetRef();
      line.m_start = GetPoint(i);
      line.m_end = GetPoint(i + 1);
      line.m_startColor = GetColor(i);
      line.m_endColor = GetColor(i + 1);
    }

    ezDebugRenderer::DrawLines(m_pWorld.Borrow(), lines, ezColor::White, 4.0f);
  }

  // Occluded thick lines: the part inside the box is rendered by the occluded pass, the rest by the depth tested pass.
  {
    ezDebugRendererLine line(ezVec3(10, -1.5f, -0.6f), ezVec3(10, -8.5f, -0.6f));
    ezDebugRenderer::DrawLinesOccluded(m_pWorld.Borrow(), ezMakeArrayPtr(&line, 1), ezColor::Orange, 3.0f);
    ezDebugRenderer::DrawLines(m_pWorld.Borrow(), ezMakeArrayPtr(&line, 1), ezColor::RoyalBlue, 3.0f);
  }

  // Transparent thick line in front of the box.
  {
    ezDebugRendererLine line(ezVec3(8.5f, -3, 0.4f), ezVec3(8.5f, -7, 0.4f));
    ezDebugRenderer::DrawLines(m_pWorld.Borrow(), ezMakeArrayPtr(&line, 1), ezColor(1, 1, 1, 0.5f), 6.0f);
  }

  // Shapes with thick lines.
  {
    ezDebugRenderer::DrawLineBox(m_pWorld.Borrow(), ezBoundingBox::MakeFromCenterAndHalfExtents(ezVec3(10, -11, 4), ezVec3(1.5f)), ezColor::Yellow, 2.0f);
    ezDebugRenderer::DrawLineSphere(m_pWorld.Borrow(), ezBoundingSphere::MakeFromCenterAndRadius(ezVec3(10, -11, -1.5f), 1.5f), ezColor::Coral, 2.0f);
    ezDebugRenderer::DrawArrow(m_pWorld.Borrow(), 3.0f, ezColor::Cyan, 2.0f, ezTransform(ezVec3(10, -9.5f, -6)), ezVec3(0, -1, 0));
  }

  Run();
  if (ShouldApplicationQuit())
    return ezTestAppRun::Quit;

  // first frame no image is captured yet
  if (iCurFrame < 1)
    return ezTestAppRun::Continue;

  EZ_TEST_IMAGE(0, 150);

  return ezTestAppRun::Quit;
}

ezTestAppRun ezGameEngineTestApplication_DebugRendering::SubTestShapesExec(ezInt32 iCurFrame)
{
  {
    auto pCamera = ezDynamicCast<ezGameState*>(GetActiveGameState())->GetMainCamera();
    pCamera->SetCameraMode(ezCameraMode::PerspectiveFixedFovY, 100.0f, 0.1f, 1000.0f);
    pCamera->LookAt(ezVec3::MakeZero(), ezVec3(1, 0, 0), ezVec3(0, 0, 1));
  }

  // The shapes are placed in a 4x3 grid on the plane x = 10. +Y is to the right, +Z is up.
  // Most shapes are tilted, so that they are not seen exactly from the front or side.
  const float fColumns[] = {-11.0f, -4.0f, 3.0f, 10.0f};
  const float fRows[] = {6.0f, 0.0f, -6.0f};
  auto CellPos = [&](ezUInt32 uiColumn, ezUInt32 uiRow)
  { return ezVec3(10.0f, fColumns[uiColumn], fRows[uiRow]); };

  const ezQuat qTilt = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::MakeFromDegree(30)) * ezQuat::MakeFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::MakeFromDegree(-25));

  // Cross: the center is transformed as well.
  {
    ezTransform t(CellPos(0, 0), qTilt);
    ezDebugRenderer::DrawCross(m_pWorld.Borrow(), ezVec3(0, 0, 0.5f), 3.0f, ezColor::Red, 0.0f, t);
  }

  // Wireframe capsule.
  {
    ezDebugRenderer::DrawLineCapsuleZ(m_pWorld.Borrow(), 2.0f, 1.0f, ezColor::LawnGreen, 0.0f, ezTransform(CellPos(1, 0), qTilt));
  }

  // Wireframe cylinder.
  {
    ezDebugRenderer::DrawLineCylinderZ(m_pWorld.Borrow(), 3.0f, 1.2f, ezColor::Gold, 0.0f, ezTransform(CellPos(2, 0), qTilt));
  }

  // Solid cone with outline and both caps, starting at the given position.
  {
    ezDebugRenderer::DrawCylinder(m_pWorld.Borrow(), 1.5f, 0.5f, 3.0f, ezColor::SteelBlue, ezColor::White, 0.0f, ezTransform(CellPos(3, 0) - ezVec3(0, 0, 1.5f), qTilt), true, true, ezBasisAxis::PositiveZ);
  }

  // Solid angle with outline, in the plane facing the camera, from -60 to +90 degrees around the up direction.
  {
    ezTransform t(CellPos(0, 1), ezQuat::MakeIdentity(), ezVec3(2.5f));
    ezDebugRenderer::DrawAngle(m_pWorld.Borrow(), ezAngle::MakeFromDegree(-60), ezAngle::MakeFromDegree(90), ezColor::DodgerBlue.WithAlpha(0.6f), ezColor::White, 0.0f, t, ezVec3(0, 0, 1), ezVec3(-1, 0, 0));
  }

  // Opening cone with different colors for the inside and outside, opening towards the camera and up.
  {
    ezTransform t(CellPos(1, 1) - ezVec3(0, 0, 1.5f), ezQuat::MakeShortestRotation(ezVec3(1, 0, 0), ezVec3(-0.5f, -0.4f, 1).GetNormalized()), ezVec3(2.5f));
    ezDebugRenderer::DrawOpeningCone(m_pWorld.Borrow(), ezAngle::MakeFromDegree(40), ezColor::Orange, ezColor::MediumPurple, t);
  }

  // Limit cone with different angles along Y and Z, solid and as lines.
  {
    ezTransform t(CellPos(2, 1) - ezVec3(0, 0, 1.5f), ezQuat::MakeShortestRotation(ezVec3(1, 0, 0), ezVec3(-0.3f, 0.3f, 1).GetNormalized()), ezVec3(3.0f));
    ezDebugRenderer::DrawLimitCone(m_pWorld.Borrow(), ezAngle::MakeFromDegree(20), ezAngle::MakeFromDegree(50), ezColor::Teal.WithAlpha(0.6f), ezColor::Aquamarine, 0.0f, t);
  }

  // Wireframe frustum with plane normals.
  {
    ezFrustum f = ezFrustum::MakeFromFOV(CellPos(3, 1) - ezVec3(-1, 2.0f, 0.5f), ezVec3(-0.8f, 1, 0.3f).GetNormalized(), ezVec3(0, 0, 1), ezAngle::MakeFromDegree(50), ezAngle::MakeFromDegree(35), 0.5f, 3.0f);
    ezDebugRenderer::DrawLineFrustum(m_pWorld.Borrow(), f, ezColor::Cornsilk, 0.0f, true);
  }

  // Thin line arrows along different axes.
  {
    ezTransform t(CellPos(0, 2), qTilt);
    ezDebugRenderer::DrawArrow(m_pWorld.Borrow(), 2.5f, ezColor::Red, 0.0f, t, ezVec3(1, 0, 0));
    ezDebugRenderer::DrawArrow(m_pWorld.Borrow(), 2.5f, ezColor::Lime, 0.0f, t, ezVec3(0, 1, 0));
    ezDebugRenderer::DrawArrow(m_pWorld.Borrow(), 2.5f, ezColor::Blue, 0.0f, t, ezVec3(0, 0, 1));
  }

  // Wireframe cylinder along a negative axis, without solid part.
  {
    ezDebugRenderer::DrawCylinder(m_pWorld.Borrow(), 0.5f, 1.5f, 3.0f, ezColor::MakeZero(), ezColor::Coral, 0.0f, ezTransform(CellPos(1, 2) + ezVec3(0, 1.5f, 0), qTilt), false, false, ezBasisAxis::NegativeY);
  }

  // Solid triangles with both windings. Top row is single-sided, so one of the two is culled, bottom row is two-sided.
  {
    const ezVec3 c = CellPos(2, 2);

    ezDebugRendererTriangle tris[2];
    tris[0] = ezDebugRendererTriangle(c + ezVec3(0, -2.0f, 0.3f), c + ezVec3(0, -0.2f, 0.3f), c + ezVec3(0, -1.1f, 2.0f));
    tris[1] = ezDebugRendererTriangle(c + ezVec3(0, 0.2f, 0.3f), c + ezVec3(0, 1.1f, 2.0f), c + ezVec3(0, 2.0f, 0.3f));
    ezDebugRenderer::DrawSolidTriangles(m_pWorld.Borrow(), ezMakeArrayPtr(tris), ezColor::Gainsboro, false);

    for (auto& tri : tris)
    {
      for (auto& pos : tri.m_position)
        pos.z -= 2.3f;
    }

    tris[0].m_color = ezColor::Yellow;
    tris[1].m_color = ezColor::Cyan;
    ezDebugRenderer::DrawSolidTriangles(m_pWorld.Borrow(), ezMakeArrayPtr(tris), ezColor::White, true);
  }

  // 3D text with different alignments, the crosses mark the anchor positions.
  {
    const ezVec3 c = CellPos(3, 2);
    const ezVec3 vAnchors[] = {c + ezVec3(0, 0, 1.5f), c, c - ezVec3(0, 0, 1.5f)};

    ezDebugRenderer::Draw3DText(m_pWorld.Borrow(), "Center", vAnchors[0], ezColor::White, 16, ezDebugTextHAlign::Center, ezDebugTextVAlign::Bottom);
    ezDebugRenderer::Draw3DText(m_pWorld.Borrow(), "Left", vAnchors[1], ezColor::Yellow, 16, ezDebugTextHAlign::Left, ezDebugTextVAlign::Center);
    ezDebugRenderer::Draw3DText(m_pWorld.Borrow(), "Right", vAnchors[2], ezColor::Orange, 16, ezDebugTextHAlign::Right, ezDebugTextVAlign::Top);

    for (const ezVec3& vAnchor : vAnchors)
    {
      ezDebugRenderer::DrawCross(m_pWorld.Borrow(), vAnchor, 0.5f, ezColor::Red);
    }
  }

  // Screen space: 2D lines, 2D triangles with both windings and a line rectangle around them.
  {
    ezDebugRenderer::Draw2DLineRectangle(m_pWorld.Borrow(), ezRectFloat(4, 212, 312, 24), 0.0f, ezColor::Gray);

    ezTempHybridArray<ezDebugRendererLine, 8> lines;
    for (ezUInt32 i = 0; i < 8; ++i)
    {
      ezDebugRendererLine& line = lines.ExpandAndGetRef();
      line.m_start.Set(10.0f + i * 16.0f, (i % 2 == 0) ? 216.0f : 232.0f, 0.0f);
      line.m_end.Set(26.0f + i * 16.0f, (i % 2 == 0) ? 232.0f : 216.0f, 0.0f);
      line.m_startColor = ezColor::MakeHSV(30.0f * i, 1.0f, 1.0f);
      line.m_endColor = ezColor::MakeHSV(30.0f * (i + 1), 1.0f, 1.0f);
    }
    ezDebugRenderer::Draw2DLines(m_pWorld.Borrow(), lines, ezColor::White);

    ezDebugRendererTriangle tris[2];
    tris[0] = ezDebugRendererTriangle(ezVec3(180, 232, 0), ezVec3(200, 216, 0), ezVec3(220, 232, 0));
    tris[0].m_color = ezColor::LawnGreen;
    tris[1] = ezDebugRendererTriangle(ezVec3(240, 216, 0), ezVec3(260, 232, 0), ezVec3(280, 216, 0));
    tris[1].m_color = ezColor::HotPink;
    ezDebugRenderer::Draw2DTriangles(m_pWorld.Borrow(), ezMakeArrayPtr(tris), ezColor::White);
  }

  Run();
  if (ShouldApplicationQuit())
    return ezTestAppRun::Quit;

  // first frame no image is captured yet
  if (iCurFrame < 1)
    return ezTestAppRun::Continue;

  EZ_TEST_IMAGE(0, 150);

  return ezTestAppRun::Quit;
}
