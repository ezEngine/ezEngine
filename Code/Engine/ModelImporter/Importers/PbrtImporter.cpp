#include <PCH.h>
#include <ModelImporter/Importers/PbrtImporter.h>
#include <ModelImporter/Importers/PbrtImporter_Context.h>
#include <ModelImporter/Importers/PbrtImporter_ParsingFunctions.h>
#include <ModelImporter/Importers/PbrtImporter_ImportFunctions.h>
#include <ModelImporter/Scene.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Logging/Log.h>

namespace ezModelImporter
{
  using namespace Pbrt;

  namespace PbrtCommandLookup
  {
    // Reads a readily parsed object into the scene.
    typedef void(*ReadObjectFunc)(ezStringView type, ezArrayPtr<Parameter> parameters, ParseContext& context, ezModelImporter::Scene& outScene);
    // Scopes have some effect on the context, but don't have any additional data.
    typedef void(*ReadScopeFunc)(ParseContext& context);
    // Transform applies itself to the top element in the transform stack.
    // Parsing is different for each transform.
    typedef void(*ParseTransformFunc)(ParseContext& context, ezStringView& remainingSceneText);

    bool s_initialized = false;
    ezHashTable<ezString, ReadScopeFunc> s_scopes;
    ezHashTable<ezString, ParseTransformFunc> s_transforms;
    ezHashTable<ezString, ReadObjectFunc> s_objects;
  }


  PbrtImporter::PbrtImporter()
  {
    if (!PbrtCommandLookup::s_initialized)
    {
      // All in lower case since we convert the incoming string to lower case.

      // Scopes.
      PbrtCommandLookup::s_scopes.Insert("worldbegin", &PbrtScopeFunctions::WorldBegin);
      PbrtCommandLookup::s_scopes.Insert("worldend", &PbrtScopeFunctions::WorldEnd);
      PbrtCommandLookup::s_scopes.Insert("attributebegin", &PbrtScopeFunctions::AttributeBegin);
      PbrtCommandLookup::s_scopes.Insert("attributeend", &PbrtScopeFunctions::AttributeEnd);
      PbrtCommandLookup::s_scopes.Insert("transformbegin", &PbrtScopeFunctions::TransformBegin);
      PbrtCommandLookup::s_scopes.Insert("transformend", &PbrtScopeFunctions::TransformEnd);
      PbrtCommandLookup::s_scopes.Insert("objectend", &PbrtScopeFunctions::ObjectEnd);
      // Known missing:
      // * ActiveTransform & TransformTimes

      // Transforms
      PbrtCommandLookup::s_transforms.Insert("identity", &PbrtTransformFunctions::Identity);
      PbrtCommandLookup::s_transforms.Insert("translate", &PbrtTransformFunctions::Translate);
      PbrtCommandLookup::s_transforms.Insert("rotate", &PbrtTransformFunctions::Rotate);
      PbrtCommandLookup::s_transforms.Insert("scale", &PbrtTransformFunctions::Scale);
      PbrtCommandLookup::s_transforms.Insert("lookat", &PbrtTransformFunctions::LookAt);
      PbrtCommandLookup::s_transforms.Insert("transform", &PbrtTransformFunctions::Transform);
      PbrtCommandLookup::s_transforms.Insert("concattransform", &PbrtTransformFunctions::ConcatTransform);

      // Known missing:
      // * CoordinateSystem
      // * CoordSysTransform

      // Objects.
      PbrtCommandLookup::s_objects.Insert("shape", &PbrtObjectParseFunctions::Shape);
      PbrtCommandLookup::s_objects.Insert("material", &PbrtObjectParseFunctions::Material);
      PbrtCommandLookup::s_objects.Insert("makenamedmaterial", &PbrtObjectParseFunctions::MakeNamedMaterial);
      PbrtCommandLookup::s_objects.Insert("namedmaterial", &PbrtObjectParseFunctions::NamedMaterial);
      PbrtCommandLookup::s_objects.Insert("material", &PbrtObjectParseFunctions::Material);
      PbrtCommandLookup::s_objects.Insert("texture", &PbrtObjectParseFunctions::Texture);
      PbrtCommandLookup::s_objects.Insert("objectbegin", &PbrtObjectParseFunctions::ObjectBegin);
      PbrtCommandLookup::s_objects.Insert("objectinstance", &PbrtObjectParseFunctions::ObjectInstance);
      // Known missing:
      // * Film
      // * Sampler
      // * Camera
      // * MakeNamedMaterial
      // * NamedMAterial

      PbrtCommandLookup::s_initialized = true;
    }
  }

  ezArrayPtr<const ezString> PbrtImporter::GetSupportedFileFormats() const
  {
    static const ezString extension = "pbrt";
    return ezMakeArrayPtr(&extension, 1);
  }

