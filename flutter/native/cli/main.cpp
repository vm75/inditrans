#include <fstream>
#include <inditrans.h>
#include <iostream>
#include <utilities.h>
#include <vector>

using InditransLogger = void(const std::string&);
extern InditransLogger* inditransLogger;

TranslitOptions getTranslitOptions(const std::string_view& optStr) noexcept {
  Utf8String str(optStr);
  static constexpr std::array<std::string_view, static_cast<size_t>(MaxTranslitOptions)> optionStrings = {
    "None",
    "TamilTraditional",
    "TamilSuperscripted",
    "ASCIINumerals",
    "IgnoreVedicAccents",
    "ShowQuotedMarkers",
  };

  TranslitOptions mask { TranslitOptions::None };
  for (const auto& opt : str.split(" \t,:;/|&")) {
    auto match = std::find(optionStrings.begin(), optionStrings.end(), opt);
    if (match != optionStrings.end()) {
      const auto idx = std::distance(optionStrings.begin(), match);
      if (idx > 0) {
        mask = mask | static_cast<TranslitOptions>(1 << (idx - 1));
      }
    }
  }

  return mask;
}

std::vector<std::string> processArgs(std::string parseOptions, int argc, char** argv) {
  auto optString = "," + parseOptions + ",";
  bool stopParsing = false;
  std::vector<std::string> options {};
  std::vector<std::string> result {};

  int i = 1;
  while (i < argc) {
    std::string nextArg { argv[i++] };
    if (nextArg == "--") {
      stopParsing = true;
      continue;
    }

    if (!stopParsing && nextArg != "") {
      if (optString.find("," + nextArg + ":,") != std::string::npos) {
        options.push_back(nextArg);
        if (i < argc) {
          options.push_back(argv[i++]);
        }
        continue;
      } else if (optString.find("," + nextArg + ",") != std::string::npos) {
        options.push_back(nextArg);
        continue;
      }
    }
    result.push_back(nextArg);
  }

  options.push_back("--");
  options.insert(options.end(), result.begin(), result.end());
  return options;
}

int main(int argc, char** argv) {
  // parse args
  //   -f from-script, default: "itrans"
  //   -t to-script, default: "devanagari"
  //   -o options

  std::string fromScript { "itrans" };
  std::string toScript { "devanagari" };
  TranslitOptions options { TranslitOptions::None };
  auto args = processArgs("-f:,-t:,-o:", argc, argv);
  int i = 0;
  for (; i < args.size(); i++) {
    if (args[i] == "--") {
      i++;
      break;
    }
    if (args[i] == "-f") {
      fromScript = args[++i];
    } else if (args[i] == "-t") {
      toScript = args[++i];
    } else if (args[i] == "-o") {
      options = getTranslitOptions(args[++i]);
    }
  }

  std::string input;
  if (i < args.size()) {
    while (i < args.size()) {
      input += args[i++] + " ";
    }
  }

  while (input != "exit" && input != "quit") {
    if (!input.empty()) {
      std::string output = transliterate(input, fromScript, toScript, options);
      std::cout << output << std::endl;
    }

    std::cout << "$ ";
    std::getline(std::cin, input); // read a line from cin into input

    if (input.starts_with("-f ")) {
      fromScript = input.substr(3);
      input = "";
    } else if (input.starts_with("-t ")) {
      toScript = input.substr(3);
      input = "";
    } else if (input.starts_with("-o ")) {
      options = getTranslitOptions(input.substr(3));
      input = "";
    }
  }

  return 0;
}