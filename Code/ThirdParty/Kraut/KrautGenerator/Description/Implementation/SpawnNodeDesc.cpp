#include <KrautGenerator/PCH.h>

#include <KrautGenerator/Description/SpawnNodeDesc.h>

namespace Kraut
{
  SpawnNodeDesc::SpawnNodeDesc()
  {
    Reset();
  }

  SpawnNodeDesc::~SpawnNodeDesc() = default;

  void SpawnNodeDesc::Reset()
  {
    m_sTexture[Kraut::BranchGeometryType::Branch] = "Textures/Bark/Default.dds";
    //m_sTextureN[Kraut::BranchGeometryType::Branch] = "";

    m_bUsed = false;
    m_bEnable[Kraut::BranchGeometryType::Frond] = false;
    m_bRestrictGrowthToFrondPlane = false;
    m_bEnable[Kraut::BranchGeometryType::Branch] = true;
    m_bVisible = true;

    m_uiMinBranches = 4;
    m_uiMaxBranches = 4;

    m_fNodeHeight = 0.0f;
    m_BranchTypeMode = Kraut::BranchTypeMode::Default;

    m_fNodeSpacingBefore = 0.5f;
    m_fNodeSpacingAfter = 0.0f;

    m_fBranchlessPartABS = 0.0f;
    m_fBranchlessPartEndABS = 0.0f;

    m_fBranchLengthScale.Initialize(20, 1.0f, 0.0f, 1.0f);
    m_MaxBranchLengthParentScale.Initialize(20, 1.0f, 0.0f, 1.0f);
    m_BranchContour.Initialize(50, 1.0f, 0.1f, 1.0f);

    m_uiMinBranchLengthInCM = 100;
    m_uiMaxBranchLengthInCM = 100;

    //m_fMinBranchThickness = 0.4f;
    m_uiMinBranchThicknessInCM = 20;
    m_uiMaxBranchThicknessInCM = 20;

    m_fMaxRotationalDeviation = 0.0f;

    m_fBranchAngle = 90.0f;
    m_fMaxBranchAngleDeviation = 0.0f;

    m_bTargetDirRelative = false;
    m_TargetDirection = Kraut::BranchTargetDir::Straight;
    m_TargetDirection2 = Kraut::BranchTargetDir::Upwards;
    m_TargetDir2Uage = Kraut::BranchTargetDir2Usage::Off;
    m_fTargetDir2Usage = 2.5f;
    m_fMaxTargetDirDeviation = 0.0f;

    m_fGrowMaxTargetDirDeviation = 0.0f;
    m_fGrowMaxDirChangePerSegment = 5.0f;

    m_fRoundnessFactor = 0.5f;

    m_bActAsObstacle = true;
    m_bDoPhysicalSimulation = true;
    m_fPhysicsLookAhead = 1.5f;
    m_fPhysicsEvasionAngle = 30.0f;

    m_uiLowerBound = 0;
    m_uiUpperBound = 100;

    m_bAllowSubType[0] = true;
    m_bAllowSubType[1] = true;
    m_bAllowSubType[2] = true;

    m_FrondUpOrientation = Kraut::LeafOrientation::Upwards;
    m_uiMaxFrondOrientationDeviation = 0;

    switch (m_Type)
    {
      case Kraut::BranchType::Trunk1:
      case Kraut::BranchType::Trunk2:
      case Kraut::BranchType::Trunk3:
        m_iSegmentLengthCM = 5;
        m_uiMinBranchThicknessInCM = 40;
        m_uiMaxBranchThicknessInCM = 40;
        break;
      case Kraut::BranchType::MainBranches1:
      case Kraut::BranchType::MainBranches2:
      case Kraut::BranchType::MainBranches3:
        m_iSegmentLengthCM = 5;
        break;
      case Kraut::BranchType::SubBranches1:
      case Kraut::BranchType::SubBranches2:
      case Kraut::BranchType::SubBranches3:
        m_iSegmentLengthCM = 2;
        break;
      case Kraut::BranchType::Twigs1:
      case Kraut::BranchType::Twigs2:
      case Kraut::BranchType::Twigs3:
        m_iSegmentLengthCM = 1;
        break;
      //case Kraut::BranchType::SubTwigs1:
      //case Kraut::BranchType::SubTwigs2:
      //case Kraut::BranchType::SubTwigs3:
      //  m_iSegmentLengthCM = 1;
      //  break;
      default:
        m_iSegmentLengthCM = 10;
        break;
    }


    // ********* Fronds *********
    m_uiNumFronds = 1;
    m_uiFrondDetail = 1;
    m_fFrondWidth = 0.5f;
    m_fFrondHeight = 0.5f;
    m_uiMaxFrondOrientationDeviation = 0;
    m_FrondUpOrientation = Kraut::LeafOrientation::AlongBranch;
    m_FrondContour.Initialize(40, 1.0f, 0.0f, 1.0f);
    m_FrondWidth.Initialize(50, 1.0f, 0.0f, 1.0f);
    m_FrondHeight.Initialize(50, 1.0f, -1.0f, 1.0f);
    m_FrondContourMode = SpawnNodeDesc::Full;
    m_uiVariationColor[Kraut::BranchGeometryType::Frond][0] = m_uiVariationColor[Kraut::BranchGeometryType::Frond][1] = m_uiVariationColor[Kraut::BranchGeometryType::Frond][2] = 0;
    m_uiVariationColor[Kraut::BranchGeometryType::Frond][3] = 50;
    m_fTextureRepeat = 0.0f;
    m_bAlignFrondsOnSurface = false;

    m_sTexture[Kraut::BranchGeometryType::Frond] = "Textures/Leaves/Default.dds";
    //m_sTextureN[Kraut::BranchGeometryType::Frond] = "";


    // ********* Billboard Leaves *********
    m_bBillboardLeaves = true;
    m_bEnable[Kraut::BranchGeometryType::Leaf] = false;
    m_fLeafSize = 0.25f;
    m_fLeafInterval = 0;
    m_LeafScale.Initialize(25, 1.0f, 0.0f, 1.0f);

    m_sTexture[Kraut::BranchGeometryType::Leaf] = "Textures/LeafBillboards/Default.dds";
    //m_sTextureN[Kraut::BranchGeometryType::Leaf] = "";

    m_uiVariationColor[Kraut::BranchGeometryType::Leaf][0] = m_uiVariationColor[Kraut::BranchGeometryType::Leaf][1] = m_uiVariationColor[Kraut::BranchGeometryType::Leaf][2] = 0;
    m_uiVariationColor[Kraut::BranchGeometryType::Leaf][3] = 50;


    // ********* Flares *********
    m_uiFlares = 0;
    m_fFlareWidth = 2.0f;
    m_FlareWidthCurve.Initialize(50, 1.0f, 0.0f, 1.0f);
    m_fFlareRotation = 0.0f;
    m_bRotateTexCoords = true;
  }

