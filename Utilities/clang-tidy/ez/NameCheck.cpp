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
        if (name.size() == 0 || isUppercase(name[0]))
        {
          return {};
        }
        if (name.size() > 1 && isUppercase(name[1]))
        {
          result = std::string(name.begin(), name.begin() + 1);
          name.erase(0, 1);
        }
        else if (name.size() > 2 && isUppercase(name[2]))
        {
          result = std::string(name.begin(), name.begin() + 2);
          if (result == "is" || result == "on" || result == "id")
          {
            return {};
          }
          name.erase(0, 2);
        }
        return result;
      }

      std::string AddPrefixForType(clang::StringRef typeName, std::string newName)
      {
        if (typeName.endswith("Handle"))
        {
          return newName.insert(0, "h");
        }
        else if (typeName == "ezSharedPtr" || typeName == "ezUniquePtr" ||
                 typeName == "ezPointerWithFlags" || typeName == "QPointer" ||
                 typeName == "shared_ptr" || typeName == "unique_ptr")
        {
          return newName.insert(0, "p");
        }
        else if (typeName == "ezStringView" || typeName == "ezHybridString" ||
                 typeName == "ezHashedString" || typeName == "ezTempHashedString" ||
                 typeName == "basic_string" || typeName == "ezStringBuilder" ||
                 typeName == "QString")
        {
          return newName.insert(0, "s");
        }
        else if (typeName == "ezAtomicBool")
        {
          return newName.insert(0, "b");
        }
        else if (typeName == "ezAtomicInteger")
        {
          return newName.insert(0, "i");
        }
        else if (typeName.startswith("ezVec") || typeName.startswith("ezSimdVec"))
        {
          return newName.insert(0, "v");
        }
        else if (typeName.startswith("ezMat") || typeName.startswith("ezSimdMat"))
        {
          return newName.insert(0, "m");
        }
        else if (typeName.startswith("ezQuat") || typeName == "ezSimdQuat")
        {
          return newName.insert(0, "q");
        }
        else if (typeName == "ezSimdFloat")
        {
          return newName.insert(0, "f");
        }
        return newName;
      }

      std::string AddPrefix(const std::string& baseName, const Type* type)
      {
        std::string newName = baseName;
        auto oldPrefix = StripPrefix(newName);

        auto templateParamType = dyn_cast<TemplateTypeParmType>(type);
        auto templateParamType2 = dyn_cast<SubstTemplateTypeParmType>(type);
        if (templateParamType || templateParamType2)
        {
          // If we hit a template parameter, don't apply any of the type specific
          // rules
          return baseName;
        }

        // Leave fixed size arrays as is (e.g. bool m_bSomeBools[3];)
        if (dyn_cast<ConstantArrayType>(type))
        {
          return baseName;
        }

        // TODO: Enforce PascalCase

        if (isLowercase(newName[0]))
        {
          const char upperChar[] = {toUppercase(newName[0]), '\0'};
          newName.replace(0, 1, upperChar);
        }

        const TypedefType* typedefType = dyn_cast<TypedefType>(type);
        if (typedefType)
        {
          auto typedefName = typedefType->getDecl()->getName();
          if (typedefName.endswith("Handle") || typedefName == "HANDLE")
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
            return AddPrefixForType(recordName, std::move(newName));
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
                  return AddPrefixForType(templateName, std::move(newName));
                }
              }
            }
          }
        }

        return newName;
      }

      NameCheck::NameCheck(StringRef Name, ClangTidyContext* Context)
        : RenamerClangTidyCheck(Name, Context)
      {
      }

      llvm::Optional<RenamerClangTidyCheck::FailureInfo>
      NameCheck::getDeclFailureInfo(const NamedDecl* Decl,
        const SourceManager& SM) const
      {
        const FieldDecl* field = dyn_cast<FieldDecl>(Decl);
        const VarDecl* var = dyn_cast<VarDecl>(Decl);
        if (field && field->getIdentifier())
        {
          // No rules on public fields
          if (field->getAccess() == AS_public)
          {
            return llvm::None;
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
              return llvm::None;
            }
          }
          newName = AddPrefix(newName, type);

          newName.insert(0, "m_");

          if (newName != field->getName())
          {
            return FailureInfo{"field", std::move(newName)};
          }
        }
        else if (var && var->getIdentifier())
        {
          // s_ rule only applies to static members of structs or classes
          if (!var->isStaticDataMember())
          {
            return llvm::None;
          }

          if (var->getAccess() == AS_public)
          {
            return llvm::None;
          }

          // s_ rule does not apply to compile time constants.
          if (var->isConstexpr())
          {
            return llvm::None;
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
              return llvm::None;
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
              return llvm::None;
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
        return llvm::None;
      }

      llvm::Optional<clang::tidy::RenamerClangTidyCheck::FailureInfo>
      NameCheck::getMacroFailureInfo(const Token& MacroNameTok,
        const SourceManager& SM) const
      {
        return llvm::None;
      }

      RenamerClangTidyCheck::DiagInfo
      NameCheck::getDiagInfo(const NamingCheckId& ID,
        const NamingCheckFailure& Failure) const
      {
        return DiagInfo{
          "class / struct member '%0' does not follow naming convention",
          [&](DiagnosticBuilder& Diag) { Diag << ID.second; }};
      }

    } // namespace ez
  }   // namespace tidy
} // namespace clang

// * Do we really want to apply prefixes to arrays? Where to stop? bool
// m_bIsUsed[3]; ezDynmicArray<bool> m_bVar? what about ezUInt8 m_uiData[128];
// -> On fixed size arrays keep the already existing prefix
// * static ezWorldModuleTypeId TYPE_ID. This is not a constant. Why all upper
// case? Why not the s_ prefix? -> s_TypeId
// * Open Questions, What do do about quaternions / vectors / matrices?