////////////////////////////////////////////////////////
/// Git commit version file — static SPM build entry ///
////////////////////////////////////////////////////////
// Normally produced by tools/get_git_commit.sh at build time. On the
// spm/package-swift branch we ship a fixed value so `swift build` works
// from a fresh clone without a pre-build phase. CocoaPods builds bypass
// this header entirely via the COCOAPODS preprocessor flag.

#define GIT_COMMIT "-spm"
