import 'dart:convert';
import 'dart:io';

const changeLogFiles = ["flutter/CHANGELOG.md"];
const filesToUpdate = [
  ".version",
  "flutter/pubspec.yaml",
  "flutter/ios/inditrans.podspec",
  "flutter/macos/inditrans.podspec",
];

class Version {
  final int _major, _minor, _patch;

  Version(this._major, this._minor, this._patch);

  Version bumpMajor() {
    return Version(_major + 1, 0, 0);
  }

  Version bumpMinor() {
    return Version(_major, _minor + 1, 0);
  }

  Version bumpPatch() {
    return Version(_major, _minor, _patch + 1);
  }

  @override
  String toString() {
    return '$_major.$_minor.$_patch';
  }

  static Version fromString(String versionStr) {
    final versionParts =
        versionStr.split('.').map((str) => int.parse(str)).toList();
    return Version(versionParts[0], versionParts[1], versionParts[2]);
  }
}

void updateVersion(Version from, Version to) {
  for (final path in filesToUpdate) {
    final file = File(path);
    final contents = file.readAsStringSync();
    file.writeAsStringSync(contents.replaceAll(from.toString(), to.toString()));
  }
}

void changeLog(Version to, String log) {
  for (final path in changeLogFiles) {
    final file = File(path);
    final contents = file.readAsStringSync();
    final prefix = "## $to\n\n- $log\n\n";
    file.writeAsStringSync(prefix);
    file.writeAsStringSync(contents, mode: FileMode.append);
  }
}

List<String> getopt(String parseOptions, List<String> args) {
  final optString = ",$parseOptions,";
  var stopParsing = false;
  List<String> options = [];
  List<String> result = [];
  outer:
  while (!args.isEmpty) {
    var nextArg = args.removeAt(0);
    if (nextArg == "--") {
      stopParsing = true;
      continue;
    }

    if (!stopParsing && !nextArg.isEmpty) {
      switch (nextArg) {
        case '--d':
          continue outer;
        case '--t':
          continue outer;
        case '--h':
          continue outer;
      }
      if (optString.contains(RegExp('.*,$nextArg:,.*'))) {
        options.add(nextArg);
        if (!args.isEmpty) {
          options.add(args.removeAt(0));
        }
        continue outer;
      } else if (optString.contains(RegExp('.*,$nextArg,.*'))) {
        options.add(nextArg);
        continue outer;
      }
    }
    result.add(nextArg);
  }
  if (parseOptions.isEmpty) {
    return result;
  } else {
    options.add('--');
    options.addAll(result);
    return options;
  }
}

void main(List<String> args) {
  final rootDir = Directory.current;

  final versionFile = File("${rootDir.path}/.version");
  if (!versionFile.existsSync()) {
    print("Run from root folder");
    return;
  }
  final currentVersion = Version.fromString(versionFile.readAsStringSync());
  Version? nextVersion;

  if (args.length > 0) {
    if (args.first == '-M') {
      nextVersion = currentVersion.bumpMajor();
    } else if (args.first == '-m') {
      nextVersion = currentVersion.bumpMinor();
    } else if (args.first == '-p') {
      nextVersion = currentVersion.bumpPatch();
    }
    if (nextVersion != null) {
      args.removeAt(0);
    }
  }
  if (nextVersion == null) {
    stdout.write("Enter new version (current version is $currentVersion): ");
    final versionStr = stdin.readLineSync(encoding: utf8);
    if (versionStr == null) {
      return;
    }
    nextVersion = Version.fromString(versionStr);
  }
  late String? log;
  if (!args.isEmpty) {
    log = args.first;
  } else {
    stdout.write("Enter changelog: ");
    log = stdin.readLineSync(encoding: utf8);
    if (log == null) {
      return;
    }
  }
  updateVersion(currentVersion, nextVersion);
  changeLog(nextVersion, log);

  print(
      "Updated version from '$currentVersion' to $nextVersion with log: $log");
}
