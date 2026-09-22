// ignore_for_file: avoid_print

import 'dart:convert';
import 'dart:io';

const semverRegex = r'^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)$';

void main(List<String> args) async {
  String? tagArg;
  String? extractChangelogPath;
  var checkPackages = false;
  var checkArtifacts = true;

  for (final arg in args) {
    if (arg.startsWith('--tag=')) {
      tagArg = arg.substring('--tag='.length);
    } else if (arg.startsWith('--extract-changelog=')) {
      extractChangelogPath = arg.substring('--extract-changelog='.length);
    } else if (arg == '--check-packages') {
      checkPackages = true;
    } else if (arg == '--skip-artifacts') {
      checkArtifacts = false;
    } else if (!arg.startsWith('--') && tagArg == null) {
      tagArg = arg;
    }
  }

  // Fallback to GitHub Actions environment variable if available
  if (tagArg == null && Platform.environment['GITHUB_REF_TYPE'] == 'tag') {
    tagArg = Platform.environment['GITHUB_REF_NAME'];
  }

  final errors = <String>[];
  final semverPattern = RegExp(semverRegex);
  late final String expectedVersion;

  // 1. Determine expected release version from the authoritative source
  if (tagArg != null && tagArg.isNotEmpty) {
    // Git tag (vMAJOR.MINOR.PATCH) is the sole release authority
    if (!tagArg.startsWith('v')) {
      errors.add("Release tag must start with 'v', got: '$tagArg'");
      print('❌ Release tag format error: Tag must start with "v"');
      exit(1);
    }
    final tagVersion = tagArg.substring(1);
    if (!semverPattern.hasMatch(tagVersion)) {
      errors.add(
        "Release tag '$tagArg' does not contain valid SemVer: '$tagVersion'",
      );
      print("❌ Release tag '$tagArg' is not a valid SemVer string.");
      exit(1);
    }
    expectedVersion = tagVersion;
    print("✓ Git tag '$tagArg' is sole release authority (expected version: '$expectedVersion')");
  } else {
    // Non-tag validation (CI on main / dev, local check): derive version from package manifest
    final pubspecFile = File('flutter/pubspec.yaml');
    if (!pubspecFile.existsSync()) {
      print('❌ Error: flutter/pubspec.yaml not found.');
      exit(1);
    }
    final match = RegExp(r'^version:\s*([^\s#]+)', multiLine: true)
        .firstMatch(pubspecFile.readAsStringSync());
    if (match == null || !semverPattern.hasMatch(match.group(1)!)) {
      print('❌ Error: Valid SemVer version not found in flutter/pubspec.yaml');
      exit(1);
    }
    expectedVersion = match.group(1)!;
    print("✓ Derived expected version from flutter/pubspec.yaml: '$expectedVersion'");
  }

  // 2. Validate flutter/pubspec.yaml
  final pubspecFile = File('flutter/pubspec.yaml');
  if (!pubspecFile.existsSync()) {
    errors.add('flutter/pubspec.yaml not found.');
  } else {
    final match = RegExp(r'^version:\s*([^\s#]+)', multiLine: true)
        .firstMatch(pubspecFile.readAsStringSync());
    if (match == null) {
      errors.add('version field not found in flutter/pubspec.yaml');
    } else if (match.group(1) != expectedVersion) {
      errors.add(
        "flutter/pubspec.yaml version '${match.group(1)}' != '$expectedVersion'",
      );
    } else {
      print("✓ flutter/pubspec.yaml version matches '$expectedVersion'");
    }
  }

  // 3. Validate nodejs/package.json
  final packageJsonFile = File('nodejs/package.json');
  if (!packageJsonFile.existsSync()) {
    errors.add('nodejs/package.json not found.');
  } else {
    try {
      final json = jsonDecode(packageJsonFile.readAsStringSync()) as Map<String, dynamic>;
      final pkgVersion = json['version'] as String?;
      if (pkgVersion != expectedVersion) {
        errors.add(
          "nodejs/package.json version '$pkgVersion' != '$expectedVersion'",
        );
      } else {
        print("✓ nodejs/package.json version matches '$expectedVersion'");
      }
    } catch (e) {
      errors.add('Failed to parse nodejs/package.json: $e');
    }
  }

  // 4. Validate iOS podspec
  final iosPodspec = File('flutter/ios/inditrans.podspec');
  if (!iosPodspec.existsSync()) {
    errors.add('flutter/ios/inditrans.podspec not found.');
  } else {
    final match = RegExp(r"s\.version\s*=\s*'([^']+)'")
        .firstMatch(iosPodspec.readAsStringSync());
    if (match == null) {
      errors.add('s.version not found in flutter/ios/inditrans.podspec');
    } else if (match.group(1) != expectedVersion) {
      errors.add(
        "flutter/ios/inditrans.podspec version '${match.group(1)}' != '$expectedVersion'",
      );
    } else {
      print("✓ flutter/ios/inditrans.podspec version matches '$expectedVersion'");
    }
  }

  // 5. Validate macOS podspec
  final macosPodspec = File('flutter/macos/inditrans.podspec');
  if (!macosPodspec.existsSync()) {
    errors.add('flutter/macos/inditrans.podspec not found.');
  } else {
    final match = RegExp(r"s\.version\s*=\s*'([^']+)'")
        .firstMatch(macosPodspec.readAsStringSync());
    if (match == null) {
      errors.add('s.version not found in flutter/macos/inditrans.podspec');
    } else if (match.group(1) != expectedVersion) {
      errors.add(
        "flutter/macos/inditrans.podspec version '${match.group(1)}' != '$expectedVersion'",
      );
    } else {
      print("✓ flutter/macos/inditrans.podspec version matches '$expectedVersion'");
    }
  }

  // 6. Validate Android build.gradle
  final androidGradle = File('flutter/android/build.gradle');
  if (!androidGradle.existsSync()) {
    errors.add('flutter/android/build.gradle not found.');
  } else {
    final match = RegExp(r"""version\s*=?\s*["']([^"']+)["']""")
        .firstMatch(androidGradle.readAsStringSync());
    if (match == null) {
      errors.add('version not found in flutter/android/build.gradle');
    } else if (match.group(1) != expectedVersion) {
      errors.add(
        "flutter/android/build.gradle version '${match.group(1)}' != '$expectedVersion'",
      );
    } else {
      print("✓ flutter/android/build.gradle version matches '$expectedVersion'");
    }
  }

  // 7. Validate Linux CMakeLists.txt
  final linuxCMake = File('flutter/linux/CMakeLists.txt');
  if (!linuxCMake.existsSync()) {
    errors.add('flutter/linux/CMakeLists.txt not found.');
  } else {
    final match = RegExp(r'project\(\$\{PROJECT_NAME\}\s+VERSION\s+([^\s\)]+)')
        .firstMatch(linuxCMake.readAsStringSync());
    if (match == null) {
      errors.add('VERSION not found in flutter/linux/CMakeLists.txt');
    } else if (match.group(1) != expectedVersion) {
      errors.add(
        "flutter/linux/CMakeLists.txt version '${match.group(1)}' != '$expectedVersion'",
      );
    } else {
      print("✓ flutter/linux/CMakeLists.txt version matches '$expectedVersion'");
    }
  }

  // 8. Validate Windows CMakeLists.txt
  final windowsCMake = File('flutter/windows/CMakeLists.txt');
  if (!windowsCMake.existsSync()) {
    errors.add('flutter/windows/CMakeLists.txt not found.');
  } else {
    final match = RegExp(r'project\(\$\{PROJECT_NAME\}\s+VERSION\s+([^\s\)]+)')
        .firstMatch(windowsCMake.readAsStringSync());
    if (match == null) {
      errors.add('VERSION not found in flutter/windows/CMakeLists.txt');
    } else if (match.group(1) != expectedVersion) {
      errors.add(
        "flutter/windows/CMakeLists.txt version '${match.group(1)}' != '$expectedVersion'",
      );
    } else {
      print("✓ flutter/windows/CMakeLists.txt version matches '$expectedVersion'");
    }
  }

  // 9. Validate root CHANGELOG.md
  final changelogFile = File('CHANGELOG.md');
  if (!changelogFile.existsSync()) {
    errors.add('Authoritative CHANGELOG.md not found at repository root.');
  } else {
    final changelogContent = changelogFile.readAsStringSync();
    final escapedVersion = RegExp.escape(expectedVersion);
    final versionHeaderPattern = RegExp('^##\\s*\\[?v?$escapedVersion\\]?.*\$', multiLine: true);
    final match = versionHeaderPattern.firstMatch(changelogContent);
    if (match == null) {
      errors.add(
        "CHANGELOG.md missing release section for version '$expectedVersion' (expected '## [$expectedVersion]' or '## $expectedVersion')",
      );
    } else {
      print("✓ CHANGELOG.md contains release notes for version '$expectedVersion'");
      if (extractChangelogPath != null) {
        final startIdx = match.end;
        final nextHeaderMatch = RegExp(r'^##\s+', multiLine: true).firstMatch(changelogContent.substring(startIdx));
        final sectionContent = nextHeaderMatch != null
            ? changelogContent.substring(startIdx, startIdx + nextHeaderMatch.start).trim()
            : changelogContent.substring(startIdx).trim();
        File(extractChangelogPath).writeAsStringSync('$sectionContent\n');
        print("✓ Extracted release notes to '$extractChangelogPath'");
      }
    }
  }

  // 10. Validate required compiled artifacts
  if (checkArtifacts) {
    final wasmFile = File('flutter/assets/inditrans.wasm');
    if (!wasmFile.existsSync() || wasmFile.lengthSync() == 0) {
      errors.add(
        'Required binary artifact flutter/assets/inditrans.wasm missing or empty.',
      );
    } else {
      print('✓ flutter/assets/inditrans.wasm present (${wasmFile.lengthSync()} bytes)');
    }

    final jsFile = File('js/public/inditrans.js');
    if (!jsFile.existsSync() || jsFile.lengthSync() == 0) {
      errors.add(
        'Required JS wrapper artifact js/public/inditrans.js missing or empty.',
      );
    } else {
      print('✓ js/public/inditrans.js present (${jsFile.lengthSync()} bytes)');
    }
  }

  // 11. Fail-closed, deterministic package publish dry-run checks
  if (checkPackages) {
    print('Running package publish dry-runs...');
    // Prepare temporary package changelog for pub.dev dry-run if not present
    final tempFlutterChangelog = File('flutter/CHANGELOG.md');
    final changelogCreated = !tempFlutterChangelog.existsSync();
    if (changelogCreated && changelogFile.existsSync()) {
      changelogFile.copySync('flutter/CHANGELOG.md');
    }
    try {
      final flutterResult = await Process.run(
        'flutter',
        ['pub', 'publish', '--dry-run'],
        workingDirectory: 'flutter',
      );
      if (flutterResult.exitCode != 0) {
        errors.add(
          'flutter pub publish --dry-run failed with exit code ${flutterResult.exitCode}:\n${flutterResult.stderr}\n${flutterResult.stdout}',
        );
      } else {
        print('✓ flutter pub publish --dry-run passed cleanly');
      }
    } finally {
      if (changelogCreated && tempFlutterChangelog.existsSync()) {
        tempFlutterChangelog.deleteSync();
      }
    }

    // Node.js pack dry-run
    final npmResult = await Process.run(
      'npm',
      ['pack', '--dry-run'],
      workingDirectory: 'nodejs',
    );
    if (npmResult.exitCode != 0) {
      errors.add(
        'npm pack --dry-run failed with exit code ${npmResult.exitCode}:\n${npmResult.stderr}\n${npmResult.stdout}',
      );
    } else {
      print('✓ npm pack --dry-run passed cleanly');
    }
  }

  if (errors.isNotEmpty) {
    print('\n❌ Release validation failed with ${errors.length} error(s):');
    for (final err in errors) {
      print('  - $err');
    }
    exit(1);
  }

  print('\n✨ All release and version validation checks PASSED for $expectedVersion!');
}
;