  void SpawnNodeDesc::Serialize(aeStreamOut& s) const
  {
    aeUInt32 uiVersion = 43;
    s << uiVersion;

    s << m_bUsed;

    const aeInt8 e = m_Type;
    s << e;

    s << m_uiMinBranches;
    s << m_uiMaxBranches;

    // Version 30: Removed this line
    // s << fNodeChance;

    s << m_fNodeHeight;
    s << m_fNodeSpacingBefore;
    s << m_fNodeSpacingAfter;

    s << m_fBranchlessPartABS;

    // Version 37: Removed this line
    //s << m_fMinBranchThickness;

    // Version 30: Removed this line
    // s << m_fMaxBranchThickness;

    s << m_fMaxRotationalDeviation;
    s << m_fBranchAngle;
    s << m_fMaxBranchAngleDeviation;

    aeUInt8 td = m_TargetDirection;
    s << td;
    s << m_fMaxTargetDirDeviation;

    s << m_fGrowMaxTargetDirDeviation;
    s << m_fGrowMaxDirChangePerSegment;

    s << m_fRoundnessFactor;

    float m_fWindInfluence = 0.0f;
    s << m_fWindInfluence;

    s << m_fPhysicsLookAhead;
    s << m_fPhysicsEvasionAngle;

    // Version 4
    //s << m_fLowerBound;
    //s << m_fUpperBound;

    // Version 5
    s << m_bAllowSubType[0];
    s << m_bAllowSubType[1];
    s << m_bAllowSubType[2];

    // Version 6
    float m_fWindBendiness = 0.0f;
    s << m_fWindBendiness;

    // Version 7
    s << m_bEnable[Kraut::BranchGeometryType::Frond];

    // Version 8
    s << m_sTexture[Kraut::BranchGeometryType::Branch];

    // Version 9
    // s << m_sTextureN[Kraut::BranchGeometryType::Branch]; // Removed in Version 43

    // Version 10
    s << m_fBranchlessPartEndABS;
    //s << m_fBranchlessPartEndREL;

    // Version 11
    //s << m_bAlignSubsAtTip;
    bool bTemp = true;
    s << bTemp;

    // Version 12
    td = m_TargetDirection2;
    s << td;
    td = m_TargetDir2Uage;
    s << td;
    s << m_fTargetDir2Usage;

    // Version 13
    s << m_uiLowerBound;
    s << m_uiUpperBound;

    // Version 14
    s << m_uiMinBranchLengthInCM;
    s << m_uiMaxBranchLengthInCM;

    // Version 15
    // Kraut::BranchType was extended

    // Version 16
    s << m_bDoPhysicalSimulation;

    // Version 17
    s << (aeUInt8)m_FrondUpOrientation;
    s << m_uiMaxFrondOrientationDeviation;

    // Version 18
    s << m_iSegmentLengthCM;

    // Version 19
    s << m_bActAsObstacle;

    // Version 20
    s << m_bEnable[Kraut::BranchGeometryType::Branch];
    m_BranchContour.Serialize(s);

    // Version 21
    m_fBranchLengthScale.Serialize(s);

    // Version 22
    // Removed BranchlessPartREL (start/end)

    // Version 23
    s << m_sTexture[Kraut::BranchGeometryType::Frond];
    //s << m_sTextureN[Kraut::BranchGeometryType::Frond]; // Removed in Version 43

    // Version 24
    s << m_uiNumFronds;
    //s << m_uiFrondWidthCM;  // removed in version 42
    //s << m_uiFrondHeightCM; // removed in version 42
    m_FrondContour.Serialize(s);
    m_FrondWidth.Serialize(s);
    m_FrondHeight.Serialize(s);

    // Version 25
    s << m_uiFrondDetail;

    // Version 26
    {
      aeInt8 t = (aeInt8)m_FrondContourMode;
      s << t;
    }

    // Version 27
    s << m_bRestrictGrowthToFrondPlane;

    // Version 28
    m_MaxBranchLengthParentScale.Serialize(s);

    // Version 29 (not used in Version 30 anymore)
    // s << m_bVisible;

    // Version 31
    s << m_bTargetDirRelative;

    // Version 32
    s << m_bEnable[Kraut::BranchGeometryType::Leaf];
    s << m_fLeafSize;
    s << m_sTexture[Kraut::BranchGeometryType::Leaf];
    //s << m_sTextureN[Kraut::BranchGeometryType::Leaf]; // Removed in Version 43

    // Version 33
    s << m_fLeafInterval;

    // Version 34
    m_LeafScale.Serialize(s);

    // Version 35
    s << m_uiFlares;
    s << m_fFlareWidth;
    m_FlareWidthCurve.Serialize(s);
    s << m_fFlareRotation;

    // Version 36
    s << m_bRotateTexCoords;

    // Version 37
    s << m_uiMinBranchThicknessInCM;
    s << m_uiMaxBranchThicknessInCM;

    // Version 38
    s << m_uiVariationColor[Kraut::BranchGeometryType::Frond][0];
    s << m_uiVariationColor[Kraut::BranchGeometryType::Frond][1];
    s << m_uiVariationColor[Kraut::BranchGeometryType::Frond][2];
    s << m_uiVariationColor[Kraut::BranchGeometryType::Frond][3];
    s << m_uiVariationColor[Kraut::BranchGeometryType::Leaf][0];
    s << m_uiVariationColor[Kraut::BranchGeometryType::Leaf][1];
    s << m_uiVariationColor[Kraut::BranchGeometryType::Leaf][2];
    s << m_uiVariationColor[Kraut::BranchGeometryType::Leaf][3];

    // Version 39
    s << m_fTextureRepeat;
    s << m_bAlignFrondsOnSurface;

    // Version 40
    s << m_bBillboardLeaves;

    // Version 41
    const aeInt8 btm = (aeInt8)m_BranchTypeMode;
    s << btm;

    // Version 42
    s << m_fFrondWidth;
    s << m_fFrondHeight;
  }

