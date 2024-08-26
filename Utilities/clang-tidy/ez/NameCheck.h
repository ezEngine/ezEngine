//===--- MemberVarCheckCheck.h - clang-tidy ---------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_MEMBERVARCHECKCHECK_H
#  define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_EZ_MEMBERVARCHECKCHECK_H

#  include "../utils/RenamerClangTidyCheck.h"

namespace clang
{
  namespace tidy
  {
    namespace ez
    {

      class NameCheck : public RenamerClangTidyCheck
      {
      public:
        NameCheck(StringRef Name, ClangTidyContext* Context);

      protected:
        std::optional<FailureInfo>
        getDeclFailureInfo(const NamedDecl* Decl,
          const SourceManager& SM) const override;

        std::optional<FailureInfo>
        getMacroFailureInfo(const Token& MacroNameTok,
          const SourceManager& SM) const override;

        DiagInfo getDiagInfo(const NamingCheckId& ID,
          const NamingCheckFailure& Failure) const override;

      private:
        mutable const SourceManager* m_pSourceManager = nullptr;
      };

    } // namespace ez
  } // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_EZ_MEMBERVARCHECKCHECK_H
