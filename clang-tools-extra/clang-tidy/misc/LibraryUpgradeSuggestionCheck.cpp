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

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace misc {

LibraryUpgradeSuggestionCheck::LibraryUpgradeSuggestionCheck(StringRef Name, ClangTidyContext *Context)
  : ClangTidyCheck(Name, Context) {

    auto DBOption = Options.get("db");
    auto UserOption = Options.get("user");
    auto OldVersionOption = Options.get("old_version");
    auto NewVersionOption = Options.get("new_version");
    assert(DBOption.hasValue() && "db option required");
    assert(UserOption.hasValue() && "user option required");
    assert(OldVersionOption.hasValue() && "old_version option required");
    assert(NewVersionOption.hasValue() && "new_version option required");
    DB = DBOption.getValue();
    User = UserOption.getValue();
    OldVersion = OldVersionOption.getValue();
    NewVersion = NewVersionOption.getValue();

    // TODO: Contact DB and obtain changes
    Changes.insert(std::make_pair("clang::CFGBlock::getTerminator", Change{METHOD, "clang::CFGBlock::getTerminator"}));
    Changes.insert(std::make_pair("clang::immutability::Values::maybeFields", Change{FIELD, "clang::immutability::Values::maybeFields"}));
}

void LibraryUpgradeSuggestionCheck::storeOptions(ClangTidyOptions::OptionMap &Opts) {
    Options.store(Opts, "db", DB);
    Options.store(Opts, "user", User);
    Options.store(Opts, "old_version", OldVersion);
    Options.store(Opts, "new_version", NewVersion);
  }


void LibraryUpgradeSuggestionCheck::registerMatchers(MatchFinder *Finder) {
  /*
  To find usages of a declaration which changes between versions, we look for a
  MemberExpr which refers to that declaration.

  To match a namespaced decl like clang::immutability::Values::maybeFields, we
  match on a decl called maybeFields which has a declaration context of
  clang::immutability::Values. This, recursively, can be matched by finding a
  decl called Values with a declaration context clang::immutability (and so on).
  For example, the final matcher might look like:

  Finder->addMatcher(
    memberExpr(hasDeclaration(
      namedDecl(hasName("maybeFields"), hasDeclContext(
        namedDecl(hasName("Values"), hasDeclContext(
          namedDecl(hasName("immutability"), hasDeclContext(
            namedDecl(hasName("clang")))))))))).bind("member"),
    this
  );
  */

  for (auto & pair: Changes) {
    std::size_t start = 0, end;
    clang::ast_matchers::internal::Matcher<Decl> matcher = anything();
    std::string part;
    // Split name "a::b::c::d" by "::" and construct the matcher from the inside out
    // (as shown above).
    while ((end = pair.first.find("::", start)) != std::string::npos) {
      part = pair.first.substr(start, end - start);
      matcher = namedDecl(hasName(part), hasDeclContext(std::move(matcher)));
      start = end + 2;
    }
    part = pair.first.substr(start);
    matcher = namedDecl(hasName(part), hasDeclContext(std::move(matcher)));

    Finder->addMatcher(memberExpr(hasDeclaration(std::move(matcher))).bind("member"), this);
  }
}

void LibraryUpgradeSuggestionCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedMember = Result.Nodes.getNodeAs<MemberExpr>("member");

  diag(MatchedMember->getMemberLoc(), "found a problem: " + MatchedMember->getMemberDecl()->getName().str());
}

} // namespace misc
} // namespace tidy
} // namespace clang
