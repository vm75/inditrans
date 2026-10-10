# Release Process

This document describes the versioning, validation, and release workflow for `inditrans`.

## Versioning scheme

`inditrans` uses [Semantic Versioning 2.0.0](https://semver.org/):
- **Format**: `MAJOR.MINOR.PATCH` (e.g. `0.13.0`, `1.0.0`)
- **Git tag convention**: `vX.Y.Z` (e.g. `v0.13.0`)
- **Release authority**: The Git tag (`vMAJOR.MINOR.PATCH`) is the **sole release authority**. There is no independent mutable version file.

All distribution manifests are kept strictly synchronized to the target version:
- `flutter/pubspec.yaml`
- `nodejs/package.json`
- `flutter/ios/inditrans.podspec`
- `flutter/macos/inditrans.podspec`
- `flutter/android/build.gradle`
- `flutter/linux/CMakeLists.txt`
- `flutter/windows/CMakeLists.txt`

The single consolidated changelog is maintained in `CHANGELOG.md` at repository root following [Keep a Changelog](https://keepachangelog.com/).

## Pre-release checklist

1. Ensure the working tree is clean on `main`.
2. Run all unit tests across distributions:
   ```bash
   make testall
   ```
3. Bump the version across package manifests and record release notes:
   ```bash
   # Bump patch, minor, or major version:
   python3 ./tool/bump_version.py patch --log "Short description of changes"
   # Or interactively:
   make version
   ```
4. Validate version consistency, changelog entry, and package readiness:
   ```bash
   make validate
   # Or with tag and dry-run package checks:
  python3 tool/verify_release.py --tag=vX.Y.Z --check-packages --check-artifacts
   ```
5. Commit the version bump:
   ```bash
   git add .
   git commit -m "chore: bump version to X.Y.Z"
   ```

## Publishing a release

Production releases are triggered exclusively by pushing an annotated or signed SemVer tag matching `vMAJOR.MINOR.PATCH`:

```bash
git tag -a vX.Y.Z -m "Release vX.Y.Z"
git push origin vX.Y.Z
```

### Canonical automation pipeline (`.github/workflows/release.yml`)

The tag-triggered release workflow strictly enforces the canonical path:
**tag → validate → test/build → publish → GitHub Release**

1. **`tag`**: Pushing `vX.Y.Z` triggers `.github/workflows/release.yml`. There is no manual production publishing workflow.
2. **`validate`**:
   - Treats the git tag (`vMAJOR.MINOR.PATCH`) as the authoritative release version.
   - Validates that all distribution manifests match the tag version.
   - Verifies `CHANGELOG.md` contains an entry for `X.Y.Z`.
   - Extracts release notes into an artifact (`release_notes.md`) for the GitHub Release.
3. **`test/build`**:
   - Builds and smoke tests the Windows DLL and builds the Linux shared library before Emscripten setup, avoiding its `cmake/` directory in native build command lookup.
   - Rebuilds all publishable artifacts from the exact tagged commit:
     - Standalone WASM (`flutter/assets/inditrans.wasm`) and JS WASM (`js/public/inditrans.js`) via Emscripten (`make wasm`).
     - Linux native shared library for Flutter FFI tests.
     - Node.js package build (`cd nodejs && yarn build`).
   - Runs full test suites across all distributions:
     - Native C++ engine tests (`make test`).
     - Flutter tests and static analysis (`dart analyze`, `flutter test`).
     - Node.js Jest test suite (`yarn test`).
   - Deterministically verifies all binary artifacts exist and are non-empty.
   - Performs fail-closed publish dry-runs (`flutter pub publish --dry-run` and `npm pack --dry-run`).
4. **`publish`**:
   - **`publish-pub`**: Publishes package to [pub.dev](https://pub.dev/packages/inditrans) using pub.dev Automated Publishing via GitHub Actions OIDC (`id-token: write`, `flutter pub publish --force`).
   - **`publish-npm`**: Publishes package to [npm](https://www.npmjs.com/package/@vm75/inditrans) using npm Trusted Publishing via GitHub Actions OIDC (`id-token: write`, `npm publish --access=public`).
5. **`github-release`**:
   - Creates a GitHub Release for tag `vX.Y.Z` using `gh release create`.
   - Attaches the release notes extracted during the `validate` step.

## Safe testing and validation

To verify the release pipeline safely without creating a real release or publishing packages:

- **Local validation**:
  ```bash
  python3 tool/verify_release.py --check-packages --check-artifacts
  ```
- **Tag-specific validation**:
  ```bash
  python3 tool/verify_release.py --tag=v0.13.0 --check-packages --check-artifacts
  ```
- **Continuous Integration**:
  The normal CI workflow (`.github/workflows/ci.yml`) runs on pushes to `main` and PRs targeting `main`, running release validation, native engine tests, Flutter tests/dry-run, and Node.js tests/dry-run.
  Its PR performance gate compares timing, output hashes, and allocation metrics
  against `perf-00-baseline`, using the current harness with unchanged tagged engine sources.

## Security and registry configuration

- **pub.dev (OIDC Trusted Publishing)**:
  - Enabled via the pub.dev package Admin settings under **Automated publishing** -> **Publishing from GitHub Actions**.
  - Repository is configured with GitHub workflow `.github/workflows/release.yml` and environment `pub.dev`.
  - Pub requires `id-token: write` permission to exchange an OpenID Connect token with pub.dev, eliminating the need for long-lived credentials.
- **npm (OIDC Trusted Publishing)**:
  - Configured on npmjs.com under package settings for `@vm75/inditrans` under **Publishing Access** -> **Trusted Publishing**.
  - Configured with provider GitHub Actions, owner/repository `vm75/inditrans`, workflow `.github/workflows/release.yml`, and environment `npm`.
  - The release job uses GitHub Actions OIDC (`id-token: write`) and `npm publish --access=public`; no long-lived `NPM_TOKEN` or `NODE_AUTH_TOKEN` is required.
- **GitHub Environments**:
  - GitHub Environments `pub.dev` and `npm` can be configured with deployment protection rules and required reviewers if manual gates are desired prior to registry publication.

## Partial releases and failure recovery

Release integrity requires that published package versions are immutable and release tags are never moved or deleted to mask failures.

### Invariants
1. **Tags are immutable**: Never move or delete an existing `vX.Y.Z` tag that has triggered build or publish steps.
2. **Registry versions are immutable**: Neither pub.dev nor npm allows re-publishing or overwriting an already published version number.
3. **Fail-safe isolation**: Jobs are isolated and dependent; publication only occurs after all validation, compilation, and testing steps pass.

### Recovery scenarios

- **Validation or build fails (before publication)**:
  - No packages are published to either registry.
  - Fix the underlying issue on `main`.
  - Bump to a new patch/minor version using `python3 tool/bump_version.py`.
  - Create and push a new tag for the incremented version (e.g. `v0.13.1`).
  - Do not move or reuse the failed tag.

- **pub.dev succeeds, but npm fails (or vice versa)**:
  - One registry now contains version `X.Y.Z`, while the other does not.
  - Inspect the failed job in the GitHub Actions run logs (e.g., registry outage or OIDC misconfiguration).
  - If the failure was transient (network timeout or temporary registry error): rerun only the failed job (`publish-npm` or `publish-pub`) from the GitHub Actions UI.
  - If the issue cannot be resolved by rerunning the job: **do not attempt to unpublish or overwrite**. Advance to the next version on `main` (e.g. `v0.13.1`), commit, tag, and publish. Both registries will synchronize on the newer release.

- **Both packages publish, but GitHub Release creation fails**:
  - Both packages are safely live on their respective registries.
  - Inspect why `github-release` failed (e.g., transient GitHub API issue).
  - Rerun the `github-release` job in GitHub Actions.
  - Alternatively, create the GitHub Release manually using the tagged commit notes:
    ```bash
    python3 tool/verify_release.py --tag=vX.Y.Z --extract-changelog=release_notes.md
    gh release create "vX.Y.Z" --title "Release vX.Y.Z" --notes-file release_notes.md --verify-tag
    ```

- **Workflow rerun after successful publication**:
  - If a workflow run is retried after a package has already been published to pub.dev or npm, the registry will reject the duplicate version with a fail-closed error.
  - Do not attempt to force republishing over an existing version. Instead, bump the version and push a fresh tag.
