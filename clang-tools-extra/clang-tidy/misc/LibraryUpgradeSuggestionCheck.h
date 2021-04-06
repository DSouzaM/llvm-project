//===--- LibraryUpgradeSuggestionCheck.h - clang-tidy -----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_LIBRARYUPGRADESUGGESTIONCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_LIBRARYUPGRADESUGGESTIONCHECK_H

#include "../ClangTidyCheck.h"

namespace clang {
namespace tidy {
namespace misc {

/// Suggests code changes to automate upgrading between versions of a library.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/misc-library-upgrade-suggestion.html

enum ChangeKind {
  FIELD,
  METHOD
};

struct Change {
  ChangeKind kind;
  std::string name;
  // For now, we'll just report breaking changes (and worry about suggesting upgrades after)
};

class LibraryUpgradeSuggestionCheck : public ClangTidyCheck {
  std::string DB;
  std::string User;
  std::string OldVersion;
  std::string NewVersion;
  std::map<std::string, Change> Changes;
public:
  LibraryUpgradeSuggestionCheck(StringRef Name, ClangTidyContext *Context);
  void storeOptions(ClangTidyOptions::OptionMap &Opts) override;
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace misc
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_LIBRARYUPGRADESUGGESTIONCHECK_H
