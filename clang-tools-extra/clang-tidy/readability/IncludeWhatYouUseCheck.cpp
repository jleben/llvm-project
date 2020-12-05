//===--- IncludeWhatYouUseCheck.cpp - clang-tidy --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "IncludeWhatYouUseCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

#include <cstdio>
#include <iostream>

using namespace std;
using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace readability {

void IncludeWhatYouUseCheck::registerMatchers(MatchFinder *Finder) {
    auto exprMatcher = [](auto matcherFunc)
    {
        return matcherFunc(hasDeclaration(decl().bind("decl")), isExpansionInMainFile())
            .bind("expr");
    };

    Finder->addMatcher(exprMatcher(memberExpr), this);
    Finder->addMatcher(exprMatcher(cxxConstructExpr), this);
    Finder->addMatcher(exprMatcher(declRefExpr), this);

    Finder->addMatcher(typeLoc(
        loc(qualType(hasDeclaration(decl().bind("decl")))),
        isExpansionInMainFile())
            .bind("typeLoc"), this);
}

void IncludeWhatYouUseCheck::check(const MatchFinder::MatchResult &Result) {
    checkTypeLoc(Result);
    checkExpr(Result);

#if 0
  if (MatchedDecl->getName().startswith("awesome_"))
    return;
  diag(MatchedDecl->getLocation(), "function %0 is insufficiently awesome")
      << MatchedDecl;
  diag(MatchedDecl->getLocation(), "insert 'awesome'", DiagnosticIDs::Note)
      << FixItHint::CreateInsertion(MatchedDecl->getLocation(), "awesome_");
#endif
}

void IncludeWhatYouUseCheck::checkTypeLoc(const ast_matchers::MatchFinder::MatchResult &Result)
{
    const auto *typeLoc = Result.Nodes.getNodeAs<TypeLoc>("typeLoc");
    if (!typeLoc) {
        return;
    }

    const auto *decl = Result.Nodes.getNodeAs<Decl>("decl");
    if (!decl) {
        cerr << "No decl." << endl;
        return;
    }

    checkLocation(typeLoc->getBeginLoc(), decl->getLocation(), Result);
}

void IncludeWhatYouUseCheck::checkExpr(const ast_matchers::MatchFinder::MatchResult &Result)
{
    const auto *expr = Result.Nodes.getNodeAs<Expr>("expr");
    if (!expr) {
        return;
    }

    const auto *decl = Result.Nodes.getNodeAs<Decl>("decl");
    if (!decl) {
        cerr << "No decl." << endl;
        return;
    }

    checkLocation(expr->getExprLoc(), decl->getLocation(), Result);
}

void IncludeWhatYouUseCheck::checkLocation
(SourceLocation const &useLoc, SourceLocation const &declLoc,
    const ast_matchers::MatchFinder::MatchResult &Result)
{

    auto fileId = Result.SourceManager->getFileID(declLoc);

    if (fileId == Result.SourceManager->getMainFileID()) {
        return;
    }

    auto fileEntry = Result.SourceManager->getFileEntryForID(fileId);
    if (!fileEntry) {
        cerr << "No declaration file." << endl;
        return;
    }

    auto fileName = fileEntry->getName().str();
    if (!m_includes.count(fileName)) {
        m_includes.insert(fileName);
        diag(SourceLocation(), "Include: %0") << fileName;
    }
}

} // namespace readability
} // namespace tidy
} // namespace clang
