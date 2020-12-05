//===--- IncludeWhatYouUseCheck.h - clang-tidy ------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_READABILITY_INCLUDEWHATYOUUSECHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_READABILITY_INCLUDEWHATYOUUSECHECK_H

#include "../ClangTidyCheck.h"

#include <unordered_set>
#include <string>

namespace clang {
namespace tidy {
namespace readability {

/// FIXME: Write a short description.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/readability-include-what-you-use.html
class IncludeWhatYouUseCheck : public ClangTidyCheck {
public:
  IncludeWhatYouUseCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
  void checkTypeLoc(const ast_matchers::MatchFinder::MatchResult &Result);
  void checkExpr(const ast_matchers::MatchFinder::MatchResult &Result);
  void checkLocation(SourceLocation const &useLoc,
    SourceLocation const &declLoc,
    const ast_matchers::MatchFinder::MatchResult &Result);

private:
    std::unordered_set<std::string> m_includes;
};

} // namespace readability
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_READABILITY_INCLUDEWHATYOUUSECHECK_H
