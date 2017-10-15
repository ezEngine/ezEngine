#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>

class ezCurve1D;

class ezCurve1DControlPoint : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCurve1DControlPoint, ezReflectedClass);
public:

  ezVec2 m_Point;
  ezVec2 m_LeftTangent;
  ezVec2 m_RightTangent;
};

class ezCurve1DData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCurve1DData, ezReflectedClass);
public:
  ezColorGammaUB m_CurveColor;
  ezDynamicArray<ezCurve1DControlPoint> m_ControlPoints;
};

class ezCurve1DAssetData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCurve1DAssetData, ezReflectedClass);
public:

  ezDynamicArray<ezCurve1DData> m_Curves;
};


class ezCurve1DAssetDocument : public ezSimpleAssetDocument<ezCurve1DAssetData>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCurve1DAssetDocument, ezSimpleAssetDocument<ezCurve1DAssetData>);

public:
  ezCurve1DAssetDocument(const char* szDocumentPath);

  virtual const char* QueryAssetType() const override { return "Curve1D"; }

  /// \brief Fills out the ezCurve1D structure with an exact copy of the data in the asset.
  /// Does NOT yet sort the control points, so before evaluating the curve, that must be called manually.
  void FillCurve(ezUInt32 uiCurveIdx, ezCurve1D& out_Result) const;

  ezUInt32 GetCurveCount() const;

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override;
  virtual ezStatus InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader) override;

};
