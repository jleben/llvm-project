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
#include "clang/Basic/Module.h"

#include <cstdio>
#include <iostream>
#include <sstream>
#include <algorithm>

using namespace std;
using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace readability {

IncludeWhatYouUseCheck::~IncludeWhatYouUseCheck()
{
    if (!m_includes.empty())
    {
        vector<string> paths(m_includes.begin(), m_includes.end());
        sort(paths.begin(), paths.end());

        ostringstream msg;
        msg << "Required includes:" << endl;
        for (auto &path : paths) {
            msg << path << endl;
        }
        msg << "... Examples follow ...";

        diag(SourceLocation(), msg.str());
    }
}

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

    if (decl->hasOwningModule()) {
        cerr << "Owning module: " << decl->getOwningModule()->getFullModuleName() << endl;
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

std::string fileName(FileID fileId, SourceManager *sourceManager)
{
    auto fileEntry = sourceManager->getFileEntryForID(fileId);
    if (fileEntry) {
        return fileEntry->getName().str();
    }
    else {
        return "";
    }
}

SourceLocation findIncludedSystemHeaderForLoc(SourceLocation location, SourceManager *sourceManager)
{
    auto fileId = sourceManager->getFileID(location);

    auto includeLocation = sourceManager->getIncludeLoc(fileId);
    if (!includeLocation.isValid())
        return location;

    auto includerKind = sourceManager->getFileCharacteristic(includeLocation);
    if (includerKind != SrcMgr::CharacteristicKind::C_System)
        return location;

    return findIncludedSystemHeaderForLoc(includeLocation, sourceManager);
}

void IncludeWhatYouUseCheck::checkLocation
(SourceLocation const &useLoc, SourceLocation const &declLoc,
    const ast_matchers::MatchFinder::MatchResult &Result)
{
    auto *sourceManager = Result.SourceManager;

    auto fileId = Result.SourceManager->getFileID(declLoc);
    if (fileId == Result.SourceManager->getMainFileID()) {
        return;
    }

    auto systemIncludeLoc = findIncludedSystemHeaderForLoc(declLoc, Result.SourceManager);

    if (systemIncludeLoc.isValid() && systemIncludeLoc != declLoc) {
        auto systemFileId = Result.SourceManager->getFileID(systemIncludeLoc);
#if 0
        cerr << "Substituting " << fileName(fileId, Result.SourceManager)
             << " with " << fileName(systemFileId, Result.SourceManager) << endl;
#endif
        fileId = systemFileId;
    }

    if (fileId == sourceManager->getMainFileID()) {
        return;
    }

    auto fileEntry = sourceManager->getFileEntryForID(fileId);
    if (!fileEntry) {
        cerr << "No file for ID." << endl;
        return;
    }

    auto fileName = sourceManager->getFileManager().getCanonicalName(fileEntry).str();
    if (!m_includes.count(fileName)) {
        m_includes.insert(fileName);
        diag(useLoc, "Requires: %0") << fileName;
    }
}

} // namespace readability
} // namespace tidy
} // namespace clang