  void SpawnNodeDesc::Deserialize(aeStreamIn& s)
  {
    Reset();

    aeUInt32 uiVersion = 1;
    s >> uiVersion;

    s >> m_bUsed;

    if (uiVersion >= 2)
    {
      aeInt8 e = 0;
      s >> e;
      m_Type = (Kraut::BranchType::Enum)e;

      if (uiVersion < 15)
      {
        if (e > 0)
          m_Type = (Kraut::BranchType::Enum)(e + 2);
      }
    }

    s >> m_uiMinBranches;
    s >> m_uiMaxBranches;

    if (uiVersion < 30)
    {
      float fNodeChance;
      s >> fNodeChance;
    }

    s >> m_fNodeHeight;
    s >> m_fNodeSpacingBefore;
    s >> m_fNodeSpacingAfter;

    s >> m_fBranchlessPartABS;

    if (uiVersion < 22)
    {
      float fDummy;
      s >> fDummy; //m_fBranchlessPartREL;
    }

    if (uiVersion < 21)
    {
      float fTemp;

      for (aeUInt32 i = 0; i < 10; ++i)
        s >> fTemp;
    }

    if (uiVersion < 14)
    {
      float fDummy;
      s >> fDummy;
      m_uiMinBranchLengthInCM = (aeUInt16)(fDummy * 100.0f);
      s >> fDummy;
      m_uiMaxBranchLengthInCM = (aeUInt16)(fDummy * 100.0f);
    }

    if (uiVersion < 36)
    {
      float m_fMinBranchThickness;
      s >> m_fMinBranchThickness;
    }

    if (uiVersion < 30)
    {
      float m_fMaxBranchThickness;
      s >> m_fMaxBranchThickness;

      m_uiMinBranchThicknessInCM = (aeUInt16)(m_fMaxBranchThickness * 100.0f);
      m_uiMaxBranchThicknessInCM = m_uiMinBranchThicknessInCM;
    }

    s >> m_fMaxRotationalDeviation;
    s >> m_fBranchAngle;
    s >> m_fMaxBranchAngleDeviation;

    aeUInt8 td;
    s >> td;
    m_TargetDirection = (Kraut::BranchTargetDir::Enum)td;
    s >> m_fMaxTargetDirDeviation;

    s >> m_fGrowMaxTargetDirDeviation;
    s >> m_fGrowMaxDirChangePerSegment;

    s >> m_fRoundnessFactor;
    float m_fWindInfluence;
    s >> m_fWindInfluence;

    if (uiVersion >= 3)
    {
      s >> m_fPhysicsLookAhead;
      s >> m_fPhysicsEvasionAngle;
    }

    if ((uiVersion >= 4) && (uiVersion < 13))
    {
      float fDummy;
      s >> fDummy;
      m_uiLowerBound = (aeUInt8)(fDummy * 100.0f);
      s >> fDummy;
      m_uiUpperBound = (aeUInt8)(fDummy * 100.0f);
    }

    if (uiVersion >= 5)
    {
      s >> m_bAllowSubType[0];
      s >> m_bAllowSubType[1];
      s >> m_bAllowSubType[2];
    }

    if (uiVersion >= 6)
    {
      float m_fWindBendiness;
      s >> m_fWindBendiness;
    }

    if (uiVersion >= 7)
    {
      s >> m_bEnable[Kraut::BranchGeometryType::Frond];
    }

    if (uiVersion >= 8)
    {
      aeString sTex;
      s >> sTex;
      m_sTexture[Kraut::BranchGeometryType::Branch] = sTex;
    }

    if ((uiVersion >= 9) && (uiVersion < 43))
    {
      aeString sTex;
      s >> sTex; //m_sTextureN[Kraut::BranchGeometryType::Branch] = sTex;
    }

    if (uiVersion >= 10)
    {
      s >> m_fBranchlessPartEndABS;

      if (uiVersion < 22)
      {
        float fDummy;
        s >> fDummy;
      }
    }

    if (uiVersion >= 11)
    {
      bool bTemp;
      s >> bTemp; //m_bAlignSubsAtTip;
    }

    if (uiVersion >= 12)
    {
      s >> td;
      m_TargetDirection2 = (Kraut::BranchTargetDir::Enum)td;
      s >> td;
      m_TargetDir2Uage = (Kraut::BranchTargetDir2Usage::Enum)td;
      s >> m_fTargetDir2Usage;
    }

    if (uiVersion >= 13)
    {
      s >> m_uiLowerBound;
      s >> m_uiUpperBound;
    }

    if (uiVersion >= 14)
    {
      s >> m_uiMinBranchLengthInCM;
      s >> m_uiMaxBranchLengthInCM;
    }

    if (uiVersion >= 16)
    {
      s >> m_bDoPhysicalSimulation;
    }

    if (uiVersion >= 17)
    {
      aeUInt8 temp;
      s >> temp;
      m_FrondUpOrientation = (Kraut::LeafOrientation::Enum)temp;
      s >> m_uiMaxFrondOrientationDeviation;
    }

    m_iSegmentLengthCM = 10;
    if (uiVersion >= 18)
    {
      s >> m_iSegmentLengthCM;
    }

    if (uiVersion >= 19)
    {
      s >> m_bActAsObstacle;
    }

    if (uiVersion >= 20)
    {
      s >> m_bEnable[Kraut::BranchGeometryType::Branch];
      m_BranchContour.Deserialize(s, uiVersion < 21);
    }

    if (uiVersion >= 21)
    {
      m_fBranchLengthScale.Deserialize(s, false);
    }

    // Version 22: Removed some data

    if (uiVersion >= 23)
    {
      aeString sTex;
      s >> sTex;
      m_sTexture[Kraut::BranchGeometryType::Frond] = sTex;

      if (uiVersion < 43)
      {
        s >> sTex; //m_sTextureN[Kraut::BranchGeometryType::Frond] = sTex;
      }
    }

    if (uiVersion >= 24)
    {
      s >> m_uiNumFronds;

      if (uiVersion < 42)
      {
        aeUInt8 m_uiFrondWidthCM;
        s >> m_uiFrondWidthCM;
        m_fFrondWidth = m_uiFrondWidthCM / 100.0f;

        aeUInt8 m_uiFrondHeightCM;
        s >> m_uiFrondHeightCM;
        m_fFrondHeight = m_uiFrondHeightCM / 100.0f;
      }

      m_FrondContour.Deserialize(s);
      m_FrondWidth.Deserialize(s);
      m_FrondHeight.Deserialize(s);
    }

    if (uiVersion >= 25)
    {
      s >> m_uiFrondDetail;
    }

    if (uiVersion >= 26)
    {
      aeInt8 t = 0;
      s >> t;
      m_FrondContourMode = (SpawnNodeDesc::FrondContourMode)t;
    }

    if (uiVersion >= 27)
    {
      s >> m_bRestrictGrowthToFrondPlane;
    }

    if (uiVersion >= 28)
    {
      m_MaxBranchLengthParentScale.Deserialize(s);
    }
    else
    {
      AE_CHECK_ALWAYS(false, "not implemented");
    }

    if (uiVersion == 29)
    {
      bool bVisible;
      s >> bVisible;
    }

    if (uiVersion >= 31)
    {
      s >> m_bTargetDirRelative;
    }

    if (uiVersion >= 32)
    {
      s >> m_bEnable[Kraut::BranchGeometryType::Leaf];
      s >> m_fLeafSize;

      aeString sTex;
      s >> sTex;
      m_sTexture[Kraut::BranchGeometryType::Leaf] = sTex;

      if (uiVersion < 43)
      {
        s >> sTex; //m_sTextureN[Kraut::BranchGeometryType::Leaf] = sTex;
      }
    }

    if (uiVersion >= 33)
    {
      s >> m_fLeafInterval;
    }

    if (uiVersion >= 34)
    {
      m_LeafScale.Deserialize(s);
    }

    if (uiVersion >= 35)
    {
      s >> m_uiFlares;
      s >> m_fFlareWidth;
      m_FlareWidthCurve.Deserialize(s);
      s >> m_fFlareRotation;
    }

    if (uiVersion >= 36)
    {
      s >> m_bRotateTexCoords;
    }

    if (uiVersion >= 37)
    {
      s >> m_uiMinBranchThicknessInCM;
      s >> m_uiMaxBranchThicknessInCM;
    }

    if (uiVersion >= 38)
    {
      s >> m_uiVariationColor[Kraut::BranchGeometryType::Frond][0];
      s >> m_uiVariationColor[Kraut::BranchGeometryType::Frond][1];
      s >> m_uiVariationColor[Kraut::BranchGeometryType::Frond][2];
      s >> m_uiVariationColor[Kraut::BranchGeometryType::Frond][3];
      s >> m_uiVariationColor[Kraut::BranchGeometryType::Leaf][0];
      s >> m_uiVariationColor[Kraut::BranchGeometryType::Leaf][1];
      s >> m_uiVariationColor[Kraut::BranchGeometryType::Leaf][2];
      s >> m_uiVariationColor[Kraut::BranchGeometryType::Leaf][3];
    }

    if (uiVersion >= 39)
    {
      s >> m_fTextureRepeat;
      s >> m_bAlignFrondsOnSurface;
    }

    if (uiVersion >= 40)
    {
      s >> m_bBillboardLeaves;
    }

    if (uiVersion >= 41)
    {
      aeInt8 btm = 0;
      s >> btm;
      m_BranchTypeMode = (Kraut::BranchTypeMode::Enum)btm;
    }

    if (uiVersion >= 42)
    {
      s >> m_fFrondWidth;
      s >> m_fFrondHeight;
    }
  }