void main(List<String> args) async {
  String? tagArg;
  String? extractChangelogPath;
  var checkPackages = false;
  var checkArtifacts = true;

  for (final arg in args) {
    if (arg.startsWith('--tag=')) {
      tagArg = arg.substring('--tag='.length);
    } else if (arg.startsWith('--extract-changelog=')) {
      extractChangelogPath = arg.substring('--extract-changelog='.length);
    } else if (arg == '--check-packages') {
      checkPackages = true;
    } else if (arg == '--skip-artifacts') {
      checkArtifacts = false;
    } else if (!arg.startsWith('--') && tagArg == null) {
      tagArg = arg;
    }
  }

  // Fallback to GitHub Actions environment variable if available
  if (tagArg == null && Platform.environment['GITHUB_REF_TYPE'] == 'tag') {
    tagArg = Platform.environment['GITHUB_REF_NAME'];
  }

  final errors = <String>[];
  final semverPattern = RegExp(semverRegex);
  late final String expectedVersion;

  // 1. Determine expected release version from the authoritative source
  if (tagArg != null && tagArg.isNotEmpty) {
    // Git tag (vMAJOR.MINOR.PATCH) is the sole release authority
    if (!tagArg.startsWith('v')) {
      errors.add("Release tag must start with 'v', got: '$tagArg'");
      print('❌ Release tag format error: Tag must start with "v"');
      exit(1);
    }
    final tagVersion = tagArg.substring(1);
    if (!semverPattern.hasMatch(tagVersion)) {
      errors.add(
        "Release tag '$tagArg' does not contain valid SemVer: '$tagVersion'",
      );
      print("❌ Release tag '$tagArg' is not a valid SemVer string.");
      exit(1);
    }
    expectedVersion = tagVersion;
    print("✓ Git tag '$tagArg' is sole release authority (expected version: '$expectedVersion')");
  } else {
    // Non-tag validation (CI on main / dev, local check): derive version from package manifest
    final pubspecFile = File('flutter/pubspec.yaml');
    if (!pubspecFile.existsSync()) {
      print('❌ Error: flutter/pubspec.yaml not found.');
      exit(1);
    }
    final match = RegExp(r'^version:\s*([^\s#]+)', multiLine: true)
        .firstMatch(pubspecFile.readAsStringSync());
    if (match == null || !semverPattern.hasMatch(match.group(1)!)) {
      print('❌ Error: Valid SemVer version not found in flutter/pubspec.yaml');
      exit(1);
    }
    expectedVersion = match.group(1)!;
    print("✓ Derived expected version from flutter/pubspec.yaml: '$expectedVersion'");
  }

  // 2. Validate flutter/pubspec.yaml
  final pubspecFile = File('flutter/pubspec.yaml');
  if (!pubspecFile.existsSync()) {
    errors.add('flutter/pubspec.yaml not found.');
  } else {
    final match = RegExp(r'^version:\s*([^\s#]+)', multiLine: true)
        .firstMatch(pubspecFile.readAsStringSync());
    if (match == null) {
      errors.add('version field not found in flutter/pubspec.yaml');
    } else if (match.group(1) != expectedVersion) {
      errors.add(
        "flutter/pubspec.yaml version '${match.group(1)}' != '$expectedVersion'",
      );
    } else {
      print("✓ flutter/pubspec.yaml version matches '$expectedVersion'");
    }
  }

  // 3. Validate nodejs/package.json
  final packageJsonFile = File('nodejs/package.json');
  if (!packageJsonFile.existsSync()) {
    errors.add('nodejs/package.json not found.');
  } else {
    try {
      final json = jsonDecode(packageJsonFile.readAsStringSync()) as Map<String, dynamic>;
      final pkgVersion = json['version'] as String?;
      if (pkgVersion != expectedVersion) {
        errors.add(
          "nodejs/package.json version '$pkgVersion' != '$expectedVersion'",
        );
      } else {
        print("✓ nodejs/package.json version matches '$expectedVersion'");
      }
    } catch (e) {
      errors.add('Failed to parse nodejs/package.json: $e');
    }
  }

  // 4. Validate iOS podspec
  final iosPodspec = File('flutter/ios/inditrans.podspec');
  if (!iosPodspec.existsSync()) {
    errors.add('flutter/ios/inditrans.podspec not found.');
  } else {
    final match = RegExp(r"s\.version\s*=\s*'([^']+)'")
        .firstMatch(iosPodspec.readAsStringSync());
    if (match == null) {
      errors.add('s.version not found in flutter/ios/inditrans.podspec');
    } else if (match.group(1) != expectedVersion) {
      errors.add(
        "flutter/ios/inditrans.podspec version '${match.group(1)}' != '$expectedVersion'",
      );
    } else {
      print("✓ flutter/ios/inditrans.podspec version matches '$expectedVersion'");
    }
  }

  // 5. Validate macOS podspec
  final macosPodspec = File('flutter/macos/inditrans.podspec');
  if (!macosPodspec.existsSync()) {
    errors.add('flutter/macos/inditrans.podspec not found.');
  } else {
    final match = RegExp(r"s\.version\s*=\s*'([^']+)'")
        .firstMatch(macosPodspec.readAsStringSync());
    if (match == null) {
      errors.add('s.version not found in flutter/macos/inditrans.podspec');
    } else if (match.group(1) != expectedVersion) {
      errors.add(
        "flutter/macos/inditrans.podspec version '${match.group(1)}' != '$expectedVersion'",
      );
    } else {
      print("✓ flutter/macos/inditrans.podspec version matches '$expectedVersion'");
    }
  }

  // 6. Validate Android build.gradle
  final androidGradle = File('flutter/android/build.gradle');
  if (!androidGradle.existsSync()) {
    errors.add('flutter/android/build.gradle not found.');
  } else {
    final match = RegExp(r"""version\s*=?\s*["']([^"']+)["']""")
        .firstMatch(androidGradle.readAsStringSync());
    if (match == null) {
      errors.add('version not found in flutter/android/build.gradle');
    } else if (match.group(1) != expectedVersion) {
      errors.add(
        "flutter/android/build.gradle version '${match.group(1)}' != '$expectedVersion'",
      );
    } else {
      print("✓ flutter/android/build.gradle version matches '$expectedVersion'");
    }
  }

  // 7. Validate Linux CMakeLists.txt
  final linuxCMake = File('flutter/linux/CMakeLists.txt');
  if (!linuxCMake.existsSync()) {
    errors.add('flutter/linux/CMakeLists.txt not found.');
  } else {
    final match = RegExp(r'project\(\$\{PROJECT_NAME\}\s+VERSION\s+([^\s\)]+)')
        .firstMatch(linuxCMake.readAsStringSync());
    if (match == null) {
      errors.add('VERSION not found in flutter/linux/CMakeLists.txt');
    } else if (match.group(1) != expectedVersion) {
      errors.add(
        "flutter/linux/CMakeLists.txt version '${match.group(1)}' != '$expectedVersion'",
      );
    } else {
      print("✓ flutter/linux/CMakeLists.txt version matches '$expectedVersion'");
    }
  }

  // 8. Validate Windows CMakeLists.txt
  final windowsCMake = File('flutter/windows/CMakeLists.txt');
  if (!windowsCMake.existsSync()) {
    errors.add('flutter/windows/CMakeLists.txt not found.');
  } else {
    final match = RegExp(r'project\(\$\{PROJECT_NAME\}\s+VERSION\s+([^\s\)]+)')
        .firstMatch(windowsCMake.readAsStringSync());
    if (match == null) {
      errors.add('VERSION not found in flutter/windows/CMakeLists.txt');
    } else if (match.group(1) != expectedVersion) {
      errors.add(
        "flutter/windows/CMakeLists.txt version '${match.group(1)}' != '$expectedVersion'",
      );
    } else {
      print("✓ flutter/windows/CMakeLists.txt version matches '$expectedVersion'");
    }
  }

  // 9. Validate root CHANGELOG.md
  final changelogFile = File('CHANGELOG.md');
  if (!changelogFile.existsSync()) {
    errors.add('Authoritative CHANGELOG.md not found at repository root.');
  } else {
    final changelogContent = changelogFile.readAsStringSync();
    final escapedVersion = RegExp.escape(expectedVersion);
    final versionHeaderPattern = RegExp('^##\\s*\\[?v?$escapedVersion\\]?.*\$', multiLine: true);
    final match = versionHeaderPattern.firstMatch(changelogContent);
    if (match == null) {
      errors.add(
        "CHANGELOG.md missing release section for version '$expectedVersion' (expected '## [$expectedVersion]' or '## $expectedVersion')",
      );
    } else {
      print("✓ CHANGELOG.md contains release notes for version '$expectedVersion'");
      if (extractChangelogPath != null) {
        final startIdx = match.end;
        final nextHeaderMatch = RegExp(r'^##\s+', multiLine: true).firstMatch(changelogContent.substring(startIdx));
        final sectionContent = nextHeaderMatch != null
            ? changelogContent.substring(startIdx, startIdx + nextHeaderMatch.start).trim()
            : changelogContent.substring(startIdx).trim();
        File(extractChangelogPath).writeAsStringSync('$sectionContent\n');
        print("✓ Extracted release notes to '$extractChangelogPath'");
      }
    }
  }

  // 10. Validate required compiled artifacts
  if (checkArtifacts) {
    final wasmFile = File('flutter/assets/inditrans.wasm');
    if (!wasmFile.existsSync() || wasmFile.lengthSync() == 0) {
      errors.add(
        'Required binary artifact flutter/assets/inditrans.wasm missing or empty.',
      );
    } else {
      print('✓ flutter/assets/inditrans.wasm present (${wasmFile.lengthSync()} bytes)');
    }

    final jsFile = File('js/public/inditrans.js');
    if (!jsFile.existsSync() || jsFile.lengthSync() == 0) {
      errors.add(
        'Required JS wrapper artifact js/public/inditrans.js missing or empty.',
      );
    } else {
      print('✓ js/public/inditrans.js present (${jsFile.lengthSync()} bytes)');
    }
  }

  // 11. Fail-closed, deterministic package publish dry-run checks
  if (checkPackages) {
    print('Running package publish dry-runs...');
    // Prepare temporary package changelog for pub.dev dry-run if not present
    final tempFlutterChangelog = File('flutter/CHANGELOG.md');
    final changelogCreated = !tempFlutterChangelog.existsSync();
    if (changelogCreated && changelogFile.existsSync()) {
      changelogFile.copySync('flutter/CHANGELOG.md');
    }
    try {
      final flutterResult = await Process.run(
        'flutter',
        ['pub', 'publish', '--dry-run'],
        workingDirectory: 'flutter',
      );
      if (flutterResult.exitCode != 0) {
        errors.add(
          'flutter pub publish --dry-run failed with exit code ${flutterResult.exitCode}:\n${flutterResult.stderr}\n${flutterResult.stdout}',
        );
      } else {
        print('✓ flutter pub publish --dry-run passed cleanly');
      }
    } finally {
      if (changelogCreated && tempFlutterChangelog.existsSync()) {
        tempFlutterChangelog.deleteSync();
      }
    }

    // Node.js pack dry-run
    final npmResult = await Process.run(
      'npm',
      ['pack', '--dry-run'],
      workingDirectory: 'nodejs',
    );
    if (npmResult.exitCode != 0) {
      errors.add(
        'npm pack --dry-run failed with exit code ${npmResult.exitCode}:\n${npmResult.stderr}\n${npmResult.stdout}',
      );
    } else {
      print('✓ npm pack --dry-run passed cleanly');
    }
  }

  if (errors.isNotEmpty) {
    print('\n❌ Release validation failed with ${errors.length} error(s):');
    for (final err in errors) {
      print('  - $err');
    }
    exit(1);
  }

  print('\n✨ All release and version validation checks PASSED for $expectedVersion!');
}
;

