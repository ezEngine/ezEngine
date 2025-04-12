//===--- MemberVarCheckCheck.cpp - clang-tidy -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "NameCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

#include "clang/Basic/CharInfo.h"
#include <stdio.h>

using namespace clang::ast_matchers;

namespace clang
{
  namespace tidy
  {
    namespace ez
    {

      void StripToBaseName(std::string& name)
      {
        if (name.size() > 2 && name[1] == '_')
        {
          name.erase(0, 2);
        }
        else if (name.size() > 1 && name[0] == '_')
        {
          name.erase(0, 1);
        }
      }

      std::string StripPrefix(std::string& name)
      {
        std::string result;
        if (name.size() == 0 || isUppercase(name[0]) || isDigit(name[0]))
        {
          return {};
        }
        if (name.size() > 1 && (isUppercase(name[1]) || isDigit(name[1])))
        {
          // x, y and z are not considered invalid prefixes and should be kept.
          if (name[0] != 'x' && name[0] != 'y' && name[0] != 'z')
          {
            result = std::string(name.begin(), name.begin() + 1);
            name.erase(0, 1);
          }
        }
        else if (name.size() > 2 && (isUppercase(name[2]) || isDigit(name[2])))
        {
          result = std::string(name.begin(), name.begin() + 2);
          if (result == "ui" || result == "pp" || result == "sz")
          {
            name.erase(0, 2);
          }
          else
          {
            return {};
          }
        }
        return result;
      }

      std::string AddPrefixForType(clang::StringRef typeName, std::string newName, bool* prefixAdded = nullptr)
      {
        if (typeName.endswith("Handle"))
        {
          if (prefixAdded)
            *prefixAdded = true;
          return newName.insert(0, "h");
        }
        else if (typeName == "ezSharedPtr" || typeName == "ezUniquePtr" ||
                 typeName == "ezPointerWithFlags" || typeName == "QPointer" ||
                 typeName == "shared_ptr" || typeName == "unique_ptr")
        {
          if (prefixAdded)
            *prefixAdded = true;
          return newName.insert(0, "p");
        }
        else if (typeName == "ezStringView" || typeName == "ezHybridString" ||
                 typeName == "ezHashedString" || typeName == "ezTempHashedString" ||
                 typeName == "basic_string" || typeName == "ezStringBuilder" ||
                 typeName == "QString")
        {
          if (prefixAdded)
            *prefixAdded = true;
          return newName.insert(0, "s");
        }
        else if (typeName == "ezAtomicBool")
        {
          if (prefixAdded)
            *prefixAdded = true;
          return newName.insert(0, "b");
        }
        else if (typeName == "ezAtomicInteger32" || typeName == "ezAtomicInteger64")
        {
          if (prefixAdded)
            *prefixAdded = true;
          return newName.insert(0, "i");
        }
        else if (typeName.startswith("ezVec") || typeName.startswith("ezSimdVec"))
        {
          if (prefixAdded)
            *prefixAdded = true;
          return newName.insert(0, "v");
        }
        else if (typeName.startswith("ezMat") || typeName.startswith("ezSimdMat"))
        {
          if (prefixAdded)
            *prefixAdded = true;
          return newName.insert(0, "m");
        }
        else if (typeName.startswith("ezQuat") || typeName == "ezSimdQuat")
        {
          if (prefixAdded)
            *prefixAdded = true;
          return newName.insert(0, "q");
        }
        else if (typeName == "ezSimdFloat")
        {
          if (prefixAdded)
            *prefixAdded = true;
          return newName.insert(0, "f");
        }
        if (prefixAdded)
          *prefixAdded = false;
        return newName;
      }

