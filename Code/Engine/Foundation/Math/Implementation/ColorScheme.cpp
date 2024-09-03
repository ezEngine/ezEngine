#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/ColorScheme.h>

ezColor ezColorScheme::s_Colors[Count][10] = {
  {
    ezColorGammaUB(201, 42, 42),   // oc-red-9
    ezColorGammaUB(224, 49, 49),   // oc-red-8
    ezColorGammaUB(240, 62, 62),   // oc-red-7
    ezColorGammaUB(250, 82, 82),   // oc-red-6
    ezColorGammaUB(255, 107, 107), // oc-red-5
    ezColorGammaUB(255, 135, 135), // oc-red-4
    ezColorGammaUB(255, 168, 168), // oc-red-3
    ezColorGammaUB(255, 201, 201), // oc-red-2
    ezColorGammaUB(255, 227, 227), // oc-red-1
    ezColorGammaUB(255, 245, 245), // oc-red-0
  },
  {
    ezColorGammaUB(166, 30, 77),   // oc-pink-9
    ezColorGammaUB(194, 37, 92),   // oc-pink-8
    ezColorGammaUB(214, 51, 108),  // oc-pink-7
    ezColorGammaUB(230, 73, 128),  // oc-pink-6
    ezColorGammaUB(240, 101, 149), // oc-pink-5
    ezColorGammaUB(247, 131, 172), // oc-pink-4
    ezColorGammaUB(250, 162, 193), // oc-pink-3
    ezColorGammaUB(252, 194, 215), // oc-pink-2
    ezColorGammaUB(255, 222, 235), // oc-pink-1
    ezColorGammaUB(255, 240, 246), // oc-pink-0
  },
  {
    ezColorGammaUB(134, 46, 156),  // oc-grape-9
    ezColorGammaUB(156, 54, 181),  // oc-grape-8
    ezColorGammaUB(174, 62, 201),  // oc-grape-7
    ezColorGammaUB(190, 75, 219),  // oc-grape-6
    ezColorGammaUB(204, 93, 232),  // oc-grape-5
    ezColorGammaUB(218, 119, 242), // oc-grape-4
    ezColorGammaUB(229, 153, 247), // oc-grape-3
    ezColorGammaUB(238, 190, 250), // oc-grape-2
    ezColorGammaUB(243, 217, 250), // oc-grape-1
    ezColorGammaUB(248, 240, 252), // oc-grape-0
  },
  {
    ezColorGammaUB(95, 61, 196),   // oc-violet-9
    ezColorGammaUB(103, 65, 217),  // oc-violet-8
    ezColorGammaUB(112, 72, 232),  // oc-violet-7
    ezColorGammaUB(121, 80, 242),  // oc-violet-6
    ezColorGammaUB(132, 94, 247),  // oc-violet-5
    ezColorGammaUB(151, 117, 250), // oc-violet-4
    ezColorGammaUB(177, 151, 252), // oc-violet-3
    ezColorGammaUB(208, 191, 255), // oc-violet-2
    ezColorGammaUB(229, 219, 255), // oc-violet-1
    ezColorGammaUB(243, 240, 255), // oc-violet-0
  },
  {
    ezColorGammaUB(54, 79, 199),   // oc-indigo-9
    ezColorGammaUB(59, 91, 219),   // oc-indigo-8
    ezColorGammaUB(66, 99, 235),   // oc-indigo-7
    ezColorGammaUB(76, 110, 245),  // oc-indigo-6
    ezColorGammaUB(92, 124, 250),  // oc-indigo-5
    ezColorGammaUB(116, 143, 252), // oc-indigo-4
    ezColorGammaUB(145, 167, 255), // oc-indigo-3
    ezColorGammaUB(186, 200, 255), // oc-indigo-2
    ezColorGammaUB(219, 228, 255), // oc-indigo-1
    ezColorGammaUB(237, 242, 255), // oc-indigo-0
  },
  {
    ezColorGammaUB(24, 100, 171),  // oc-blue-9
    ezColorGammaUB(25, 113, 194),  // oc-blue-8
    ezColorGammaUB(28, 126, 214),  // oc-blue-7
    ezColorGammaUB(34, 139, 230),  // oc-blue-6
    ezColorGammaUB(51, 154, 240),  // oc-blue-5
    ezColorGammaUB(77, 171, 247),  // oc-blue-4
    ezColorGammaUB(116, 192, 252), // oc-blue-3
    ezColorGammaUB(165, 216, 255), // oc-blue-2
    ezColorGammaUB(208, 235, 255), // oc-blue-1
    ezColorGammaUB(231, 245, 255), // oc-blue-0
  },
  {
    ezColorGammaUB(11, 114, 133),  // oc-cyan-9
    ezColorGammaUB(12, 133, 153),  // oc-cyan-8
    ezColorGammaUB(16, 152, 173),  // oc-cyan-7
    ezColorGammaUB(21, 170, 191),  // oc-cyan-6
    ezColorGammaUB(34, 184, 207),  // oc-cyan-5
    ezColorGammaUB(59, 201, 219),  // oc-cyan-4
    ezColorGammaUB(102, 217, 232), // oc-cyan-3
    ezColorGammaUB(153, 233, 242), // oc-cyan-2
    ezColorGammaUB(197, 246, 250), // oc-cyan-1
    ezColorGammaUB(227, 250, 252), // oc-cyan-0
  },
  {
    ezColorGammaUB(8, 127, 91),    // oc-teal-9
    ezColorGammaUB(9, 146, 104),   // oc-teal-8
    ezColorGammaUB(12, 166, 120),  // oc-teal-7
    ezColorGammaUB(18, 184, 134),  // oc-teal-6
    ezColorGammaUB(32, 201, 151),  // oc-teal-5
    ezColorGammaUB(56, 217, 169),  // oc-teal-4
    ezColorGammaUB(99, 230, 190),  // oc-teal-3
    ezColorGammaUB(150, 242, 215), // oc-teal-2
    ezColorGammaUB(195, 250, 232), // oc-teal-1
    ezColorGammaUB(230, 252, 245), // oc-teal-0
  },
  {
    ezColorGammaUB(43, 138, 62),   // oc-green-9
    ezColorGammaUB(47, 158, 68),   // oc-green-8
    ezColorGammaUB(55, 178, 77),   // oc-green-7
    ezColorGammaUB(64, 192, 87),   // oc-green-6
    ezColorGammaUB(81, 207, 102),  // oc-green-5
    ezColorGammaUB(105, 219, 124), // oc-green-4
    ezColorGammaUB(140, 233, 154), // oc-green-3
    ezColorGammaUB(178, 242, 187), // oc-green-2
    ezColorGammaUB(211, 249, 216), // oc-green-1
    ezColorGammaUB(235, 251, 238), // oc-green-0
  },
  {
    ezColorGammaUB(92, 148, 13),   // oc-lime-9
    ezColorGammaUB(102, 168, 15),  // oc-lime-8
    ezColorGammaUB(116, 184, 22),  // oc-lime-7
    ezColorGammaUB(130, 201, 30),  // oc-lime-6
    ezColorGammaUB(148, 216, 45),  // oc-lime-5
    ezColorGammaUB(169, 227, 75),  // oc-lime-4
    ezColorGammaUB(192, 235, 117), // oc-lime-3
    ezColorGammaUB(216, 245, 162), // oc-lime-2
    ezColorGammaUB(233, 250, 200), // oc-lime-1
    ezColorGammaUB(244, 252, 227), // oc-lime-0
  },
  {
    ezColorGammaUB(230, 119, 0),   // oc-yellow-9
    ezColorGammaUB(240, 140, 0),   // oc-yellow-8
    ezColorGammaUB(245, 159, 0),   // oc-yellow-7
    ezColorGammaUB(250, 176, 5),   // oc-yellow-6
    ezColorGammaUB(252, 196, 25),  // oc-yellow-5
    ezColorGammaUB(255, 212, 59),  // oc-yellow-4
    ezColorGammaUB(255, 224, 102), // oc-yellow-3
    ezColorGammaUB(255, 236, 153), // oc-yellow-2
    ezColorGammaUB(255, 243, 191), // oc-yellow-1
    ezColorGammaUB(255, 249, 219), // oc-yellow-0
  },
  {
    ezColorGammaUB(217, 72, 15),   // oc-orange-9
    ezColorGammaUB(232, 89, 12),   // oc-orange-8
    ezColorGammaUB(247, 103, 7),   // oc-orange-7
    ezColorGammaUB(253, 126, 20),  // oc-orange-6
    ezColorGammaUB(255, 146, 43),  // oc-orange-5
    ezColorGammaUB(255, 169, 77),  // oc-orange-4
    ezColorGammaUB(255, 192, 120), // oc-orange-3
    ezColorGammaUB(255, 216, 168), // oc-orange-2
    ezColorGammaUB(255, 232, 204), // oc-orange-1
    ezColorGammaUB(255, 244, 230), // oc-orange-0
  },
  {
    ezColorGammaUB(33, 37, 41),    // oc-gray-9
    ezColorGammaUB(52, 58, 64),    // oc-gray-8
    ezColorGammaUB(73, 80, 87),    // oc-gray-7
    ezColorGammaUB(134, 142, 150), // oc-gray-6
    ezColorGammaUB(173, 181, 189), // oc-gray-5
    ezColorGammaUB(206, 212, 218), // oc-gray-4
    ezColorGammaUB(222, 226, 230), // oc-gray-3
    ezColorGammaUB(233, 236, 239), // oc-gray-2
    ezColorGammaUB(241, 243, 245), // oc-gray-1
    ezColorGammaUB(248, 249, 250), // oc-gray-0
  },
};

