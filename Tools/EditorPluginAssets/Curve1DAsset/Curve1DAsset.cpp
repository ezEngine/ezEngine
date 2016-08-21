#include <PCH.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAsset.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAssetManager.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <CoreUtils/Image/Image.h>
#include <Foundation/Math/Curve1D.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCurve1DControlPoint, 1, ezRTTIDefaultAllocator<ezCurve1DControlPoint>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Point", m_Point),
    EZ_MEMBER_PROPERTY("Left Tangent", m_LeftTangent)->AddAttributes(new ezDefaultValueAttribute(ezVec2(-0.1f, 0))),
    EZ_MEMBER_PROPERTY("Right Tangent", m_RightTangent)->AddAttributes(new ezDefaultValueAttribute(ezVec2(+0.1f, 0))),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCurve1DData, 1, ezRTTIDefaultAllocator<ezCurve1DData>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("Control Points", m_ControlPoints),
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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCurve1DAssetDocument, 1, ezRTTINoAllocator);
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
    ccp.m_fValue = cp.m_Point.y;
    ccp.m_LeftTangent = cp.m_LeftTangent;
    ccp.m_RightTangent = cp.m_RightTangent;
  }
}

ezUInt32 ezCurve1DAssetDocument::GetCurveCount() const
{
  const ezCurve1DAssetData* pProp = GetProperties();
  return pProp->m_Curves.GetCount();
}

ezStatus ezCurve1DAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform, const ezAssetFileHeader& AssetHeader)
{
  const ezCurve1DAssetData* pProp = GetProperties();

  //ezCurve1DResourceDescriptor desc;
  //FillcurveData(desc.m_curve);
  //desc.m_curve.SortControlPoints();

  //desc.Save(stream);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezCurve1DAssetDocument::InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader)
{
  const ezCurve1DAssetData* pProp = GetProperties();

  ezImage img;
  img.SetWidth(256);
  img.SetHeight(256);
  img.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);
  img.AllocateImageData();
  
  ezColorGammaUB fillColor(50, 50, 50);
  for (ezUInt32 y = 0; y < img.GetHeight(); ++y)
  {
    for (ezUInt32 x = 0; x < img.GetWidth(); ++x)
    {
      ezColorGammaUB* pixel = img.GetPixelPointer<ezColorGammaUB>(0, 0, 0, x, y, 0);
      *pixel = fillColor;
    }
  }

  float fExtentsMin, fExtentsMax;
  float fExtremesMin, fExtremesMax;

  for (ezUInt32 curveIdx = 0; curveIdx < pProp->m_Curves.GetCount(); ++curveIdx)
  {
    ezCurve1D curve;
    FillCurve(curveIdx, curve);

    float fMin, fMax;
    curve.GetExtents(fMin, fMax);

    float fMin2, fMax2;
    curve.GetExtremeValues(fMin2, fMax2);

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
  const float div = 1.0f / (img.GetWidth() - 1);
  const float factor = range * div;

  const float lowValue = (fExtremesMin > 0) ? 0.0f : fExtremesMin;
  const float highValue = (fExtremesMax < 0) ? 0.0f : fExtremesMax;

  const float range2 = highValue - lowValue;


  ezColorGammaUB curveColor[] =
  {
    ezColor::Red,
    ezColor::LimeGreen,
    ezColor::Blue,
    ezColor::Beige,
  };

  for (ezUInt32 curveIdx = 0; curveIdx < pProp->m_Curves.GetCount(); ++curveIdx)
  {
    ezCurve1D curve;
    FillCurve(curveIdx, curve);
    curve.SortControlPoints();

    const ezColorGammaUB curColor = curveColor[curveIdx % EZ_ARRAY_SIZE(curveColor)];

    for (ezUInt32 x = 0; x < img.GetWidth(); ++x)
    {
      const float pos = fExtentsMin + x * factor;
      const float value = 1.0f - (curve.Evaluate(pos) - lowValue) / range2;

      const ezUInt32 y = ezMath::Clamp<ezUInt32>(img.GetHeight() * value, 0, img.GetHeight() - 1);
      
      ezColorGammaUB* pixel = img.GetPixelPointer<ezColorGammaUB>(0, 0, 0, x, y);
      *pixel = curColor;
    }
  }

  return SaveThumbnail(img, AssetHeader);

  // no idea how to get a QPainter to a QImage
  //{
  //  QImage qimg(256, 256, QImage::Format_RGBA8888);
  //  qimg.fill(QColor(50, 50, 50));
  //  QPainter p(qimg.paintEngine()->paintDevice());
  //  QPainter* painter = &p;
  //  painter->setBrush(Qt::NoBrush);
  //  painter->setRenderHint(QPainter::Antialiasing);


  //  const ezCurve1DAssetData* pProp = GetProperties();

  //  float fExtentsMin, fExtentsMax;
  //  float fExtremesMin, fExtremesMax;

  //  for (ezUInt32 curveIdx = 0; curveIdx < pProp->m_Curves.GetCount(); ++curveIdx)
  //  {
  //    ezCurve1D curve;
  //    FillCurve(curveIdx, curve);

  //    float fMin, fMax;
  //    curve.GetExtents(fMin, fMax);

  //    float fMin2, fMax2;
  //    curve.GetExtremeValues(fMin2, fMax2);

  //    if (curveIdx == 0)
  //    {
  //      fExtentsMin = fMin;
  //      fExtentsMax = fMax;
  //      fExtremesMin = fMin2;
  //      fExtremesMax = fMax2;
  //    }
  //    else
  //    {
  //      fExtentsMin = ezMath::Min(fExtentsMin, fMin);
  //      fExtentsMax = ezMath::Max(fExtentsMax, fMax);
  //      fExtremesMin = ezMath::Min(fExtremesMin, fMin2);
  //      fExtremesMax = ezMath::Max(fExtremesMax, fMax2);
  //    }
  //  }

  //  const float range = fExtentsMax - fExtentsMin;
  //  const float div = 1.0f / (qimg.width() - 1);
  //  const float factor = range * div;

  //  const float lowValue = (fExtremesMin > 0) ? 0.0f : fExtremesMin;
  //  const float highValue = (fExtremesMax < 0) ? 0.0f : fExtremesMax;

  //  const float range2 = highValue - lowValue;


  //  QColor curveColor[] =
  //  {
  //    QColor(255, 0, 0),
  //    QColor(0, 255, 0),
  //    QColor(0, 0, 255),
  //    QColor(200, 200, 200),
  //  };

  //  for (ezUInt32 curveIdx = 0; curveIdx < pProp->m_Curves.GetCount(); ++curveIdx)
  //  {
  //    QPainterPath path;

  //    ezCurve1D curve;
  //    FillCurve(curveIdx, curve);
  //    curve.SortControlPoints();

  //    const QColor curColor = curveColor[curveIdx % EZ_ARRAY_SIZE(curveColor)];
  //    painter->setPen(curColor);

  //    for (ezUInt32 x = 0; x < (ezUInt32)qimg.width(); ++x)
  //    {
  //      const float pos = fExtentsMin + x * factor;
  //      const float value = 1.0f - (curve.Evaluate(pos) - lowValue) / range2;

  //      const ezUInt32 y = ezMath::Clamp<ezUInt32>(qimg.height() * value, 0, qimg.height() - 1);

  //      if (x == 0)
  //        path.moveTo(x, y);
  //      else
  //        path.lineTo(x, y);
  //    }

  //    painter->drawPath(path);
  //  }

  //  return SaveThumbnail(qimg, AssetHeader);
  //}
}

