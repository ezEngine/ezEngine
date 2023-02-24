#include <TexConv/TexConvPCH.h>

#include "NoiseGen.h"

ezNoiseGen::ezNoiseGen()
  : ezApplication("NoiseGen")
{
}

#define UI0 1597334673U
#define UI1 3812015801U
#define UI2 ezVec2U32(UI0, UI1)
#define UI3 ezVec3U32(UI0, UI1, 2798796415U)
#define UIF (1.0f / float(0xffffffffU))

ezVec3 hash33(ezVec3 p)
{
  ezVec3U32 q = ezVec3U32(static_cast<uint32_t>(p.x), static_cast<uint32_t>(p.y), static_cast<uint32_t>(p.z)).CompMul(UI3);
  q = (q.x ^ q.y ^ q.z) * UI3;
  return ezVec3(-1.0f) + (ezVec3(static_cast<float>(q.x), static_cast<float>(q.y), static_cast<float>(q.z)) * (UIF * 2.0f));
}

// Gradient noise by iq (modified to be tileable)
float gradientNoise(ezVec3 x, float freq)
{
  // grid
  ezVec3 p(ezMath::Floor(x.x), ezMath::Floor(x.y), ezMath::Floor(x.z));
  ezVec3 w(ezMath::Fraction(x.x), ezMath::Fraction(x.y), ezMath::Fraction(x.z));

  // quintic interpolant
  ezVec3 u = w.CompMul(w).CompMul(w).CompMul(w.CompMul(w * 6.0f - ezVec3(15.0f)) + ezVec3(10.0f));

  // gradients
  ezVec3 ga = hash33((p + ezVec3(0.0f, 0.0f, 0.0f)).Mod(freq));
  ezVec3 gb = hash33((p + ezVec3(1.0f, 0.0f, 0.0f)).Mod(freq));
  ezVec3 gc = hash33((p + ezVec3(0.0f, 1.0f, 0.0f)).Mod(freq));
  ezVec3 gd = hash33((p + ezVec3(1.0f, 1.0f, 0.0f)).Mod(freq));
  ezVec3 ge = hash33((p + ezVec3(0.0f, 0.0f, 1.0f)).Mod(freq));
  ezVec3 gf = hash33((p + ezVec3(1.0f, 0.0f, 1.0f)).Mod(freq));
  ezVec3 gg = hash33((p + ezVec3(0.0f, 1.0f, 1.0f)).Mod(freq));
  ezVec3 gh = hash33((p + ezVec3(1.0f, 1.0f, 1.0f)).Mod(freq));

  // projections
  float va = ga.Dot(w - ezVec3(0., 0., 0.));
  float vb = gb.Dot(w - ezVec3(1., 0., 0.));
  float vc = gc.Dot(w - ezVec3(0., 1., 0.));
  float vd = gd.Dot(w - ezVec3(1., 1., 0.));
  float ve = ge.Dot(w - ezVec3(0., 0., 1.));
  float vf = gf.Dot(w - ezVec3(1., 0., 1.));
  float vg = gg.Dot(w - ezVec3(0., 1., 1.));
  float vh = gh.Dot(w - ezVec3(1., 1., 1.));

  // interpolation
  return va +
         u.x * (vb - va) +
         u.y * (vc - va) +
         u.z * (ve - va) +
         u.x * u.y * (va - vb - vc + vd) +
         u.y * u.z * (va - vc - ve + vg) +
         u.z * u.x * (va - vb - ve + vf) +
         u.x * u.y * u.z * (-va + vb + vc - vd + ve - vf - vg + vh);
}

// Fbm for Perlin noise based on iq's blog
float perlinfbm(ezVec3 p, float freq, int octaves)
{
  float G = exp2(-.85);
  float amp = 1.;
  float noise = 0.;
  for (int i = 0; i < octaves; ++i)
  {
    noise += amp * gradientNoise(p * freq, freq);
    freq *= 2.;
    amp *= G;
  }

  return noise;
}

// Tileable 3D worley noise
float worleyNoise(ezVec3 uv, float freq)
{
  ezVec3 id(ezMath::Floor(uv.x), ezMath::Floor(uv.y), ezMath::Floor(uv.z));
  ezVec3 p(ezMath::Fraction(uv.x), ezMath::Fraction(uv.y), ezMath::Fraction(uv.z));;

  float minDist = 10000.0f;
  for (float x = -1.; x <= 1.; ++x)
  {
    for (float y = -1.; y <= 1.; ++y)
    {
      for (float z = -1.; z <= 1.; ++z)
      {
        ezVec3 offset = ezVec3(x, y, z);
        ezVec3 h = hash33((id + offset).Mod(freq)) * 0.5f + ezVec3(0.5f);
        h += offset;
        ezVec3 d = p - h;
        minDist = ezMath::Min(minDist, d.Dot(d));
      }
    }
  }

  // inverted worley noise
  return 1. - minDist;
}