void main(List<String> args) async {
  String? tagArg;
  String? extractChangelogPath;
  var checkPackages = false;
  var checkArtifacts = true;

  for (final arg in args) {
    if (arg.startsWith('--tag=')) {
      tagArg = arg.substring('--tag='.length);
    } else if (arg.startsWith('--extract-changelog=')) {
      extractChangelogPath = arg.substring('--extract-changelog='.length);
    } else if (arg == '--check-packages') {
      checkPackages = true;
    } else if (arg == '--skip-artifacts') {
      checkArtifacts = false;
    } else if (!arg.startsWith('--') && tagArg == null) {
      tagArg = arg;
    }
  }

  // Fallback to GitHub Actions environment variable if available
  if (tagArg == null && Platform.environment['GITHUB_REF_TYPE'] == 'tag') {
    tagArg = Platform.environment['GITHUB_REF_NAME'];
  }

  final errors = <String>[];
  final semverPattern = RegExp(semverRegex);
  late final String expectedVersion;

  // 1. Determine expected release version from the authoritative source
  if (tagArg != null && tagArg.isNotEmpty) {
    // Git tag (vMAJOR.MINOR.PATCH) is the sole release authority
    if (!tagArg.startsWith('v')) {
      errors.add("Release tag must start with 'v', got: '$tagArg'");
      print('❌ Release tag format error: Tag must start with "v"');
      exit(1);
    }
    final tagVersion = tagArg.substring(1);
    if (!semverPattern.hasMatch(tagVersion)) {
      errors.add(
        "Release tag '$tagArg' does not contain valid SemVer: '$tagVersion'",
      );
      print("❌ Release tag '$tagArg' is not a valid SemVer string.");
      exit(1);
    }
    expectedVersion = tagVersion;
    print("✓ Git tag '$tagArg' is sole release authority (expected version: '$expectedVersion')");
  } else {
    // Non-tag validation (CI on main / dev, local check): derive version from package manifest
    final pubspecFile = File('flutter/pubspec.yaml');
    if (!pubspecFile.existsSync()) {
      print('❌ Error: flutter/pubspec.yaml not found.');
      exit(1);
    }
    final match = RegExp(r'^version:\s*([^\s#]+)', multiLine: true)
        .firstMatch(pubspecFile.readAsStringSync());
    if (match == null || !semverPattern.hasMatch(match.group(1)!)) {
      print('❌ Error: Valid SemVer version not found in flutter/pubspec.yaml');
      exit(1);
    }
    expectedVersion = match.group(1)!;
    print("✓ Derived expected version from flutter/pubspec.yaml: '$expectedVersion'");
  }

  // 2. Validate flutter/pubspec.yaml
  final pubspecFile = File('flutter/pubspec.yaml');
  if (!pubspecFile.existsSync()) {
    errors.add('flutter/pubspec.yaml not found.');
  } else {
    final match = RegExp(r'^version:\s*([^\s#]+)', multiLine: true)
        .firstMatch(pubspecFile.readAsStringSync());
    if (match == null) {
      errors.add('version field not found in flutter/pubspec.yaml');
    } else if (match.group(1) != expectedVersion) {
      errors.add(
        "flutter/pubspec.yaml version '${match.group(1)}' != '$expectedVersion'",
      );
    } else {
      print("✓ flutter/pubspec.yaml version matches '$expectedVersion'");
    }
  }

  // 3. Validate nodejs/package.json
  final packageJsonFile = File('nodejs/package.json');
  if (!packageJsonFile.existsSync()) {
    errors.add('nodejs/package.json not found.');
  } else {
    try {
      final json = jsonDecode(packageJsonFile.readAsStringSync()) as Map<String, dynamic>;
      final pkgVersion = json['version'] as String?;
      if (pkgVersion != expectedVersion) {
        errors.add(
          "nodejs/package.json version '$pkgVersion' != '$expectedVersion'",
        );
      } else {
        print("✓ nodejs/package.json version matches '$expectedVersion'");
      }
    } catch (e) {
      errors.add('Failed to parse nodejs/package.json: $e');
    }
  }

  // 4. Validate iOS podspec
  final iosPodspec = File('flutter/ios/inditrans.podspec');
  if (!iosPodspec.existsSync()) {
    errors.add('flutter/ios/inditrans.podspec not found.');
  } else {
    final match = RegExp(r"s\.version\s*=\s*'([^']+)'")
        .firstMatch(iosPodspec.readAsStringSync());
    if (match == null) {
      errors.add('s.version not found in flutter/ios/inditrans.podspec');
    } else if (match.group(1) != expectedVersion) {
      errors.add(
        "flutter/ios/inditrans.podspec version '${match.group(1)}' != '$expectedVersion'",
      );
    } else {
      print("✓ flutter/ios/inditrans.podspec version matches '$expectedVersion'");
    }
  }

  // 5. Validate macOS podspec
  final macosPodspec = File('flutter/macos/inditrans.podspec');
  if (!macosPodspec.existsSync()) {
    errors.add('flutter/macos/inditrans.podspec not found.');
  } else {
    final match = RegExp(r"s\.version\s*=\s*'([^']+)'")
        .firstMatch(macosPodspec.readAsStringSync());
    if (match == null) {
      errors.add('s.version not found in flutter/macos/inditrans.podspec');
    } else if (match.group(1) != expectedVersion) {
      errors.add(
        "flutter/macos/inditrans.podspec version '${match.group(1)}' != '$expectedVersion'",
      );
    } else {
      print("✓ flutter/macos/inditrans.podspec version matches '$expectedVersion'");
    }
  }

  // 6. Validate Android build.gradle
  final androidGradle = File('flutter/android/build.gradle');
  if (!androidGradle.existsSync()) {
    errors.add('flutter/android/build.gradle not found.');
  } else {
    final match = RegExp(r"""version\s*=?\s*["']([^"']+)["']""")
        .firstMatch(androidGradle.readAsStringSync());
    if (match == null) {
      errors.add('version not found in flutter/android/build.gradle');
    } else if (match.group(1) != expectedVersion) {
      errors.add(
        "flutter/android/build.gradle version '${match.group(1)}' != '$expectedVersion'",
      );
    } else {
      print("✓ flutter/android/build.gradle version matches '$expectedVersion'");
    }
  }

  // 7. Validate Linux CMakeLists.txt
  final linuxCMake = File('flutter/linux/CMakeLists.txt');
  if (!linuxCMake.existsSync()) {
    errors.add('flutter/linux/CMakeLists.txt not found.');
  } else {
    final match = RegExp(r'project\(\$\{PROJECT_NAME\}\s+VERSION\s+([^\s\)]+)')
        .firstMatch(linuxCMake.readAsStringSync());
    if (match == null) {
      errors.add('VERSION not found in flutter/linux/CMakeLists.txt');
    } else if (match.group(1) != expectedVersion) {
      errors.add(
        "flutter/linux/CMakeLists.txt version '${match.group(1)}' != '$expectedVersion'",
      );
    } else {
      print("✓ flutter/linux/CMakeLists.txt version matches '$expectedVersion'");
    }
  }

  // 8. Validate Windows CMakeLists.txt
  final windowsCMake = File('flutter/windows/CMakeLists.txt');
  if (!windowsCMake.existsSync()) {
    errors.add('flutter/windows/CMakeLists.txt not found.');
  } else {
    final match = RegExp(r'project\(\$\{PROJECT_NAME\}\s+VERSION\s+([^\s\)]+)')
        .firstMatch(windowsCMake.readAsStringSync());
    if (match == null) {
      errors.add('VERSION not found in flutter/windows/CMakeLists.txt');
    } else if (match.group(1) != expectedVersion) {
      errors.add(
        "flutter/windows/CMakeLists.txt version '${match.group(1)}' != '$expectedVersion'",
      );
    } else {
      print("✓ flutter/windows/CMakeLists.txt version matches '$expectedVersion'");
    }
  }

  // 9. Validate root CHANGELOG.md
  final changelogFile = File('CHANGELOG.md');
  if (!changelogFile.existsSync()) {
    errors.add('Authoritative CHANGELOG.md not found at repository root.');
  } else {
    final changelogContent = changelogFile.readAsStringSync();
    final escapedVersion = RegExp.escape(expectedVersion);
    final versionHeaderPattern = RegExp('^##\\s*\\[?v?$escapedVersion\\]?.*\$', multiLine: true);
    final match = versionHeaderPattern.firstMatch(changelogContent);
    if (match == null) {
      errors.add(
        "CHANGELOG.md missing release section for version '$expectedVersion' (expected '## [$expectedVersion]' or '## $expectedVersion')",
      );
    } else {
      print("✓ CHANGELOG.md contains release notes for version '$expectedVersion'");
      if (extractChangelogPath != null) {
        final startIdx = match.end;
        final nextHeaderMatch = RegExp(r'^##\s+', multiLine: true).firstMatch(changelogContent.substring(startIdx));
        final sectionContent = nextHeaderMatch != null
            ? changelogContent.substring(startIdx, startIdx + nextHeaderMatch.start).trim()
            : changelogContent.substring(startIdx).trim();
        File(extractChangelogPath).writeAsStringSync('$sectionContent\n');
        print("✓ Extracted release notes to '$extractChangelogPath'");
      }
    }
  }

  // 10. Validate required compiled artifacts
  if (checkArtifacts) {
    final wasmFile = File('flutter/assets/inditrans.wasm');
    if (!wasmFile.existsSync() || wasmFile.lengthSync() == 0) {
      errors.add(
        'Required binary artifact flutter/assets/inditrans.wasm missing or empty.',
      );
    } else {
      print('✓ flutter/assets/inditrans.wasm present (${wasmFile.lengthSync()} bytes)');
    }

    final jsFile = File('js/public/inditrans.js');
    if (!jsFile.existsSync() || jsFile.lengthSync() == 0) {
      errors.add(
        'Required JS wrapper artifact js/public/inditrans.js missing or empty.',
      );
    } else {
      print('✓ js/public/inditrans.js present (${jsFile.lengthSync()} bytes)');
    }
  }

  // 11. Fail-closed, deterministic package publish dry-run checks
  if (checkPackages) {
    print('Running package publish dry-runs...');
    // Prepare temporary package changelog for pub.dev dry-run if not present
    final tempFlutterChangelog = File('flutter/CHANGELOG.md');
    final changelogCreated = !tempFlutterChangelog.existsSync();
    if (changelogCreated && changelogFile.existsSync()) {
      changelogFile.copySync('flutter/CHANGELOG.md');
    }
    try {
      final flutterResult = await Process.run(
        'flutter',
        ['pub', 'publish', '--dry-run'],
        workingDirectory: 'flutter',
      );
      if (flutterResult.exitCode != 0) {
        errors.add(
          'flutter pub publish --dry-run failed with exit code ${flutterResult.exitCode}:\n${flutterResult.stderr}\n${flutterResult.stdout}',
        );
      } else {
        print('✓ flutter pub publish --dry-run passed cleanly');
      }
    } finally {
      if (changelogCreated && tempFlutterChangelog.existsSync()) {
        tempFlutterChangelog.deleteSync();
      }
    }

    // Node.js pack dry-run
    final npmResult = await Process.run(
      'npm',
      ['pack', '--dry-run'],
      workingDirectory: 'nodejs',
    );
    if (npmResult.exitCode != 0) {
      errors.add(
        'npm pack --dry-run failed with exit code ${npmResult.exitCode}:\n${npmResult.stderr}\n${npmResult.stdout}',
      );
    } else {
      print('✓ npm pack --dry-run passed cleanly');
    }
  }

  if (errors.isNotEmpty) {
    print('\n❌ Release validation failed with ${errors.length} error(s):');
    for (final err in errors) {
      print('  - $err');
    }
    exit(1);
  }

  print('\n✨ All release and version validation checks PASSED for $expectedVersion!');
}
;

