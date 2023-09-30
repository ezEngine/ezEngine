#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Core/Curves/Curve1DResource.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAsset.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCurve1DAssetDocument, 3, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezCurve1DAssetDocument::ezCurve1DAssetDocument(ezStringView sDocumentPath)
  : ezSimpleAssetDocument<ezCurveGroupData>(sDocumentPath, ezAssetDocEngineConnection::None)
{
}

ezCurve1DAssetDocument::~ezCurve1DAssetDocument() = default;

void ezCurve1DAssetDocument::FillCurve(ezUInt32 uiCurveIdx, ezCurve1D& out_result) const
{
  const ezCurveGroupData* pProp = static_cast<const ezCurveGroupData*>(GetProperties());
  pProp->ConvertToRuntimeData(uiCurveIdx, out_result);
}

ezUInt32 ezCurve1DAssetDocument::GetCurveCount() const
{
  const ezCurveGroupData* pProp = GetProperties();
  return pProp->m_Curves.GetCount();
}

void ezCurve1DAssetDocument::WriteResource(ezStreamWriter& inout_stream) const
{
  const ezCurveGroupData* pProp = GetProperties();

  ezCurve1DResourceDescriptor desc;
  desc.m_Curves.SetCount(pProp->m_Curves.GetCount());

  for (ezUInt32 i = 0; i < pProp->m_Curves.GetCount(); ++i)
  {
    FillCurve(i, desc.m_Curves[i]);
    desc.m_Curves[i].SortControlPoints();
  }

  desc.Save(inout_stream);
}

ezTransformStatus ezCurve1DAssetDocument::InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  WriteResource(stream);
  return ezStatus(EZ_SUCCESS);
}

ezTransformStatus ezCurve1DAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  const ezCurveGroupData* pProp = GetProperties();

  QImage qimg(ezThumbnailSize, ezThumbnailSize, QImage::Format_RGBA8888);
  qimg.fill(QColor(50, 50, 50));

  QPainter p(&qimg);
  QPainter* painter = &p;
  painter->setBrush(Qt::NoBrush);
  painter->setRenderHint(QPainter::Antialiasing);

  if (!pProp->m_Curves.IsEmpty())
  {

    double fExtentsMin, fExtentsMax;
    double fExtremesMin, fExtremesMax;

    for (ezUInt32 curveIdx = 0; curveIdx < pProp->m_Curves.GetCount(); ++curveIdx)
    {
      ezCurve1D curve;
      FillCurve(curveIdx, curve);

      curve.SortControlPoints();
      curve.CreateLinearApproximation();

      double fMin, fMax;
      curve.QueryExtents(fMin, fMax);

      double fMin2, fMax2;
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

    for (ezUInt32 curveIdx = 0; curveIdx < pProp->m_Curves.GetCount(); ++curveIdx)
    {
      QPainterPath path;

      ezCurve1D curve;
      FillCurve(curveIdx, curve);
      curve.SortControlPoints();
      curve.CreateLinearApproximation();

      const QColor curColor = ezToQtColor(pProp->m_Curves[curveIdx]->m_CurveColor);
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
  }

  return SaveThumbnail(qimg, ThumbnailInfo);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezCurve1DControlPointPatch_1_2 : public ezGraphPatch
{
public:
  ezCurve1DControlPointPatch_1_2()
    : ezGraphPatch("ezCurve1DControlPoint", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
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
    : ezGraphPatch("ezCurve1DData", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override { pNode->RenameProperty("Control Points", "ControlPoints"); }
};

ezCurve1DDataPatch_1_2 g_ezCurve1DDataPatch_1_2;
