#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <Foundation/Tracks/ColorGradient.h>

class ezColorGradientAssetData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezColorGradientAssetData, ezReflectedClass);

public:
  ezColorGradient m_Gradient;

  /// \brief Fills out the ezColorGradient structure with an exact copy of the data in the asset.
  /// Does NOT yet sort the control points, so before evaluating the color gradient, that must be called manually.
  void FillGradientData(ezColorGradient& out_result) const;
  ezColor Evaluate(ezInt64 iTick) const;
};

class ezColorGradientAssetDocument : public ezSimpleAssetDocument<ezColorGradientAssetData>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezColorGradientAssetDocument, ezSimpleAssetDocument<ezColorGradientAssetData>);

public:
  ezColorGradientAssetDocument(ezStringView sDocumentPath);

  void WriteResource(ezStreamWriter& inout_stream) const;

protected:
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;
  virtual ezTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};
