#include <PCH.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAsset.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAssetManager.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Image/Image.h>
#include <Foundation/Math/Curve1D.h>
#include <GameEngine/Curves/Curve1DResource.h>
#include <QPainter>
#include <QPaintEngine>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCurve1DControlPoint, 2, ezRTTIDefaultAllocator<ezCurve1DControlPoint>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Point", m_Point),
    EZ_MEMBER_PROPERTY("LeftTangent", m_LeftTangent)->AddAttributes(new ezDefaultValueAttribute(ezVec2(-0.1f, 0))),
    EZ_MEMBER_PROPERTY("RightTangent", m_RightTangent)->AddAttributes(new ezDefaultValueAttribute(ezVec2(+0.1f, 0))),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCurve1DData, 2, ezRTTIDefaultAllocator<ezCurve1DData>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("ControlPoints", m_ControlPoints),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCurve1DAssetData, 1, ezRTTIDefaultAllocator<ezCurve1DAssetData>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("Curves", m_Curves),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCurve1DAssetDocument, 3, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezCurve1DAssetDocument::ezCurve1DAssetDocument(const char* szDocumentPath)
  : ezSimpleAssetDocument<ezCurve1DAssetData>(szDocumentPath)
{
}

void ezCurve1DAssetDocument::FillCurve(ezUInt32 uiCurveIdx, ezCurve1D& out_Result) const
{
  out_Result.Clear();

  const ezCurve1DAssetData* pProp = static_cast<const ezCurve1DAssetData*>(GetProperties());

  const auto& curve = pProp->m_Curves[uiCurveIdx];

  for (const auto& cp : curve.m_ControlPoints)
  {
    auto& ccp = out_Result.AddControlPoint(cp.m_Point.x);
    ccp.m_Position.y = cp.m_Point.y;
    ccp.m_LeftTangent = cp.m_LeftTangent;
    ccp.m_RightTangent = cp.m_RightTangent;
  }
}

ezUInt32 ezCurve1DAssetDocument::GetCurveCount() const
{
  const ezCurve1DAssetData* pProp = GetProperties();
  return pProp->m_Curves.GetCount();
}

ezStatus ezCurve1DAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually){
  const ezCurve1DAssetData* pProp = GetProperties();

  ezCurve1DResourceDescriptor desc;
  desc.m_Curves.SetCount(pProp->m_Curves.GetCount());

  for (ezUInt32 i = 0; i < pProp->m_Curves.GetCount(); ++i)
  {
    FillCurve(i, desc.m_Curves[i]);
    desc.m_Curves[i].SortControlPoints();
  }

  desc.Save(stream);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezCurve1DAssetDocument::InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader)
{
  const ezCurve1DAssetData* pProp = GetProperties();

  QImage qimg(256, 256, QImage::Format_RGBA8888);
  qimg.fill(QColor(50, 50, 50));

  QPainter p(&qimg);
  QPainter* painter = &p;
  painter->setBrush(Qt::NoBrush);
  painter->setRenderHint(QPainter::Antialiasing);
  painter->setRenderHint(QPainter::HighQualityAntialiasing);


  float fExtentsMin, fExtentsMax;
  float fExtremesMin, fExtremesMax;

  for (ezUInt32 curveIdx = 0; curveIdx < pProp->m_Curves.GetCount(); ++curveIdx)
  {
    ezCurve1D curve;
    FillCurve(curveIdx, curve);

    curve.SortControlPoints();
    curve.CreateLinearApproximation();

    float fMin, fMax;
    curve.QueryExtents(fMin, fMax);

    float fMin2, fMax2;
    curve.QueryExtremeValues(fMin2, fMax2);

    if (curveIdx == 0)
    {
      fExtentsMin = fMin;
      fExtentsMax = fMax;
      fExtremesMin = fMin2;
      fExtremesMax = fMax2;
    }
    else
    {
      fExtentsMin = ezMath::Min(fExtentsMin, fMin);
      fExtentsMax = ezMath::Max(fExtentsMax, fMax);
      fExtremesMin = ezMath::Min(fExtremesMin, fMin2);
      fExtremesMax = ezMath::Max(fExtremesMax, fMax2);
    }
  }

  const float range = fExtentsMax - fExtentsMin;
  const float div = 1.0f / (qimg.width() - 1);
  const float factor = range * div;

  const float lowValue = (fExtremesMin > 0) ? 0.0f : fExtremesMin;
  const float highValue = (fExtremesMax < 0) ? 0.0f : fExtremesMax;

  const float range2 = highValue - lowValue;


  QColor curveColor[] =
  {
    QColor(255, 100, 100),
    QColor(100, 255, 100),
    QColor(100, 100, 255),
    QColor(200, 200, 200),
  };

  for (ezUInt32 curveIdx = 0; curveIdx < pProp->m_Curves.GetCount(); ++curveIdx)
  {
    QPainterPath path;

    ezCurve1D curve;
    FillCurve(curveIdx, curve);
    curve.SortControlPoints();
    curve.CreateLinearApproximation();

    const QColor curColor = curveColor[curveIdx % EZ_ARRAY_SIZE(curveColor)];
    QPen pen(curColor, 8.0f);
    painter->setPen(pen);

    for (ezUInt32 x = 0; x < (ezUInt32)qimg.width(); ++x)
    {
      const float pos = fExtentsMin + x * factor;
      const float value = 1.0f - (curve.Evaluate(pos) - lowValue) / range2;

      const ezUInt32 y = ezMath::Clamp<ezUInt32>(qimg.height() * value, 0, qimg.height() - 1);

      if (x == 0)
        path.moveTo(x, y);
      else
        path.lineTo(x, y);
    }

    painter->drawPath(path);
  }

  return SaveThumbnail(qimg, AssetHeader);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezCurve1DControlPointPatch_1_2 : public ezGraphPatch
{
public:
  ezCurve1DControlPointPatch_1_2()
    : ezGraphPatch(ezGetStaticRTTI<ezCurve1DControlPoint>(), 2) {}

  virtual void Patch(ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Left Tangent", "LeftTangent");
    pNode->RenameProperty("Right Tangent", "RightTangent");
  }
};

ezCurve1DControlPointPatch_1_2 g_ezCurve1DControlPointPatch_1_2;



class ezCurve1DDataPatch_1_2 : public ezGraphPatch
{
public:
  ezCurve1DDataPatch_1_2()
    : ezGraphPatch(ezGetStaticRTTI<ezCurve1DData>(), 2) {}

  virtual void Patch(ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Control Points", "ControlPoints");
  }
};

ezCurve1DDataPatch_1_2 g_ezCurve1DDataPatch_1_2;