      std::string AddPrefix(const std::string& baseName, const Type* type, bool* prefixAdded = nullptr)
      {
        std::string newName = baseName;
        auto oldPrefix = StripPrefix(newName);

        if (auto elaboratedType = dyn_cast<ElaboratedType>(type); elaboratedType)
        {
          type = elaboratedType->desugar().getTypePtr();
        }

        auto templateParamType = dyn_cast<TemplateTypeParmType>(type);
        auto templateParamType2 = dyn_cast<SubstTemplateTypeParmType>(type);
        if (templateParamType || templateParamType2)
        {
          // If we hit a template parameter, don't apply any of the type specific
          // rules
          if (prefixAdded)
            *prefixAdded = false;
          return baseName;
        }

        // Leave fixed size arrays as is (e.g. bool m_bSomeBools[3];)
        if (dyn_cast<ConstantArrayType>(type))
        {
          if (prefixAdded)
            *prefixAdded = false;
          return baseName;
        }

        // TODO: Enforce PascalCase
        if (isLowercase(newName[0]))
        {
          const char upperChar[] = {toUppercase(newName[0]), '\0'};
          newName.replace(0, 1, upperChar);
        }

        if (prefixAdded)
          *prefixAdded = true;

        const TypedefType* typedefType = dyn_cast<TypedefType>(type);
        if (typedefType)
        {
          auto typedefName = typedefType->getDecl()->getName();
          if (typedefName.endswith("Handle") || typedefName == "HANDLE" || typedefName == "HWND" || typedefName == "HINSTANCE")
          {
            return newName.insert(0, "h");
          }
        }

        if (type->isPointerType())
        {
          const BuiltinType* pointeeType =
            dyn_cast<BuiltinType>(type->getPointeeType());
          if (pointeeType && pointeeType->isCharType())
          {
            if (oldPrefix == "p")
            {
              return newName.insert(0, oldPrefix);
            }
            else
            {
              return newName.insert(0, "sz");
            }
          }
          else
          {
            if (type->isFunctionPointerType() ||
                type->isMemberFunctionPointerType() ||
                type->isMemberDataPointerType())
            {
              if (prefixAdded)
                *prefixAdded = false;
              return newName;
            }
            else
            {
              return newName.insert(0, "p");
            }
          }
        }
        else
        {
          auto recordDecl = type->getAsCXXRecordDecl();
          if (recordDecl)
          {
            auto recordName = recordDecl->getName();
            bool localPrefixAdded = false;
            newName = AddPrefixForType(recordName, std::move(newName), &localPrefixAdded);
            if (localPrefixAdded)
            {
              return newName;
            }
            else if (auto declTemplate = dyn_cast<ClassTemplateSpecializationDecl>(recordDecl); declTemplate && (recordName == "atomic" || recordName == "ezAtomicInteger"))
            {
              auto& templateArgs = declTemplate->getTemplateArgs();
              if (templateArgs.size() == 1)
              {
                return AddPrefix(baseName, templateArgs.get(0).getAsType().getTypePtr(), prefixAdded);
              }
            }
          }
          else
          {
            const BuiltinType* builtinType = dyn_cast<BuiltinType>(type);
            if (builtinType)
            {
              auto builtinKind = builtinType->getKind();
              if (builtinKind == BuiltinType::Float ||
                  builtinKind == BuiltinType::Double)
              {
                return newName.insert(0, "f");
              }
              else if (builtinKind == BuiltinType::Bool)
              {
                return newName.insert(0, "b");
              }
              else if (builtinType->isSignedInteger())
              {
                return newName.insert(0, "i");
              }
              else if (builtinType->isUnsignedInteger())
              {
                return newName.insert(0, "ui");
              }
            }
            else
            {
              if (typedefType)
              {
                auto typedefName = typedefType->getDecl()->getName();
                if (typedefName == "ezUInt8" || typedefName == "ezUInt16" ||
                    typedefName == "ezUInt32" || typedefName == "ezUInt64" ||
                    typedefName == "size_t" || typedefName == "DWORD")
                {
                  return newName.insert(0, "ui");
                }
                else if (typedefName == "ezInt8" || typedefName == "ezInt16" ||
                         typedefName == "ezInt32" || typedefName == "ezInt64" ||
                         typedefName == "ptrdiff_t" || typedefName == "LONG")
                {
                  return newName.insert(0, "i");
                }
              }
              else
              {
                const TemplateSpecializationType* templateSpecialization =
                  dyn_cast<TemplateSpecializationType>(type);
                if (templateSpecialization)
                {
                  auto templateName = templateSpecialization->getTemplateName()
                                        .getAsTemplateDecl()
                                        ->getName();
                  bool localPrefixAdded = false;
                  newName = AddPrefixForType(templateName, std::move(newName), &localPrefixAdded);
                  if (localPrefixAdded)
                  {
                    return newName;
                  }
                }
              }
            }
          }
        }

        if (prefixAdded)
          *prefixAdded = false;

        // if we have a variable like p0 we strip the 'p' and are only left with "0"
        // this is no longer a valid identifier, so add back the 'p' in case we didn't add any other prefix.
        if (isDigit(newName[0]))
        {
          newName = oldPrefix + newName;
        }


        return newName;
      }

      NameCheck::NameCheck(StringRef Name, ClangTidyContext* Context)
        : RenamerClangTidyCheck(Name, Context)
      {
      }

