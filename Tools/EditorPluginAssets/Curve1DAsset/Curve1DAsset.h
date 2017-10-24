#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <GuiFoundation/Widgets/CurveEditData.h>

class ezCurve1D;

class ezCurve1DAssetDocument : public ezSimpleAssetDocument<ezCurveGroupData>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCurve1DAssetDocument, ezSimpleAssetDocument<ezCurveGroupData>);

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
