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
   dart ./tool/bump_version.dart patch --log "Short description of changes"
   # Or interactively:
   make version
   ```
4. Validate version consistency, changelog entry, and package readiness:
   ```bash
   make validate
   # Or with tag and dry-run package checks:
   dart ./tool/verify_release.dart --tag=vX.Y.Z --check-packages --check-artifacts
   ```
5. Commit the version bump:
   ```bash
   git add .
   git commit -m "chore: bump version to X.Y.Z"
   ```

## Publishing a release

Production releases are triggered exclusively by pushing an annotated or signed SemVer tag matching `v*.*.*`:

```bash
git tag -a vX.Y.Z -m "Release vX.Y.Z"
git push origin vX.Y.Z
```

### Canonical automation pipeline (`.github/workflows/release.yml`)

The tag-triggered release workflow strictly enforces the canonical path:
**tag → validate → test/build → publish → GitHub Release**

1. **`tag`**: Pushing `vX.Y.Z` triggers `.github/workflows/release.yml`. Manual production publishing via `workflow_dispatch` is prohibited.
2. **`validate`**:
   - Treats the git tag (`vMAJOR.MINOR.PATCH`) as the authoritative release version.
   - Validates that all distribution manifests match the tag version.
   - Verifies `CHANGELOG.md` contains an entry for `X.Y.Z`.
   - Extracts release notes into an artifact (`release_notes.md`) for the GitHub Release.
3. **`test/build`**:
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
   - **`publish-npm`**: Publishes package to [npm](https://www.npmjs.com/package/@vm75/inditrans) using `npm publish --provenance --access=public` with `NODE_AUTH_TOKEN: ${{ secrets.NPM_TOKEN }}` and Sigstore provenance (`id-token: write`).
5. **`github-release`**:
   - Creates a GitHub Release for tag `vX.Y.Z` using `gh release create`.
   - Attaches the release notes extracted during the `validate` step.

## Safe testing and validation

To verify the release pipeline safely without creating a real release or publishing packages:

- **Local validation**:
  ```bash
  dart tool/verify_release.dart --check-packages --check-artifacts
  ```
- **Tag-specific validation**:
  ```bash
  dart tool/verify_release.dart --tag=v0.13.0 --check-packages --check-artifacts
  ```
- **Continuous Integration**:
  The normal CI workflow (`.github/workflows/ci.yml`) runs on all pushes and PRs to `main` and `dev`, running release validation, native engine tests, Flutter tests/dry-run, and Node.js tests/dry-run.

## Security and registry configuration

- **pub.dev (OIDC Trusted Publishing)**:
  - Enabled via the pub.dev package Admin settings under **Automated publishing** -> **Publishing from GitHub Actions**.
  - Repository is configured with GitHub workflow `.github/workflows/release.yml` and environment `pub.dev`.
  - Pub requires `id-token: write` permission to exchange an OpenID Connect token with pub.dev, eliminating the need for long-lived credentials.
- **npm (Provenance & Token)**:
  - Repository secret `NPM_TOKEN` contains an npm automation or granular access token with publish permissions for scope `@vm75`.
  - Step specifies `npm publish --provenance --access=public` along with `id-token: write` permission to generate cryptographically signed Sigstore attestations linking the package artifact to the exact commit and workflow run.
- **GitHub Environments**:
  - GitHub Environments `pub.dev` and `npm` can be configured with deployment protection rules and required reviewers if manual gates are desired prior to registry publication.