      std::optional<RenamerClangTidyCheck::FailureInfo>
      NameCheck::getDeclFailureInfo(const NamedDecl* Decl,
        const SourceManager& SM) const
      {
        const FieldDecl* field = dyn_cast<FieldDecl>(Decl);
        const VarDecl* var = dyn_cast<VarDecl>(Decl);
        const ParmVarDecl* param = dyn_cast<ParmVarDecl>(Decl);
        if (field && field->getIdentifier()) // struct / class members
        {
          // No rules on public fields
          if (field->getAccess() == AS_public)
          {
            return std::nullopt;
          }
          // SubstTemplateTypeParmType

          std::string newName = field->getNameAsString();
          StripToBaseName(newName);
          const clang::Type* type = field->getType().getTypePtr();

          // If the field is templated, all types have already been substituted. E.g.
          // "T*" -> "int*". We need the original type "T*" so find it in the lexcial
          // context.
          if (field->isTemplated())
          {
            type = nullptr;
            auto lexicalContext = field->getLexicalDeclContext();
            for (auto& decl : lexicalContext->decls())
            {
              auto lexicalField = dyn_cast<FieldDecl>(decl);
              if (lexicalField && lexicalField->getName() == field->getName())
              {
                type = lexicalField->getType().getTypePtr();
              }
            }
            if (type == nullptr)
            {
              return std::nullopt;
            }
          }
          newName = AddPrefix(newName, type);

          newName.insert(0, "m_");

          if (newName != field->getName())
          {
            return FailureInfo{"field", std::move(newName)};
          }
        }
        else if (param && param->getIdentifier()) // function / method parameters
        {
          const DeclContext* declContext = param->getDeclContext();
          const FunctionDecl* owningFunc = dyn_cast<FunctionDecl>(declContext);

          if (!owningFunc || (owningFunc->getAccess() != AS_public && owningFunc->getAccess() != AS_none))
          {
            return std::nullopt;
          }

          if (auto cxxMethodDecl = dyn_cast<CXXMethodDecl>(owningFunc); cxxMethodDecl)
          {
            if (cxxMethodDecl->getParent()->isLambda())
            {
              // Do not check parameter names for lambdas, as they are implementation detail.
              return std::nullopt;
            }
          }

          std::string newName = param->getNameAsString();

          // Single character names are ok if they are lowercase
          if (newName.length() == 1)
          {
            newName[0] = toLowercase(newName[0]);
            if (newName != param->getName())
            {
              return FailureInfo{"param", std::move(newName)};
            }
            return std::nullopt;
          }

          // Special names
          if (newName == "lhs" || newName == "rhs" || newName == "other" || newName == "value")
          {
            return std::nullopt;
          }

          // If the var is templated, all types have already been substituted. E.g.
          // "T*" -> "int*". We need the original type "T*" so find it in the lexcial
          // context.
          const clang::Type* type = param->getType().getTypePtr();
          std::string typeString = param->getType().getAsString();
          if (!param->isTemplated())
          {
            FunctionDecl* templatedFuncDecl = owningFunc->getInstantiatedFromMemberFunction();
            if (templatedFuncDecl == nullptr)
            {
              templatedFuncDecl = owningFunc->getTemplateInstantiationPattern();
            }
            if (templatedFuncDecl != nullptr)
            {
              unsigned paramIndex = param->getFunctionScopeIndex();
              if (paramIndex >= templatedFuncDecl->getNumParams())
              {
                // This is a expanded vararg template (ARGS...), ignore it.
                return std::nullopt;
              }
              clang::ParmVarDecl* templatedParamDecl = templatedFuncDecl->getParamDecl(paramIndex);
              type = templatedParamDecl->getType().getTypePtr();
              typeString = templatedParamDecl->getType().getAsString();
            }
          }

          const char* refPrefix = "ref_";
          bool refPrefixFound = false;

          {
            llvm::StringRef newNameRef = newName;
            if (newNameRef.startswith("out_"))
            {
              newName.erase(0, 4);
              refPrefix = "out_";
              refPrefixFound = true;
            }
            else if (newNameRef.startswith("inout_"))
            {
              newName.erase(0, 6);
              refPrefix = "inout_";
              refPrefixFound = true;
            }
            else if (newNameRef.startswith("in_"))
            {
              newName.erase(0, 3);
              refPrefix = "in_";
              refPrefixFound = true;
            }
            else if (newNameRef.startswith("ref_"))
            {
              newName.erase(0, 4);
              refPrefixFound = true;
            }
            else if (newNameRef.startswith("out") && newNameRef.size() > 3 && (isUppercase(newNameRef[3]) || isDigit(newNameRef[3])))
            {
              newName.erase(0, 3);
              refPrefix = "out_";
            }
            else if (newNameRef.startswith("inout") && newNameRef.size() > 5 && (isUppercase(newNameRef[5]) || isDigit(newNameRef[5])))
            {
              newName.erase(0, 5);
              refPrefix = "inout_";
            }
            else if (type->isReferenceType() && newNameRef.startswith("in") && newNameRef.size() > 2 && (isUppercase(newNameRef[2]) || isDigit(newNameRef[2])))
            {
              newName.erase(0, 2);
              refPrefix = "in_";
            }
          }

          const clang::Type* prefixType = type;
          if (type->isReferenceType())
          {
            prefixType = type->getPointeeType().getTypePtr();
          }
          bool prefixAdded = false;
          newName = AddPrefix(newName, prefixType, &prefixAdded);
          if (!prefixAdded)
          {
            newName[0] = toLowercase(newName[0]);
          }

          const char* warningHint = "param";

          if (type->isReferenceType() && !type->isRValueReferenceType() && !type->getPointeeType().isConstQualified() && !type->getPointeeType()->isArrayType())
          {
            if (!refPrefixFound)
            {
              warningHint = "paramRef";
            }
            newName = refPrefix + newName;
          }

          if (type->isPointerType() && refPrefixFound && strcmp(refPrefix, "out_") == 0)
          {
            newName = refPrefix + newName;
          }

          if (newName != param->getName())
          {
            return FailureInfo{warningHint, std::move(newName)};
          }
        }
        else if (var && var->getIdentifier()) // static variables
        {
          // s_ rule only applies to static members of structs or classes
          if (!var->isStaticDataMember())
          {
            return std::nullopt;
          }

          if (var->getAccess() == AS_public)
          {
            return std::nullopt;
          }

          // s_ rule does not apply to compile time constants.
          if (var->isConstexpr())
          {
            return std::nullopt;
          }

          // Check for a static member of a enum definition. s_ rule doesn't apply
          // here.
          //
          // struct ezClipSpaceYMode {
          //  enum Enum {
          //    Regular,
          //    Flipped
          //  };
          //  static Enum RenderToTextureDefault;
          // };
          auto enumType = dyn_cast<EnumType>(var->getType().getTypePtr());
          if (enumType && enumType->getDecl()->getName() == "Enum")
          {

            auto varContext = var->getDeclContext();
            auto enumContext = enumType->getDecl()->getDeclContext();
            if (varContext && varContext == enumContext)
            {
              return std::nullopt;
            }
          }

          // If the var is templated, all types have already been substituted. E.g.
          // "T*" -> "int*". We need the original type "T*" so find it in the lexcial
          // context.
          const clang::Type* type = var->getType().getTypePtr();
          if (var->isTemplated())
          {
            type = nullptr;
            auto lexicalContext = var->getLexicalDeclContext();
            for (auto& decl : lexicalContext->decls())
            {
              auto lexicalVar = dyn_cast<VarDecl>(decl);
              if (lexicalVar && lexicalVar->getName() == var->getName())
              {
                type = lexicalVar->getType().getTypePtr();
              }
            }
            if (type == nullptr)
            {
              return std::nullopt;
            }
          }

          // s_ rule applies. Check for s_ prefix and Hungarian notation
          std::string newName = var->getNameAsString();
          StripToBaseName(newName);
          newName = AddPrefix(newName, type);

          newName.insert(0, "s_");

          if (newName != var->getName())
          {
            return FailureInfo{"field", std::move(newName)};
          }
        }
        return std::nullopt;
      }

