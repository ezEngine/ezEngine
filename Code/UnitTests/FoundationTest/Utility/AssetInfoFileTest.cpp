#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Utilities/AssetInfoFile.h>

EZ_CREATE_SIMPLE_TEST(Utility, AssetInfoFile)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetValue / GetValue")
  {
    ezAssetInfoFile info;
    EZ_TEST_BOOL(info.IsEmpty());

    info.SetValue(ezAssetInfoFile::Keys::NumTriangles, 1234u);
    EZ_TEST_BOOL(!info.IsEmpty());
    EZ_TEST_INT(info.GetValue(ezAssetInfoFile::Keys::NumTriangles).Get<ezUInt32>(), 1234);

    // overwriting replaces the value instead of adding a second entry
    info.SetValue(ezAssetInfoFile::Keys::NumTriangles, 42u);
    EZ_TEST_INT(info.GetValue(ezAssetInfoFile::Keys::NumTriangles).Get<ezUInt32>(), 42);
    EZ_TEST_INT(info.GetValues().GetCount(), 1);

    // an unknown key yields an invalid variant, rather than asserting
    EZ_TEST_BOOL(!info.GetValue("DoesNotExist").IsValid());

    // an invalid value removes the key again
    info.SetValue(ezAssetInfoFile::Keys::NumTriangles, ezVariant());
    EZ_TEST_BOOL(info.IsEmpty());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Write / Read")
  {
    ezAssetInfoFile info;
    info.SetValue(ezAssetInfoFile::Keys::NumVertices, 100u);
    info.SetValue(ezAssetInfoFile::Keys::NumTriangles, 50u);
    // not exactly representable, to cover the writer's readable float mode
    info.SetValue(ezAssetInfoFile::Keys::BoundsCenter, ezVec3(0.477654f, -1.2345678f, 3));
    info.SetValue(ezAssetInfoFile::Keys::BoundsHalfExtents, ezVec3(4, 5, 6));
    info.SetValue(ezAssetInfoFile::Keys::Format, ezString("BC7"));
    info.SetValue(ezAssetInfoFile::Keys::CollisionMeshType, ezString("ConvexHull"));

    ezAssetFileHeader header;
    header.SetFileHashAndVersion(0x1234567890ABCDEFull, 7);

    ezDefaultMemoryStreamStorage storage;
    {
      ezMemoryStreamWriter writer(&storage);
      EZ_TEST_BOOL(info.Write(writer, header).Succeeded());
    }

    ezAssetInfoFile read;
    ezAssetFileHeader readHeader;
    {
      ezMemoryStreamReader reader(&storage);
      EZ_TEST_BOOL(read.Read(reader, readHeader).Succeeded());
    }

    EZ_TEST_BOOL(readHeader.IsFileUpToDate(0x1234567890ABCDEFull, 7));
    EZ_TEST_INT(read.GetValues().GetCount(), 6);
    EZ_TEST_INT(read.GetValue(ezAssetInfoFile::Keys::NumVertices).Get<ezUInt32>(), 100);
    EZ_TEST_INT(read.GetValue(ezAssetInfoFile::Keys::NumTriangles).Get<ezUInt32>(), 50);
    EZ_TEST_VEC3(read.GetValue(ezAssetInfoFile::Keys::BoundsCenter).Get<ezVec3>(), ezVec3(0.477654f, -1.2345678f, 3), 0.001f);
    EZ_TEST_VEC3(read.GetValue(ezAssetInfoFile::Keys::BoundsHalfExtents).Get<ezVec3>(), ezVec3(4, 5, 6), 0.0f);
    EZ_TEST_STRING(read.GetValue(ezAssetInfoFile::Keys::Format).Get<ezString>(), "BC7");
    EZ_TEST_STRING(read.GetValue(ezAssetInfoFile::Keys::CollisionMeshType).Get<ezString>(), "ConvexHull");

    // reading replaces previous content rather than merging into it
    {
      ezMemoryStreamReader reader(&storage);
      EZ_TEST_BOOL(read.Read(reader, readHeader).Succeeded());
    }
    EZ_TEST_INT(read.GetValues().GetCount(), 6);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "String array round trip")
  {
    ezVariantArray clips;
    clips.PushBack(ezVariant(ezString("Idle")));
    clips.PushBack(ezVariant(ezString("Walk")));
    clips.PushBack(ezVariant(ezString("Run")));

    ezAssetInfoFile info;
    info.SetValue(ezAssetInfoFile::Keys::AvailableClips, ezVariant(clips));

    ezAssetFileHeader header;
    header.SetFileHashAndVersion(1, 1);

    ezDefaultMemoryStreamStorage storage;
    {
      ezMemoryStreamWriter writer(&storage);
      EZ_TEST_BOOL(info.Write(writer, header).Succeeded());
    }

    ezAssetInfoFile read;
    ezAssetFileHeader readHeader;
    {
      ezMemoryStreamReader reader(&storage);
      EZ_TEST_BOOL(read.Read(reader, readHeader).Succeeded());
    }

    const ezVariant value = read.GetValue(ezAssetInfoFile::Keys::AvailableClips);
    EZ_TEST_BOOL(value.IsA<ezVariantArray>());

    const ezVariantArray& readClips = value.Get<ezVariantArray>();
    EZ_TEST_INT(readClips.GetCount(), 3);
    EZ_TEST_STRING(readClips[0].ConvertTo<ezString>(), "Idle");
    EZ_TEST_STRING(readClips[1].ConvertTo<ezString>(), "Walk");
    EZ_TEST_STRING(readClips[2].ConvertTo<ezString>(), "Run");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AppendToDisplayString")
  {
    // nothing is appended for an empty set, not even a separator
    {
      ezAssetInfoFile info;
      ezStringBuilder s("Existing");
      info.AppendToDisplayString(s);
      EZ_TEST_STRING(s, "Existing");
    }

    // center and half extents collapse into a size and a range, rather than being listed raw
    {
      ezAssetInfoFile info;
      info.SetValue(ezAssetInfoFile::Keys::NumTriangles, 252u);
      info.SetValue(ezAssetInfoFile::Keys::BoundsCenter, ezVec3(0, 0, 0.5f));
      info.SetValue(ezAssetInfoFile::Keys::BoundsHalfExtents, ezVec3(0.25f, 0.25f, 0.5f));
      info.SetValue(ezAssetInfoFile::Keys::BoundsRadius, 0.75f);

      ezStringBuilder s;
      info.AppendToDisplayString(s, "\n");

      EZ_TEST_BOOL(s.FindSubString("Size: 0.500 x 0.500 x 1.000") != nullptr);
      EZ_TEST_BOOL(s.FindSubString("Bounds: -0.250 -0.250 0.000 to 0.250 0.250 1.000") != nullptr);
      EZ_TEST_BOOL(s.FindSubString("Triangles: 252") != nullptr);
      EZ_TEST_BOOL(s.FindSubString("Radius: 0.75") != nullptr);
      // the keys folded into the lines above must not also appear on their own
      EZ_TEST_BOOL(s.FindSubString("Center:") == nullptr);
      EZ_TEST_BOOL(s.FindSubString("Extents:") == nullptr);
    }

    // the bounds range needs both halves, so a center on its own prints nothing
    {
      ezAssetInfoFile info;
      info.SetValue(ezAssetInfoFile::Keys::BoundsCenter, ezVec3(1, 2, 3));

      ezStringBuilder s;
      info.AppendToDisplayString(s);
      EZ_TEST_BOOL(s.IsEmpty());
    }

    // width and height collapse into a single resolution line
    {
      ezAssetInfoFile info;
      info.SetValue(ezAssetInfoFile::Keys::ImageWidth, 512u);
      info.SetValue(ezAssetInfoFile::Keys::ImageHeight, 256u);
      info.SetValue(ezAssetInfoFile::Keys::Format, ezString("BC7"));

      ezStringBuilder s;
      info.AppendToDisplayString(s);

      EZ_TEST_BOOL(s.FindSubString("Resolution: 512 x 256") != nullptr);
      EZ_TEST_BOOL(s.FindSubString("Format: BC7") != nullptr);
      EZ_TEST_BOOL(s.FindSubString("Width:") == nullptr);
    }

    // a key this code does not know about is still shown, under its raw name
    {
      ezAssetInfoFile info;
      info.SetValue("SomethingNew", 42u);

      ezStringBuilder s;
      info.AppendToDisplayString(s);
      EZ_TEST_BOOL(s.FindSubString("SomethingNew: 42") != nullptr);
    }

    // an array is summarized by its element count, not by listing the elements
    {
      ezVariantArray clips;
      clips.PushBack(ezVariant(ezString("Idle")));
      clips.PushBack(ezVariant(ezString("Walk")));

      ezAssetInfoFile info;
      info.SetValue(ezAssetInfoFile::Keys::AvailableClips, ezVariant(clips));

      ezStringBuilder s;
      info.AppendToDisplayString(s);

      EZ_TEST_BOOL(s.FindSubString("Clips: 2") != nullptr);
      EZ_TEST_BOOL(s.FindSubString("Idle") == nullptr);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AppendValuesToDisplayString")
  {
    ezAssetInfoFile info;
    info.SetValue(ezAssetInfoFile::Keys::NumVertices, 4460u);
    info.SetValue(ezAssetInfoFile::Keys::NumTriangles, 5238u);
    info.SetValue(ezAssetInfoFile::Keys::NumSubMeshes, 1u);
    info.SetValue(ezAssetInfoFile::Keys::BoundsCenter, ezVec3(0, 0, 0.5f));
    info.SetValue(ezAssetInfoFile::Keys::BoundsHalfExtents, ezVec3(0.25f, 0.25f, 0.5f));

    // only the requested keys appear, so that a caller can keep a tooltip short
    {
      const ezStringView keys[] = {ezAssetInfoFile::Keys::NumTriangles, ezAssetInfoFile::Keys::BoundsHalfExtents};

      ezStringBuilder s;
      info.AppendValuesToDisplayString(s, keys);

      EZ_TEST_BOOL(s.FindSubString("Triangles: 5238") != nullptr);
      EZ_TEST_BOOL(s.FindSubString("Size: 0.500 x 0.500 x 1.000") != nullptr);
      EZ_TEST_BOOL(s.FindSubString("Vertices:") == nullptr);
      EZ_TEST_BOOL(s.FindSubString("Meshes:") == nullptr);
      // asking for the size must not also print the full coordinate range
      EZ_TEST_BOOL(s.FindSubString("Bounds:") == nullptr);
    }

    // the requested order is kept, rather than the order inside the file
    {
      const ezStringView keys[] = {ezAssetInfoFile::Keys::NumSubMeshes, ezAssetInfoFile::Keys::NumVertices};

      ezStringBuilder s;
      info.AppendValuesToDisplayString(s, keys);

      const char* szSub = s.FindSubString("Meshes:");
      const char* szVert = s.FindSubString("Vertices:");
      EZ_TEST_BOOL(szSub != nullptr && szVert != nullptr && szSub < szVert);
    }

    // a key with no value is skipped instead of printing an empty line
    {
      const ezStringView keys[] = {ezAssetInfoFile::Keys::Format, ezAssetInfoFile::Keys::NumTriangles};

      ezStringBuilder s;
      info.AppendValuesToDisplayString(s, keys);

      EZ_TEST_BOOL(s.FindSubString("Format:") == nullptr);
      EZ_TEST_STRING(s, "\nTriangles: 5238");
    }

    // asking for the absorbed half of a combined pair yields nothing on its own
    {
      ezAssetInfoFile tex;
      tex.SetValue(ezAssetInfoFile::Keys::ImageWidth, 512u);
      tex.SetValue(ezAssetInfoFile::Keys::ImageHeight, 256u);

      ezStringBuilder sHeight;
      const ezStringView keysHeight[] = {ezAssetInfoFile::Keys::ImageHeight};
      EZ_TEST_BOOL(!tex.AppendValueToDisplayString(sHeight, ezAssetInfoFile::Keys::ImageHeight));
      tex.AppendValuesToDisplayString(sHeight, keysHeight);
      EZ_TEST_BOOL(sHeight.IsEmpty());

      // while the other half prints the whole resolution
      ezStringBuilder sWidth;
      EZ_TEST_BOOL(tex.AppendValueToDisplayString(sWidth, ezAssetInfoFile::Keys::ImageWidth));
      EZ_TEST_BOOL(sWidth.FindSubString("Resolution: 512 x 256") != nullptr);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetInfoFilePathForOutput")
  {
    EZ_TEST_STRING(ezAssetInfoFile::GetInfoFilePathForOutput("C:/Foo/Bar.ezMesh"), "C:/Foo/Bar.ezMesh.ezAssetInfo");
  }
}
