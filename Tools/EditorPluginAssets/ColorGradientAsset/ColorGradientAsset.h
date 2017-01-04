#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>

class ezColorGradient;

class ezColorControlPoint : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezColorControlPoint, ezReflectedClass);
public:

  float m_fPositionX;
  ezUInt8 m_Red;
  ezUInt8 m_Green;
  ezUInt8 m_Blue;
};

class ezAlphaControlPoint : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAlphaControlPoint, ezReflectedClass);
public:

  float m_fPositionX;
  ezUInt8 m_Alpha;
};

class ezIntensityControlPoint : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezIntensityControlPoint, ezReflectedClass);
public:

  float m_fPositionX;
  float m_fIntensity;
};

class ezColorGradientAssetData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezColorGradientAssetData, ezReflectedClass);
public:

  ezDynamicArray<ezColorControlPoint> m_ColorCPs;
  ezDynamicArray<ezAlphaControlPoint> m_AlphaCPs;
  ezDynamicArray<ezIntensityControlPoint> m_IntensityCPs;
};



class ezColorGradientAssetDocument : public ezSimpleAssetDocument<ezColorGradientAssetData>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezColorGradientAssetDocument, ezSimpleAssetDocument<ezColorGradientAssetData>);

public:
  ezColorGradientAssetDocument(const char* szDocumentPath);

  virtual const char* QueryAssetType() const override { return "ColorGradient"; }

  /// \brief Fills out the ezColorGradient structure with an exact copy of the data in the asset.
  /// Does NOT yet sort the control points, so before evaluating the color gradient, that must be called manually.
  void FillGradientData(ezColorGradient& out_Result) const;

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override;
  virtual ezStatus InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader) override;

};