      std::optional<clang::tidy::RenamerClangTidyCheck::FailureInfo>
      NameCheck::getMacroFailureInfo(const Token& MacroNameTok,
        const SourceManager& SM) const
      {
        return std::nullopt;
      }

      RenamerClangTidyCheck::DiagInfo
      NameCheck::getDiagInfo(const NamingCheckId& ID,
        const NamingCheckFailure& Failure) const
      {
        if (Failure.Info.KindName == "field")
        {
          return DiagInfo{
            "class / struct member '%0' does not follow the naming convention",
            [&](DiagnosticBuilder& Diag)
            { Diag << ID.second; }};
        }
        else if (Failure.Info.KindName == "param")
        {
          return DiagInfo{
            "parameter '%0' does not follow the naming convention (%1)",
            [&](DiagnosticBuilder& Diag)
            { Diag << ID.second << Failure.Info.Fixup; }};
        }
        else if (Failure.Info.KindName == "paramRef")
        {
          return DiagInfo{
            "non const reference parameter '%0' does not follow the naming convention. non-const reference parameters should start with 'in_', 'out_' or 'inout_'.",
            [&](DiagnosticBuilder& Diag)
            { Diag << ID.second; }};
        }
        return {};
      }

    } // namespace ez
  } // namespace tidy
} // namespace clang

// * Do we really want to apply prefixes to arrays? Where to stop? bool
// m_bIsUsed[3]; ezDynmicArray<bool> m_bVar? what about ezUInt8 m_uiData[128];
// -> On fixed size arrays keep the already existing prefix
// * static ezWorldModuleTypeId TYPE_ID. This is not a constant. Why all upper
// case? Why not the s_ prefix? -> s_TypeId
// * Open Questions, What do do about quaternions / vectors / matrices?