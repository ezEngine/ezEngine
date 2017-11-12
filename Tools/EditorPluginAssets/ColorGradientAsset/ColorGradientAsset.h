#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>

class ezColorGradient;

class ezColorControlPoint : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezColorControlPoint, ezReflectedClass);
public:

  double GetTickAsTime() const { return m_iTick / 4800.0; }
  void SetTickFromTime(double time, ezInt64 fps);

  //double m_fPositionX;
  ezInt64 m_iTick; // 4800 ticks per second
  ezUInt8 m_Red;
  ezUInt8 m_Green;
  ezUInt8 m_Blue;
};

class ezAlphaControlPoint : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAlphaControlPoint, ezReflectedClass);
public:

  double GetTickAsTime() const { return m_iTick / 4800.0; }
  void SetTickFromTime(double time, ezInt64 fps);

  //double m_fPositionX;
  ezInt64 m_iTick; // 4800 ticks per second
  ezUInt8 m_Alpha;
};

class ezIntensityControlPoint : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezIntensityControlPoint, ezReflectedClass);
public:

  double GetTickAsTime() const { return m_iTick / 4800.0; }
  void SetTickFromTime(double time, ezInt64 fps);

  //double m_fPositionX;
  ezInt64 m_iTick; // 4800 ticks per second
  float m_fIntensity;
};

class ezColorGradientAssetData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezColorGradientAssetData, ezReflectedClass);
public:

  ezDynamicArray<ezColorControlPoint> m_ColorCPs;
  ezDynamicArray<ezAlphaControlPoint> m_AlphaCPs;
  ezDynamicArray<ezIntensityControlPoint> m_IntensityCPs;

  ezInt64 TickFromTime(double time);

  /// \brief Fills out the ezColorGradient structure with an exact copy of the data in the asset.
  /// Does NOT yet sort the control points, so before evaluating the color gradient, that must be called manually.
  void FillGradientData(ezColorGradient& out_Result) const;
  ezColor Evaluate(ezInt64 iTick) const;

};

class ezColorGradientAssetDocument : public ezSimpleAssetDocument<ezColorGradientAssetData>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezColorGradientAssetDocument, ezSimpleAssetDocument<ezColorGradientAssetData>);

public:
  ezColorGradientAssetDocument(const char* szDocumentPath);

  virtual const char* QueryAssetType() const override { return "ColorGradient"; }

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override;
  virtual ezStatus InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader) override;

};
