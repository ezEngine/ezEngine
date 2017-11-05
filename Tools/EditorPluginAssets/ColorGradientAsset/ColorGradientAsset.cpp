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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezColorControlPoint, 2, ezRTTIDefaultAllocator<ezColorControlPoint>)
{
  EZ_BEGIN_PROPERTIES
  {
    //EZ_MEMBER_PROPERTY("Position", m_fPositionX),
    EZ_MEMBER_PROPERTY("Tick", m_iTick),
    EZ_MEMBER_PROPERTY("Red", m_Red)->AddAttributes(new ezDefaultValueAttribute(255)),
    EZ_MEMBER_PROPERTY("Green", m_Green)->AddAttributes(new ezDefaultValueAttribute(255)),
    EZ_MEMBER_PROPERTY("Blue", m_Blue)->AddAttributes(new ezDefaultValueAttribute(255)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAlphaControlPoint, 2, ezRTTIDefaultAllocator<ezAlphaControlPoint>)
{
  EZ_BEGIN_PROPERTIES
  {
    //EZ_MEMBER_PROPERTY("Position", m_fPositionX),
    EZ_MEMBER_PROPERTY("Tick", m_iTick),
    EZ_MEMBER_PROPERTY("Alpha", m_Alpha)->AddAttributes(new ezDefaultValueAttribute(255)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezIntensityControlPoint, 2, ezRTTIDefaultAllocator<ezIntensityControlPoint>)
{
  EZ_BEGIN_PROPERTIES
  {
    //EZ_MEMBER_PROPERTY("Position", m_fPositionX),
    EZ_MEMBER_PROPERTY("Tick", m_iTick),
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


void ezColorControlPoint::SetTickFromTime(double time, ezInt64 fps)
{
  const ezUInt32 uiTicksPerStep = 4800 / fps;
  m_iTick = (ezInt64)ezMath::Round(time * 4800.0, (double)uiTicksPerStep);
}

void ezAlphaControlPoint::SetTickFromTime(double time, ezInt64 fps)
{
  const ezUInt32 uiTicksPerStep = 4800 / fps;
  m_iTick = (ezInt64)ezMath::Round(time * 4800.0, (double)uiTicksPerStep);
}

void ezIntensityControlPoint::SetTickFromTime(double time, ezInt64 fps)
{
  const ezUInt32 uiTicksPerStep = 4800 / fps;
  m_iTick = (ezInt64)ezMath::Round(time * 4800.0, (double)uiTicksPerStep);
}

ezColorGradientAssetDocument::ezColorGradientAssetDocument(const char* szDocumentPath) : ezSimpleAssetDocument<ezColorGradientAssetData>(szDocumentPath)
{
}

ezInt64 ezColorGradientAssetData::TickFromTime(double time)
{
  /// \todo Make this a property ?
  const ezUInt32 uiFramesPerSecond = 60;
  const ezUInt32 uiTicksPerStep = 4800 / uiFramesPerSecond;
  return (ezInt64)ezMath::Round(time * 4800.0, (double)uiTicksPerStep);
}

void ezColorGradientAssetData::FillGradientData(ezColorGradient& out_Result) const
{
  for (const auto& cp : m_ColorCPs)
  {
    out_Result.AddColorControlPoint(cp.GetTickAsTime(), ezColorGammaUB(cp.m_Red, cp.m_Green, cp.m_Blue));
  }

  for (const auto& cp : m_AlphaCPs)
  {
    out_Result.AddAlphaControlPoint(cp.GetTickAsTime(), cp.m_Alpha);
  }

  for (const auto& cp : m_IntensityCPs)
  {
    out_Result.AddIntensityControlPoint(cp.GetTickAsTime(), cp.m_fIntensity);
  }
}

ezStatus ezColorGradientAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  const ezColorGradientAssetData* pProp = GetProperties();

  ezColorGradientResourceDescriptor desc;
  pProp->FillGradientData(desc.m_Gradient);
  desc.m_Gradient.SortControlPoints();

  desc.Save(stream);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezColorGradientAssetDocument::InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader)
{
  const ezColorGradientAssetData* pProp = GetProperties();

  ezImage img;
  img.SetWidth(256);
  img.SetHeight(256);
  img.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);
  img.AllocateImageData();

  ezColorGradient gradient;
  pProp->FillGradientData(gradient);
  gradient.SortControlPoints();

  double fMin, fMax;
  gradient.GetExtents(fMin, fMax);
  const double range = fMax - fMin;
  const double div = 1.0 / (img.GetWidth() - 1);
  const double factor = range * div;

  for (ezUInt32 x = 0; x < img.GetWidth(); ++x)
  {
    const double pos = fMin + x * factor;

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
    : ezGraphPatch("ezColorGradientAssetData", 2) {}

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Color CPs", "ColorCPs");
    pNode->RenameProperty("Alpha CPs", "AlphaCPs");
    pNode->RenameProperty("Intensity CPs", "IntensityCPs");
  }
};

ezColorGradientAssetDataPatch_1_2 g_ezColorGradientAssetDataPatch_1_2;

//////////////////////////////////////////////////////////////////////////

class ezColorControlPoint_1_2 : public ezGraphPatch
{
public:
  ezColorControlPoint_1_2()
    : ezGraphPatch("ezColorControlPoint", 2) {}

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    auto* pPoint = pNode->FindProperty("Position");
    if (pPoint && pPoint->m_Value.IsA<float>())
    {
      const float fTime = pPoint->m_Value.Get<float>();
      pNode->AddProperty("Tick", (ezInt64)ezMath::Round(fTime * 4800.0, 4800.0 / 60.0));
    }
  }
};

ezColorControlPoint_1_2 g_ezColorControlPoint_1_2;

//////////////////////////////////////////////////////////////////////////

class ezAlphaControlPoint_1_2 : public ezGraphPatch
{
public:
  ezAlphaControlPoint_1_2()
    : ezGraphPatch("ezAlphaControlPoint", 2) {}

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    auto* pPoint = pNode->FindProperty("Position");
    if (pPoint && pPoint->m_Value.IsA<float>())
    {
      const float fTime = pPoint->m_Value.Get<float>();
      pNode->AddProperty("Tick", (ezInt64)ezMath::Round(fTime * 4800.0, 4800.0 / 60.0));
    }
  }
};

ezAlphaControlPoint_1_2 g_ezAlphaControlPoint_1_2;

//////////////////////////////////////////////////////////////////////////

class ezIntensityControlPoint_1_2 : public ezGraphPatch
{
public:
  ezIntensityControlPoint_1_2()
    : ezGraphPatch("ezIntensityControlPoint", 2) {}

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    auto* pPoint = pNode->FindProperty("Position");
    if (pPoint && pPoint->m_Value.IsA<float>())
    {
      const float fTime = pPoint->m_Value.Get<float>();
      pNode->AddProperty("Tick", (ezInt64)ezMath::Round(fTime * 4800.0, 4800.0 / 60.0));
    }
  }
};

ezIntensityControlPoint_1_2 g_ezIntensityControlPoint_1_2;
