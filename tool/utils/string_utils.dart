import 'dart:io';

void replaceByDelimiters(
  String path,
  String start,
  String end,
  StringBuffer replacement,
) {
  final genFile = File(path);
  final contents = genFile.readAsStringSync();

  final startNl = start.endsWith('\n') ? start : '$start\n';

  final startIdx = contents.indexOf(startNl);
  final endIdx = contents.indexOf(end, startIdx);
  assert(startIdx != -1 && endIdx != -1);

  genFile.writeAsStringSync(
    contents.replaceRange(
      startIdx + startNl.length,
      endIdx,
      replacement.toString(),
    ),
  );
}
