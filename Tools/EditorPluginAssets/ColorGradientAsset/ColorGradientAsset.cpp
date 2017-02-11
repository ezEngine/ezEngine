#include <PCH.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAsset.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAssetManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Image/Image.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>
#include <Foundation/Math/ColorGradient.h>
#include <GameEngine/Curves/ColorGradientResource.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezColorControlPoint, 1, ezRTTIDefaultAllocator<ezColorControlPoint>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Position", m_fPositionX),
    EZ_MEMBER_PROPERTY("Red", m_Red)->AddAttributes(new ezDefaultValueAttribute(255)),
    EZ_MEMBER_PROPERTY("Green", m_Green)->AddAttributes(new ezDefaultValueAttribute(255)),
    EZ_MEMBER_PROPERTY("Blue", m_Blue)->AddAttributes(new ezDefaultValueAttribute(255)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAlphaControlPoint, 1, ezRTTIDefaultAllocator<ezAlphaControlPoint>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Position", m_fPositionX),
    EZ_MEMBER_PROPERTY("Alpha", m_Alpha)->AddAttributes(new ezDefaultValueAttribute(255)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezIntensityControlPoint, 1, ezRTTIDefaultAllocator<ezIntensityControlPoint>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Position", m_fPositionX),
    EZ_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezColorGradientAssetData, 2, ezRTTIDefaultAllocator<ezColorGradientAssetData>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("ColorCPs", m_ColorCPs),
    EZ_ARRAY_MEMBER_PROPERTY("AlphaCPs", m_AlphaCPs),
    EZ_ARRAY_MEMBER_PROPERTY("IntensityCPs", m_IntensityCPs),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezColorGradientAssetDocument, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezColorGradientAssetDocument::ezColorGradientAssetDocument(const char* szDocumentPath) : ezSimpleAssetDocument<ezColorGradientAssetData>(szDocumentPath)
{
}

void ezColorGradientAssetDocument::FillGradientData(ezColorGradient& out_Result) const
{
  const ezColorGradientAssetData* pProp = static_cast<const ezColorGradientAssetData*>(GetProperties());

  for (const auto& cp : pProp->m_ColorCPs)
  {
    out_Result.AddColorControlPoint(cp.m_fPositionX, ezColorGammaUB(cp.m_Red, cp.m_Green, cp.m_Blue));
  }

  for (const auto& cp : pProp->m_AlphaCPs)
  {
    out_Result.AddAlphaControlPoint(cp.m_fPositionX, cp.m_Alpha);
  }

  for (const auto& cp : pProp->m_IntensityCPs)
  {
    out_Result.AddIntensityControlPoint(cp.m_fPositionX, cp.m_fIntensity);
  }
}

ezStatus ezColorGradientAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually){
  const ezColorGradientAssetData* pProp = GetProperties();

  ezColorGradientResourceDescriptor desc;
  FillGradientData(desc.m_Gradient);
  desc.m_Gradient.SortControlPoints();

  desc.Save(stream);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezColorGradientAssetDocument::InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader)
{
  ezImage img;
  img.SetWidth(256);
  img.SetHeight(256);
  img.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);
  img.AllocateImageData();

  ezColorGradient gradient;
  FillGradientData(gradient);
  gradient.SortControlPoints();

  float fMin, fMax;
  gradient.GetExtents(fMin, fMax);
  const float range = fMax - fMin;
  const float div = 1.0f / (img.GetWidth() - 1);
  const float factor = range * div;

  for (ezUInt32 x = 0; x < img.GetWidth(); ++x)
  {
    const float pos = fMin + x * factor;

    ezColorGammaUB color;
    gradient.EvaluateColor(pos, color);

    ezUInt8 alpha;
    gradient.EvaluateAlpha(pos, alpha);
    const ezColorGammaUB alphaColor = ezColorLinearUB(alpha, alpha, alpha, 255);

    const float fAlphaFactor = ezMath::ColorByteToFloat(alpha);
    ezColor colorWithAlpha = color;
    colorWithAlpha.r *= fAlphaFactor;
    colorWithAlpha.g *= fAlphaFactor;
    colorWithAlpha.b *= fAlphaFactor;

    const ezColorGammaUB colWithAlpha = colorWithAlpha;

    for (ezUInt32 y = 0; y < img.GetHeight() / 4; ++y)
    {
      ezColorGammaUB* pixel = img.GetPixelPointer<ezColorGammaUB>(0, 0, 0, x, y);
      *pixel = alphaColor;
    }

    for (ezUInt32 y = img.GetHeight() / 4; y < img.GetHeight() / 2; ++y)
    {
      ezColorGammaUB* pixel = img.GetPixelPointer<ezColorGammaUB>(0, 0, 0, x, y);
      *pixel = colWithAlpha;
    }

    for (ezUInt32 y = img.GetHeight() / 2; y < img.GetHeight(); ++y)
    {
      ezColorGammaUB* pixel = img.GetPixelPointer<ezColorGammaUB>(0, 0, 0, x, y);
      *pixel = color;
    }
  }

  return SaveThumbnail(img, AssetHeader);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezColorGradientAssetDataPatch_1_2 : public ezGraphPatch
{
public:
  ezColorGradientAssetDataPatch_1_2()
    : ezGraphPatch(ezGetStaticRTTI<ezColorGradientAssetData>(), 2) {}

  virtual void Patch(ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Color CPs", "ColorCPs");
    pNode->RenameProperty("Alpha CPs", "AlphaCPs");
    pNode->RenameProperty("Intensity CPs", "IntensityCPs");
  }
};

ezColorGradientAssetDataPatch_1_2 g_ezColorGradientAssetDataPatch_1_2;
