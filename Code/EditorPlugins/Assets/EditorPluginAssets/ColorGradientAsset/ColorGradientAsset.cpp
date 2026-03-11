#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Core/Curves/ColorGradientResource.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAsset.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezColorGradientAssetData, 3, ezRTTIDefaultAllocator<ezColorGradientAssetData>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Gradient", m_Gradient),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezColorGradientAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezColorGradientAssetDocument::ezColorGradientAssetDocument(ezStringView sDocumentPath)
  : ezSimpleAssetDocument<ezColorGradientAssetData>(sDocumentPath, ezAssetDocEngineConnection::None)
{
}

void ezColorGradientAssetDocument::WriteResource(ezStreamWriter& inout_stream) const
{
  const ezColorGradientAssetData* pProp = GetProperties();

  ezColorGradientResourceDescriptor desc;
  pProp->FillGradientData(desc.m_Gradient);

  desc.Save(inout_stream);
}

void ezColorGradientAssetData::FillGradientData(ezColorGradient& out_result) const
{
  out_result = m_Gradient;
}

ezColor ezColorGradientAssetData::Evaluate(ezInt64 iTick) const
{
  ezColorGradient temp = m_Gradient;

  ezColor color;
  temp.Evaluate(ezColorGradient::TickToTime(iTick), color);
  return color;
}

ezTransformStatus ezColorGradientAssetDocument::InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  WriteResource(stream);
  return ezStatus(EZ_SUCCESS);
}

ezTransformStatus ezColorGradientAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  const ezColorGradientAssetData* pProp = GetProperties();

  ezImageHeader imgHeader;
  imgHeader.SetWidth(256);
  imgHeader.SetHeight(256);
  imgHeader.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);
  ezImage img;
  img.ResetAndAlloc(imgHeader);

  ezColorGradient gradient;
  pProp->FillGradientData(gradient);

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
    const ezColorLinearUB alphaColor = ezColorLinearUB(alpha, alpha, alpha, 255);

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

  return SaveThumbnail(img, ThumbnailInfo);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezColorGradientAssetDataPatch_1_2 : public ezGraphPatch
{
public:
  ezColorGradientAssetDataPatch_1_2()
    : ezGraphPatch("ezColorGradientAssetData", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
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
    : ezGraphPatch("ezColorControlPoint", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    auto* pPoint = pNode->FindProperty("Position");
    if (pPoint && pPoint->m_Value.IsA<float>())
    {
      const float fTime = pPoint->m_Value.Get<float>();
      pNode->AddProperty("Tick", ezColorGradient::SnapTimeToTick(fTime));
    }
  }
};

ezColorControlPoint_1_2 g_ezColorControlPoint_1_2;

//////////////////////////////////////////////////////////////////////////

class ezAlphaControlPoint_1_2 : public ezGraphPatch
{
public:
  ezAlphaControlPoint_1_2()
    : ezGraphPatch("ezAlphaControlPoint", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    auto* pPoint = pNode->FindProperty("Position");
    if (pPoint && pPoint->m_Value.IsA<float>())
    {
      const float fTime = pPoint->m_Value.Get<float>();
      pNode->AddProperty("Tick", ezColorGradient::SnapTimeToTick(fTime));
    }
  }
};

ezAlphaControlPoint_1_2 g_ezAlphaControlPoint_1_2;

//////////////////////////////////////////////////////////////////////////

class ezIntensityControlPoint_1_2 : public ezGraphPatch
{
public:
  ezIntensityControlPoint_1_2()
    : ezGraphPatch("ezIntensityControlPoint", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    auto* pPoint = pNode->FindProperty("Position");
    if (pPoint && pPoint->m_Value.IsA<float>())
    {
      const float fTime = pPoint->m_Value.Get<float>();
      pNode->AddProperty("Tick", ezColorGradient::SnapTimeToTick(fTime));
    }
  }
};

ezIntensityControlPoint_1_2 g_ezIntensityControlPoint_1_2;

//////////////////////////////////////////////////////////////////////////

// Patch to rename old control point types to new standalone types
class ezColorControlPoint_2_3 : public ezGraphPatch
{
public:
  ezColorControlPoint_2_3()
    : ezGraphPatch("ezColorControlPoint", 3)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("ezColorGradientColorCP");
  }
};

ezColorControlPoint_2_3 g_ezColorControlPoint_2_3;

//////////////////////////////////////////////////////////////////////////

class ezAlphaControlPoint_2_3 : public ezGraphPatch
{
public:
  ezAlphaControlPoint_2_3()
    : ezGraphPatch("ezAlphaControlPoint", 3)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("ezColorGradientAlphaCP");
  }
};