// We could use a lower brightness here for our dark UI but the colors looks much nicer at higher brightness so we just apply a scale factor instead.
static constexpr ezUInt8 DarkUIBrightness = 3;
static constexpr ezUInt8 DarkUIGrayBrightness = 4; // gray is too dark at UIBrightness
static constexpr float DarkUISaturation = 0.95f;
static constexpr ezColor DarkUIFactor = ezColor(0.5f, 0.5f, 0.5f, 1.0f);
ezColor ezColorScheme::s_DarkUIColors[Count] = {
  GetColor(ezColorScheme::Red, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(ezColorScheme::Pink, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(ezColorScheme::Grape, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(ezColorScheme::Violet, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(ezColorScheme::Indigo, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(ezColorScheme::Blue, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(ezColorScheme::Cyan, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(ezColorScheme::Teal, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(ezColorScheme::Green, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(ezColorScheme::Lime, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(ezColorScheme::Yellow, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(ezColorScheme::Orange, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(ezColorScheme::Gray, DarkUIGrayBrightness, DarkUISaturation) * DarkUIFactor,
};

static constexpr ezUInt8 LightUIBrightness = 4;
static constexpr ezUInt8 LightUIGrayBrightness = 5; // gray is too dark at UIBrightness
static constexpr float LightUISaturation = 1.0f;
ezColor ezColorScheme::s_LightUIColors[Count] = {
  GetColor(ezColorScheme::Red, LightUIBrightness, LightUISaturation),
  GetColor(ezColorScheme::Pink, LightUIBrightness, LightUISaturation),
  GetColor(ezColorScheme::Grape, LightUIBrightness, LightUISaturation),
  GetColor(ezColorScheme::Violet, LightUIBrightness, LightUISaturation),
  GetColor(ezColorScheme::Indigo, LightUIBrightness, LightUISaturation),
  GetColor(ezColorScheme::Blue, LightUIBrightness, LightUISaturation),
  GetColor(ezColorScheme::Cyan, LightUIBrightness, LightUISaturation),
  GetColor(ezColorScheme::Teal, LightUIBrightness, LightUISaturation),
  GetColor(ezColorScheme::Green, LightUIBrightness, LightUISaturation),
  GetColor(ezColorScheme::Lime, LightUIBrightness, LightUISaturation),
  GetColor(ezColorScheme::Yellow, LightUIBrightness, LightUISaturation),
  GetColor(ezColorScheme::Orange, LightUIBrightness, LightUISaturation),
  GetColor(ezColorScheme::Gray, LightUIGrayBrightness, LightUISaturation),
};

// static
ezColor ezColorScheme::GetColor(float fIndex, ezUInt8 uiBrightness, float fSaturation /*= 1.0f*/, float fAlpha /*= 1.0f*/)
{
  ezUInt32 uiIndexA, uiIndexB;
  float fFrac;
  GetInterpolation(fIndex, uiIndexA, uiIndexB, fFrac);

  const ezColor a = s_Colors[uiIndexA][uiBrightness];
  const ezColor b = s_Colors[uiIndexB][uiBrightness];
  const ezColor c = ezMath::Lerp(a, b, fFrac);
  const float l = c.GetLuminance();
  return ezMath::Lerp(ezColor(l, l, l), c, fSaturation).WithAlpha(fAlpha);
}

ezColorScheme::CategoryColorFunc ezColorScheme::s_CategoryColorFunc = nullptr;

ezColor ezColorScheme::GetCategoryColor(ezStringView sCategory, CategoryColorUsage usage)
{
  if (s_CategoryColorFunc != nullptr)
  {
    return s_CategoryColorFunc(sCategory, usage);
  }

  ezInt8 iBrightnessOffset = -3;
  ezUInt8 uiSaturationStep = 0;

  if (usage == ezColorScheme::CategoryColorUsage::BorderIconColor)
  {
    // don't color these icons at all
    return ezColor::MakeZero();
  }

  if (usage == ezColorScheme::CategoryColorUsage::MenuEntryIcon || usage == ezColorScheme::CategoryColorUsage::AssetMenuIcon)
  {
    iBrightnessOffset = 2;
    uiSaturationStep = 0;
  }
  else if (usage == ezColorScheme::CategoryColorUsage::ViewportIcon)
  {
    iBrightnessOffset = 2;
    uiSaturationStep = 2;
  }
  else if (usage == ezColorScheme::CategoryColorUsage::OverlayIcon)
  {
    iBrightnessOffset = 2;
    uiSaturationStep = 0;
  }
  else if (usage == ezColorScheme::CategoryColorUsage::SceneTreeIcon)
  {
    iBrightnessOffset = 2;
    uiSaturationStep = 0;
  }
  else if (usage == ezColorScheme::CategoryColorUsage::BorderColor)
  {
    iBrightnessOffset = -3;
    uiSaturationStep = 0;
  }

  const ezUInt8 uiBrightness = (ezUInt8)ezMath::Clamp<ezInt32>(DarkUIBrightness + iBrightnessOffset, 0, 9);
  const float fSaturation = DarkUISaturation - (uiSaturationStep * 0.2f);

  if (const char* sep = sCategory.FindSubString("/"))
  {
    // chop off everything behind the first separator
    sCategory = ezStringView(sCategory.GetStartPointer(), sep);
  }

  if (sCategory.IsEqual_NoCase("AI"))
    return ezColorScheme::GetColor(ezColorScheme::Cyan, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Animation"))
    return ezColorScheme::GetColor(ezColorScheme::Pink, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Construction"))
    return ezColorScheme::GetColor(ezColorScheme::Orange, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Custom") || sCategory.IsEqual_NoCase("Game"))
    return ezColorScheme::GetColor(ezColorScheme::Red, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Effects"))
    return ezColorScheme::GetColor(ezColorScheme::Grape, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Gameplay"))
    return ezColorScheme::GetColor(ezColorScheme::Indigo, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Input"))
    return ezColorScheme::GetColor(ezColorScheme::Red, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Lighting"))
    return ezColorScheme::GetColor(ezColorScheme::Violet, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Logic"))
    return ezColorScheme::GetColor(ezColorScheme::Teal, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Physics"))
    return ezColorScheme::GetColor(ezColorScheme::Blue, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Prefabs"))
    return ezColorScheme::GetColor(ezColorScheme::Orange, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Rendering"))
    return ezColorScheme::GetColor(ezColorScheme::Lime, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Terrain"))
    return ezColorScheme::GetColor(ezColorScheme::Lime, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Scripting"))
    return ezColorScheme::GetColor(ezColorScheme::Green, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Sound"))
    return ezColorScheme::GetColor(ezColorScheme::Blue, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Utilities") || sCategory.IsEqual_NoCase("Editing"))
    return ezColorScheme::GetColor(ezColorScheme::Gray, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("XR"))
    return ezColorScheme::GetColor(ezColorScheme::Cyan, uiBrightness, fSaturation) * DarkUIFactor;

  ezLog::Warning("Color for category '{}' is undefined.", sCategory);
  return ezColor::MakeZero();
}