  float SpawnNodeDesc::GetFlareWidthAt(float fPosAlongBranch, float fNodeRadius) const
  {
    return fNodeRadius * (m_FlareWidthCurve.GetValueAt(fPosAlongBranch) * (m_fFlareWidth - 1.0f) + 1.0f);
  }


  static float SCurve(float f)
  {
    float fExp = 2.0f;

    if (f <= 0.5f)
      return 0.5f + (1.0f - aeMath::Pow(f * 2.0f, fExp)) * 0.5f;

    f -= 0.5f;
    f *= 2.0f;
    f = 1.0f - f;

    return (aeMath::Pow(f, fExp)) * 0.5f;
  }

  static float InterpolateS(float fValueA, float fValueB, float fValueC, float fInterpolation)
  {
    if (fInterpolation <= 0.5f)
    {
      float fValue = SCurve(fInterpolation * 2.0f);
      float fRange = fValueA - fValueB;
      return fRange * fValue + fValueB;
    }

    float fRange = fValueC - fValueB;

    float fValue = 1.0f - SCurve((fInterpolation - 0.5f) * 2.0f);
    return fRange * fValue + fValueB;
  }

  static float GetFlareDistance0(aeUInt32 uiFlares, float fAtAngle, float fMinWidth, float fMaxWidth, float fFlareRotation)
  {
    if ((uiFlares == 0) || (fMaxWidth <= fMinWidth))
      return fMinWidth;

    while (fFlareRotation < 0)
      fFlareRotation += 360.0f;

    const float fAnglePerFlare = 360.0f / uiFlares;
    const float fVertexAngle = 360.0f * fAtAngle + fFlareRotation;
    const aeUInt32 uiFirstFlare = ((aeUInt32)(fVertexAngle / fAnglePerFlare));
    const float fInterpolation = (fVertexAngle - uiFirstFlare * fAnglePerFlare) / fAnglePerFlare;

    return InterpolateS(fMaxWidth, fMinWidth, fMaxWidth, fInterpolation);
  }

  float SpawnNodeDesc::GetFlareDistance(float fPosAlongBranch, float fNodeRadius, float fFlareWidth, float fAtAngle) const
  {
    return GetFlareDistance0(m_uiFlares, fAtAngle, fNodeRadius, fFlareWidth, fPosAlongBranch * m_fFlareRotation);
  }
} // namespace Kraut