void main(List<String> args) async {
  String? tagArg;
  String? extractChangelogPath;
  var checkPackages = false;
  var checkArtifacts = true;

  for (final arg in args) {
    if (arg.startsWith('--tag=')) {
      tagArg = arg.substring('--tag='.length);
    } else if (arg.startsWith('--extract-changelog=')) {
      extractChangelogPath = arg.substring('--extract-changelog='.length);
    } else if (arg == '--check-packages') {
      checkPackages = true;
    } else if (arg == '--skip-artifacts') {
      checkArtifacts = false;
    } else if (!arg.startsWith('--') && tagArg == null) {
      tagArg = arg;
    }
  }

  // Fallback to GitHub Actions environment variable if available
  if (tagArg == null && Platform.environment['GITHUB_REF_TYPE'] == 'tag') {
    tagArg = Platform.environment['GITHUB_REF_NAME'];
  }

  final errors = <String>[];
  final semverPattern = RegExp(semverRegex);
  late final String expectedVersion;

  // 1. Determine expected release version from the authoritative source
  if (tagArg != null && tagArg.isNotEmpty) {
    // Git tag (vMAJOR.MINOR.PATCH) is the sole release authority
    if (!tagArg.startsWith('v')) {
      errors.add("Release tag must start with 'v', got: '$tagArg'");
      print('❌ Release tag format error: Tag must start with "v"');
      exit(1);
    }
    final tagVersion = tagArg.substring(1);
    if (!semverPattern.hasMatch(tagVersion)) {
      errors.add(
        "Release tag '$tagArg' does not contain valid SemVer: '$tagVersion'",
      );
      print("❌ Release tag '$tagArg' is not a valid SemVer string.");
      exit(1);
    }
    expectedVersion = tagVersion;
    print("✓ Git tag '$tagArg' is sole release authority (expected version: '$expectedVersion')");
  } else {
    // Non-tag validation (CI on main / dev, local check): derive version from package manifest
    final pubspecFile = File('flutter/pubspec.yaml');
    if (!pubspecFile.existsSync()) {
      print('❌ Error: flutter/pubspec.yaml not found.');
      exit(1);
    }
    final match = RegExp(r'^version:\s*([^\s#]+)', multiLine: true)
        .firstMatch(pubspecFile.readAsStringSync());
    if (match == null || !semverPattern.hasMatch(match.group(1)!)) {
      print('❌ Error: Valid SemVer version not found in flutter/pubspec.yaml');
      exit(1);
    }
    expectedVersion = match.group(1)!;
    print("✓ Derived expected version from flutter/pubspec.yaml: '$expectedVersion'");
  }

  // 2. Validate flutter/pubspec.yaml
  final pubspecFile = File('flutter/pubspec.yaml');
  if (!pubspecFile.existsSync()) {
    errors.add('flutter/pubspec.yaml not found.');
  } else {
    final match = RegExp(r'^version:\s*([^\s#]+)', multiLine: true)
        .firstMatch(pubspecFile.readAsStringSync());
    if (match == null) {
      errors.add('version field not found in flutter/pubspec.yaml');
    } else if (match.group(1) != expectedVersion) {
      errors.add(
        "flutter/pubspec.yaml version '${match.group(1)}' != '$expectedVersion'",
      );
    } else {
      print("✓ flutter/pubspec.yaml version matches '$expectedVersion'");
    }
  }

  // 3. Validate nodejs/package.json
  final packageJsonFile = File('nodejs/package.json');
  if (!packageJsonFile.existsSync()) {
    errors.add('nodejs/package.json not found.');
  } else {
    try {
      final json = jsonDecode(packageJsonFile.readAsStringSync()) as Map<String, dynamic>;
      final pkgVersion = json['version'] as String?;
      if (pkgVersion != expectedVersion) {
        errors.add(
          "nodejs/package.json version '$pkgVersion' != '$expectedVersion'",
        );
      } else {
        print("✓ nodejs/package.json version matches '$expectedVersion'");
      }
    } catch (e) {
      errors.add('Failed to parse nodejs/package.json: $e');
    }
  }

  // 4. Validate iOS podspec
  final iosPodspec = File('flutter/ios/inditrans.podspec');
  if (!iosPodspec.existsSync()) {
    errors.add('flutter/ios/inditrans.podspec not found.');
  } else {
    final match = RegExp(r"s\.version\s*=\s*'([^']+)'")
        .firstMatch(iosPodspec.readAsStringSync());
    if (match == null) {
      errors.add('s.version not found in flutter/ios/inditrans.podspec');
    } else if (match.group(1) != expectedVersion) {
      errors.add(
        "flutter/ios/inditrans.podspec version '${match.group(1)}' != '$expectedVersion'",
      );
    } else {
      print("✓ flutter/ios/inditrans.podspec version matches '$expectedVersion'");
    }
  }

  // 5. Validate macOS podspec
  final macosPodspec = File('flutter/macos/inditrans.podspec');
  if (!macosPodspec.existsSync()) {
    errors.add('flutter/macos/inditrans.podspec not found.');
  } else {
    final match = RegExp(r"s\.version\s*=\s*'([^']+)'")
        .firstMatch(macosPodspec.readAsStringSync());
    if (match == null) {
      errors.add('s.version not found in flutter/macos/inditrans.podspec');
    } else if (match.group(1) != expectedVersion) {
      errors.add(
        "flutter/macos/inditrans.podspec version '${match.group(1)}' != '$expectedVersion'",
      );
    } else {
      print("✓ flutter/macos/inditrans.podspec version matches '$expectedVersion'");
    }
  }

  // 6. Validate Android build.gradle
  final androidGradle = File('flutter/android/build.gradle');
  if (!androidGradle.existsSync()) {
    errors.add('flutter/android/build.gradle not found.');
  } else {
    final match = RegExp(r"""version\s*=?\s*["']([^"']+)["']""")
        .firstMatch(androidGradle.readAsStringSync());
    if (match == null) {
      errors.add('version not found in flutter/android/build.gradle');
    } else if (match.group(1) != expectedVersion) {
      errors.add(
        "flutter/android/build.gradle version '${match.group(1)}' != '$expectedVersion'",
      );
    } else {
      print("✓ flutter/android/build.gradle version matches '$expectedVersion'");
    }
  }

  // 7. Validate Linux CMakeLists.txt
  final linuxCMake = File('flutter/linux/CMakeLists.txt');
  if (!linuxCMake.existsSync()) {
    errors.add('flutter/linux/CMakeLists.txt not found.');
  } else {
    final match = RegExp(r'project\(\$\{PROJECT_NAME\}\s+VERSION\s+([^\s\)]+)')
        .firstMatch(linuxCMake.readAsStringSync());
    if (match == null) {
      errors.add('VERSION not found in flutter/linux/CMakeLists.txt');
    } else if (match.group(1) != expectedVersion) {
      errors.add(
        "flutter/linux/CMakeLists.txt version '${match.group(1)}' != '$expectedVersion'",
      );
    } else {
      print("✓ flutter/linux/CMakeLists.txt version matches '$expectedVersion'");
    }
  }

  // 8. Validate Windows CMakeLists.txt
  final windowsCMake = File('flutter/windows/CMakeLists.txt');
  if (!windowsCMake.existsSync()) {
    errors.add('flutter/windows/CMakeLists.txt not found.');
  } else {
    final match = RegExp(r'project\(\$\{PROJECT_NAME\}\s+VERSION\s+([^\s\)]+)')
        .firstMatch(windowsCMake.readAsStringSync());
    if (match == null) {
      errors.add('VERSION not found in flutter/windows/CMakeLists.txt');
    } else if (match.group(1) != expectedVersion) {
      errors.add(
        "flutter/windows/CMakeLists.txt version '${match.group(1)}' != '$expectedVersion'",
      );
    } else {
      print("✓ flutter/windows/CMakeLists.txt version matches '$expectedVersion'");
    }
  }

  // 9. Validate root CHANGELOG.md
  final changelogFile = File('CHANGELOG.md');
  if (!changelogFile.existsSync()) {
    errors.add('Authoritative CHANGELOG.md not found at repository root.');
  } else {
    final changelogContent = changelogFile.readAsStringSync();
    final escapedVersion = RegExp.escape(expectedVersion);
    final versionHeaderPattern = RegExp('^##\\s*\\[?v?$escapedVersion\\]?.*\$', multiLine: true);
    final match = versionHeaderPattern.firstMatch(changelogContent);
    if (match == null) {
      errors.add(
        "CHANGELOG.md missing release section for version '$expectedVersion' (expected '## [$expectedVersion]' or '## $expectedVersion')",
      );
    } else {
      print("✓ CHANGELOG.md contains release notes for version '$expectedVersion'");
      if (extractChangelogPath != null) {
        final startIdx = match.end;
        final nextHeaderMatch = RegExp(r'^##\s+', multiLine: true).firstMatch(changelogContent.substring(startIdx));
        final sectionContent = nextHeaderMatch != null
            ? changelogContent.substring(startIdx, startIdx + nextHeaderMatch.start).trim()
            : changelogContent.substring(startIdx).trim();
        File(extractChangelogPath).writeAsStringSync('$sectionContent\n');
        print("✓ Extracted release notes to '$extractChangelogPath'");
      }
    }
  }

  // 10. Validate required compiled artifacts
  if (checkArtifacts) {
    final wasmFile = File('flutter/assets/inditrans.wasm');
    if (!wasmFile.existsSync() || wasmFile.lengthSync() == 0) {
      errors.add(
        'Required binary artifact flutter/assets/inditrans.wasm missing or empty.',
      );
    } else {
      print('✓ flutter/assets/inditrans.wasm present (${wasmFile.lengthSync()} bytes)');
    }

    final jsFile = File('js/public/inditrans.js');
    if (!jsFile.existsSync() || jsFile.lengthSync() == 0) {
      errors.add(
        'Required JS wrapper artifact js/public/inditrans.js missing or empty.',
      );
    } else {
      print('✓ js/public/inditrans.js present (${jsFile.lengthSync()} bytes)');
    }
  }

  // 11. Fail-closed, deterministic package publish dry-run checks
  if (checkPackages) {
    print('Running package publish dry-runs...');
    // Prepare temporary package changelog for pub.dev dry-run if not present
    final tempFlutterChangelog = File('flutter/CHANGELOG.md');
    final changelogCreated = !tempFlutterChangelog.existsSync();
    if (changelogCreated && changelogFile.existsSync()) {
      changelogFile.copySync('flutter/CHANGELOG.md');
    }
    try {
      final flutterResult = await Process.run(
        'flutter',
        ['pub', 'publish', '--dry-run'],
        workingDirectory: 'flutter',
      );
      if (flutterResult.exitCode != 0) {
        errors.add(
          'flutter pub publish --dry-run failed with exit code ${flutterResult.exitCode}:\n${flutterResult.stderr}\n${flutterResult.stdout}',
        );
      } else {
        print('✓ flutter pub publish --dry-run passed cleanly');
      }
    } finally {
      if (changelogCreated && tempFlutterChangelog.existsSync()) {
        tempFlutterChangelog.deleteSync();
      }
    }

    // Node.js pack dry-run
    final npmResult = await Process.run(
      'npm',
      ['pack', '--dry-run'],
      workingDirectory: 'nodejs',
    );
    if (npmResult.exitCode != 0) {
      errors.add(
        'npm pack --dry-run failed with exit code ${npmResult.exitCode}:\n${npmResult.stderr}\n${npmResult.stdout}',
      );
    } else {
      print('✓ npm pack --dry-run passed cleanly');
    }
  }

  if (errors.isNotEmpty) {
    print('\n❌ Release validation failed with ${errors.length} error(s):');
    for (final err in errors) {
      print('  - $err');
    }
    exit(1);
  }

  print('\n✨ All release and version validation checks PASSED for $expectedVersion!');
}
