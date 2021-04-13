//===--- LibraryUpgradeSuggestionCheck.cpp - clang-tidy -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "LibraryUpgradeSuggestionCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include <map>
#include <fstream>
#include <sstream>
#include <string>

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace misc {

ChangeKind parseChangeKind(std::string kind) {
  if (kind == "Removed_Field" || kind == "Renamed_Field" || kind == "Moved_Field") {
    return FIELD;
  } else if (kind == "Method") {
    return METHOD;
  }
  return UNKNOWN;
}

void populateChanges(std::string ChangeFile, ChangeMap & Changes) {
  std::ifstream InFile(ChangeFile);
  assert(InFile.is_open() && "Changes file could not be opened");

  std::string Line;
  while (std::getline(InFile, Line)) {
    std::vector<std::string> Cols;
    std::istringstream Row(Line);

    std::string Cell;
    while (std::getline(Row, Cell, ',')) {
      Cols.push_back(Cell);
    }
    if (!Row && Cell.empty()) { // empty last column
        Cols.push_back("");
    }

    if (Cols.size() != 3) {
      llvm::errs() << "Row contains " << Cols.size() << " columns instead of 3. Line is \"" << Line << "\"\n";
      exit(1);
    }
    ChangeKind Kind = parseChangeKind(Cols[0]);
    if (Kind == UNKNOWN) {
      llvm::errs() << "Unknown change kind \"" << Cols[0] << "\"\n";
      exit(1);
    }
    Changes.insert(std::make_pair(Cols[1], Change{Kind, Cols[1], Cols[2]}));
  }
}

LibraryUpgradeSuggestionCheck::LibraryUpgradeSuggestionCheck(StringRef Name, ClangTidyContext *Context)
  : ClangTidyCheck(Name, Context) {
    auto ChangeFileOption = Options.get("change_file");
    assert(ChangeFileOption.hasValue() && "change_file option required");
    ChangeFile = ChangeFileOption.getValue();

    populateChanges(ChangeFile, Changes);
}

void LibraryUpgradeSuggestionCheck::storeOptions(ClangTidyOptions::OptionMap &Opts) {
    Options.store(Opts, "change_file", ChangeFile);
  }


void LibraryUpgradeSuggestionCheck::registerMatchers(MatchFinder *Finder) {
  /*
  To find usages of a declaration which changes between versions, we look for a
  MemberExpr which refers to that declaration.
  */

  for (auto & pair: Changes) {
    Finder->addMatcher(memberExpr(hasDeclaration(namedDecl(hasName(pair.second.name)))).bind("member"), this);
  }
}

SourceLocation getNearestLocation(const MemberExpr * expr) {
  SourceLocation result = expr->getExprLoc();
  if (result.isInvalid()) {
    result = expr->getBase()->getExprLoc();
  }
  return result;
}

void LibraryUpgradeSuggestionCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedMember = Result.Nodes.getNodeAs<MemberExpr>("member");

  auto d = diag(getNearestLocation(MatchedMember), "Reference to member will break: " + MatchedMember->getMemberDecl()->getQualifiedNameAsString());
  std::string name = MatchedMember->getMemberDecl()->getQualifiedNameAsString();

  auto it = Changes.find(name);
  if (it == Changes.end()) {
    llvm::dbgs() << "Name \"" << name << "\" doesn't match a registered change.\n";
    return;
  }
  Change change = it->second;
  if (change.fix != "") {
      d << FixItHint::CreateReplacement(MatchedMember->getSourceRange(), change.fix);
  }
}

} // namespace misc
} // namespace tidy
} // namespace clang