ezAlphaControlPoint_2_3 g_ezAlphaControlPoint_2_3;

//////////////////////////////////////////////////////////////////////////

class ezIntensityControlPoint_2_3 : public ezGraphPatch
{
public:
  ezIntensityControlPoint_2_3()
    : ezGraphPatch("ezIntensityControlPoint", 3)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("ezColorGradientIntensityCP");
  }
};

ezIntensityControlPoint_2_3 g_ezIntensityControlPoint_2_3;

//////////////////////////////////////////////////////////////////////////

class ezColorGradientAssetDataPatch_2_3 : public ezGraphPatch
{
public:
  ezColorGradientAssetDataPatch_2_3()
    : ezGraphPatch("ezColorGradientAssetData", 3)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    // Create new Gradient node
    ezUuid gradientGuid = ezUuid::MakeUuid();
    auto* pGradientNode = pGraph->AddNode(gradientGuid, "ezColorGradient", 1);

    // Migrate ColorCPs
    auto* pColorCPs = pNode->FindProperty("ColorCPs");
    if (pColorCPs && pColorCPs->m_Value.IsA<ezVariantArray>())
    {
      const ezVariantArray& oldCPs = pColorCPs->m_Value.Get<ezVariantArray>();
      ezVariantArray newCPs;

      for (const auto& cpGuid : oldCPs)
      {
        auto* pOldCP = pGraph->GetNode(cpGuid.Get<ezUuid>());
        if (pOldCP)
        {
          ezUuid newCPGuid = ezUuid::MakeUuid();
          auto* pNewCP = pGraph->AddNode(newCPGuid, "ezColorGradientColorCP", 1);

          pNewCP->AddProperty("Tick", pOldCP->FindProperty("Tick")->m_Value);
          pNewCP->AddProperty("Red", pOldCP->FindProperty("Red")->m_Value);
          pNewCP->AddProperty("Green", pOldCP->FindProperty("Green")->m_Value);
          pNewCP->AddProperty("Blue", pOldCP->FindProperty("Blue")->m_Value);

          newCPs.PushBack(newCPGuid);
        }
      }
      pGradientNode->AddProperty("ColorCPs", newCPs);
    }

    // Migrate AlphaCPs
    auto* pAlphaCPs = pNode->FindProperty("AlphaCPs");
    if (pAlphaCPs && pAlphaCPs->m_Value.IsA<ezVariantArray>())
    {
      const ezVariantArray& oldCPs = pAlphaCPs->m_Value.Get<ezVariantArray>();
      ezVariantArray newCPs;

      for (const auto& cpGuid : oldCPs)
      {
        auto* pOldCP = pGraph->GetNode(cpGuid.Get<ezUuid>());
        if (pOldCP)
        {
          ezUuid newCPGuid = ezUuid::MakeUuid();
          auto* pNewCP = pGraph->AddNode(newCPGuid, "ezColorGradientAlphaCP", 1);

          pNewCP->AddProperty("Tick", pOldCP->FindProperty("Tick")->m_Value);
          pNewCP->AddProperty("Alpha", pOldCP->FindProperty("Alpha")->m_Value);

          newCPs.PushBack(newCPGuid);
        }
      }
      pGradientNode->AddProperty("AlphaCPs", newCPs);
    }

    // Migrate IntensityCPs
    auto* pIntensityCPs = pNode->FindProperty("IntensityCPs");
    if (pIntensityCPs && pIntensityCPs->m_Value.IsA<ezVariantArray>())
    {
      const ezVariantArray& oldCPs = pIntensityCPs->m_Value.Get<ezVariantArray>();
      ezVariantArray newCPs;

      for (const auto& cpGuid : oldCPs)
      {
        auto* pOldCP = pGraph->GetNode(cpGuid.Get<ezUuid>());
        if (pOldCP)
        {
          ezUuid newCPGuid = ezUuid::MakeUuid();
          auto* pNewCP = pGraph->AddNode(newCPGuid, "ezColorGradientIntensityCP", 1);

          pNewCP->AddProperty("Tick", pOldCP->FindProperty("Tick")->m_Value);
          pNewCP->AddProperty("Intensity", pOldCP->FindProperty("Intensity")->m_Value);

          newCPs.PushBack(newCPGuid);
        }
      }
      pGradientNode->AddProperty("IntensityCPs", newCPs);
    }

    // Replace old structure with new
    pNode->RemoveProperty("ColorCPs");
    pNode->RemoveProperty("AlphaCPs");
    pNode->RemoveProperty("IntensityCPs");
    pNode->AddProperty("Gradient", gradientGuid);
  }
};

ezColorGradientAssetDataPatch_2_3 g_ezColorGradientAssetDataPatch_2_3;