// Tileable Worley fbm inspired by Andrew Schneider's Real-Time Volumetric Cloudscapes
// chapter in GPU Pro 7.
float worleyFbm(ezVec3 p, float freq)
{
  return worleyNoise(p * freq, freq) * 0.625f +
         worleyNoise(p * freq * 2.0f, freq * 2.0f) * 0.25f +
         worleyNoise(p * freq * 4.0f, freq * 4.0f) * 0.125f;
}

float remap(float x, float a, float b, float c, float d)
{
  return (((x - a) / (b - a)) * (d - c)) + c;
}

ezResult ezNoiseGen::BeforeCoreSystemsStartup()
{
  ezStartup::AddApplicationTag("tool");
  ezStartup::AddApplicationTag("noisegen");

  return SUPER::BeforeCoreSystemsStartup();
}

void ezNoiseGen::AfterCoreSystemsStartup()
{
  ezFileSystem::AddDataDirectory("", "App", ":", ezFileSystem::AllowWrites).IgnoreResult();

  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  // Add the empty data directory to access files via absolute paths
  ezFileSystem::AddDataDirectory("", "App", ":", ezFileSystem::AllowWrites).IgnoreResult();
}

void ezNoiseGen::BeforeCoreSystemsShutdown()
{
  ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  SUPER::BeforeCoreSystemsShutdown();
}

uint8_t floatToUint8(float value)
{
  return static_cast<uint8_t>(ezMath::Clamp(value * 255.0f, 0.0f, 255.0f));
}


ezApplication::Execution ezNoiseGen::Run()
{
  SetReturnCode(-1);

  ezImageHeader header;
  header.SetWidth(128);
  header.SetHeight(128);
  header.SetDepth(128);
  header.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);

  ezImage noiseCube;
  noiseCube.ResetAndAlloc(header);

  const float halfPixelSize = 0.5f / 128.0f;
  const float invSize = 1.0f / 128.0f;

  float frequency = 4.0f;

  for(uint32_t slice = 0; slice < 128; slice++)
  {
    float uvZ = halfPixelSize + static_cast<float>(slice) * invSize;
    for(uint32_t y =0; y < 128; y++)
    {
      auto curPixel = noiseCube.GetPixelPointer<ezColorLinearUB>(0, 0, 0, 0, y, slice);
      float uvY = halfPixelSize + static_cast<float>(y) * invSize;
      for(uint32_t x=0; x < 128; x++,curPixel++)
      {
        float uvX = halfPixelSize + static_cast<float>(x) * invSize;

        const ezVec3 uvw(uvX, uvY, uvZ);

        float perlinFbm = ezMath::Lerp(1.0f, perlinfbm(uvw, frequency, 7), 0.5f);
        perlinFbm = ezMath::Abs(perlinFbm * 2.0f - 1.0f);

        *curPixel = {};
        curPixel->r = floatToUint8(perlinFbm);
        curPixel->g = floatToUint8(worleyFbm(uvw, frequency));
        curPixel->b = floatToUint8(worleyFbm(uvw, frequency * 2.0f));
        curPixel->a = floatToUint8(worleyFbm(uvw, frequency * 4.0f));
      }
    }
  }

  ezStringBuilder appDir = ezOSFile::GetCurrentWorkingDirectory();
  appDir.AppendPath("output.dds");

  if(noiseCube.SaveTo(appDir).Failed())
  {
    ezLog::Error("Failed to write result to output.dds");
  }

  /*    vec2 uv = fragCoord / iResolution.xy;
    vec2 m = iMouse.xy / iResolution.xy;

    vec4 col = vec4(0.);
    
    float slices = 128.; // number of layers of the 3d texture
    float freq = 4.;
    
    float pfbm= mix(1., perlinfbm(vec3(uv, floor(m.y*slices)/slices), 4., 7), .5);
    pfbm = abs(pfbm * 2. - 1.); // billowy perlin noise
    
    col.g += worleyFbm(vec3(uv, floor(m.y*slices)/slices), freq);
    col.b += worleyFbm(vec3(uv, floor(m.y*slices)/slices), freq*2.);
    col.a += worleyFbm(vec3(uv, floor(m.y*slices)/slices), freq*4.);
    col.r += remap(pfbm, 0., 1., col.g, 1.); // perlin-worley
    
    fragColor = vec4(col);
    */

  SetReturnCode(0);
  return ezApplication::Execution::Quit;
}

EZ_CONSOLEAPP_ENTRY_POINT(ezNoiseGen);