  void ImportSceneImpl(const char* szFileName, ParseContext& context, Scene& outScene)
  {
    ezStringBuilder content;

    {
      ezFileReader sceneFile;
      if (sceneFile.Open(szFileName).Failed())
      {
        ezLog::Error("Failed to open '{0}'", szFileName);
        return;
      }

      content.ReadAll(sceneFile);
      sceneFile.Close();
    }
    ezString sceneFolderPath = ezPathUtils::GetFileDirectory(szFileName);

    // Read pbrt file command by command.
    // A command is either an object, a scope or a transform.
    //
    // Object: Always have a name and a parameterlist. Call function in s_objects to store data in the scene.
    // Scope: No additional stuff but influences the context object.
    // Transform: Changes the top level transform on the transform stack. Call function in s_transforms to parse as well.

    ezDynamicArray<Parameter> parameterList;

    ezStringView remainingSceneText = content;
    while (remainingSceneText.IsValid())
    {
      ezStringView commandName = PbrtParseHelper::ReadCommandName(remainingSceneText);

      // Is it an include? (special)
      if (commandName == "include")
      {
        ezStringBuilder includeFilename = sceneFolderPath;
        includeFilename.AppendPath(ezString(PbrtParseHelper::ReadBlock(remainingSceneText, '\"', '\"')));
        ImportSceneImpl(includeFilename.GetData(), context, outScene);
      }

      // Is it an object?
      else if (PbrtCommandLookup::s_objects.Contains(commandName))
      {
        // Extract type.
        ezStringView type = PbrtParseHelper::ReadBlock(remainingSceneText, '\"', '\"');
        if (!type.IsValid())
        {
          ezLog::Error("Object '{0}' in pbrt file '{1}' has no type specifier.", commandName, szFileName);
          continue;
        }

        // Extract parameters.
        parameterList.Clear();
        ezStringView parameterDesc = PbrtParseHelper::ReadBlock(remainingSceneText, '\"', '\"');
        while (parameterDesc.IsValid())
        {
          // Usually two strings, type and name.
          auto& parameter = parameterList.ExpandAndGetRef();

          const char* separatorPos = parameterDesc.FindSubString(" ");
          if (separatorPos == nullptr)
          {
            parameter.type = ParamType::INVALID;
            parameter.name = parameterDesc;
            //parameter.data;
          }
          else
          {
            ezStringView paramTypeString(parameterDesc.GetData(), separatorPos);
            ParamType paramType = PbrtParseHelper::GetParamType(paramTypeString);
            if (paramType == ParamType::INVALID)
            {
              ezString paramTypeStringInst = paramTypeString;
              ezLog::Error("Unknown parameter type '{0}' in pbrt file '{1}' for an object '{2}'.", paramTypeStringInst, commandName, szFileName);
              break;
            }

            parameter.type = paramType;
            parameter.name = ezStringView(separatorPos + 1, parameterDesc.GetEndPosition());
            parameter.data = PbrtParseHelper::ParseParameterBlock(paramType, remainingSceneText);
          }

          // Next parameter.
          parameterDesc = PbrtParseHelper::ReadBlock(remainingSceneText, '\"', '\"');
        }

        PbrtCommandLookup::s_objects[commandName](type, parameterList, context, outScene);
      }

      // Is it a transform?
      else if (PbrtCommandLookup::s_transforms.Contains(commandName))
      {
        PbrtCommandLookup::s_transforms[commandName](context, remainingSceneText);
      }

      // Is it a scope?
      else if (PbrtCommandLookup::s_scopes.Contains(commandName))
      {
        PbrtCommandLookup::s_scopes[commandName](context);
      }

      else
      {
        ezLog::Warning("Unknown Pbrt command '{0}'.", commandName);
      }

      // Skip lines until something does not start like a parameter.
      PbrtParseHelper::SkipWhiteSpaces(remainingSceneText);
      while (remainingSceneText.IsValid() &&
              !(remainingSceneText.GetCharacter() >= 'A' && remainingSceneText.GetCharacter() <= 'Z') &&
              !(remainingSceneText.GetCharacter() >= 'a' && remainingSceneText.GetCharacter() <= 'z'))
      {
        PbrtParseHelper::SkipWhiteSpaces(remainingSceneText);
        PbrtParseHelper::SkipCurrentLine(remainingSceneText);
        PbrtParseHelper::SkipWhiteSpaces(remainingSceneText);
      }
    }
  }

  ezSharedPtr<Scene> PbrtImporter::ImportScene(const char* szFileName)
  {
    ezLogBlock("Load Pbrt scene", szFileName);
    ezSharedPtr<Scene> outScene = EZ_DEFAULT_NEW(Scene);
    ParseContext context(szFileName);

    ImportSceneImpl(szFileName, context, *outScene);

    if (context.IsInWorld())
    {
      ezLog::Error("Missing 'WorldEnd' in pbrt file '{0}'.", szFileName);
    }

    return std::move(outScene);
  }
}
