#include "static_scripts.h"
#include <chrono>
#include <cstdio>

#ifdef INDTRANSLIT_ALLOC_PROBE
#include <dlfcn.h>
#endif

namespace data = inditrans::static_data;

template <ReaderPolicy Policy> uint64_t exerciseLookup(const ScriptReaderMap& reader, size_t repetitions) {
  uint64_t sum = 0;
  for (size_t run = 0; run < repetitions; ++run) {
    for (const auto& entry : data::readerEntries0) {
      const auto match = reader.lookupToken<Policy>(entry.key.data(), entry.key.data() + entry.key.size());
      sum += match.matchLen;
      for (const auto& token : match.tokens())
        sum += token.idx + uint64_t(token.scriptType) * 256;
    }
  }
  return sum;
}

uint64_t exerciseMetadata() {
  uint64_t sum = 0;
  for (const auto& entry : data::names) {
    const auto* reader = getScriptReaderMap(entry.name);
    const auto* writer = getScriptWriterMap(entry.name);
    sum += data::findScript(entry.name) + 1;
    if (reader) {
      for (std::string_view text : { "க", "क़", "KH", "🙂" }) {
        const auto match = reader->lookupToken(text.data(), text.data() + text.size());
        sum += match.matchLen + match.tokens().size();
      }
    }
    for (size_t type = 0; type < 8; ++type)
      sum += writer->lookupChar(static_cast<TokenType>(type), 0).size();
  }
  const auto* reader = getScriptReaderMap("indic");
  constexpr std::string_view text = "க²";
  sum += reader->lookupToken(text.data(), text.data() + text.size()).matchLen;
  TamilPrefixLookup::LookupState state { };
  const auto prefix = TamilPrefixLookup { };
  prefix.lookup({ { TokenType::Vowel, 0, ScriptType::Indic } }, state);
  TokenUnit dental { { TokenType::Consonant, 17, ScriptType::Indic } };
  dental.vowelMark = { TokenType::VowelMark, 4 };
  prefix.lookup(dental, state);
  return sum + state.matchLen;
}

int main(int argc, char** argv) {
#ifdef INDTRANSLIT_ALLOC_PROBE
  if (argc == 2 && std::string_view(argv[1]) == "--alloc") {
    using Probe = void (*)();
    const auto reset = reinterpret_cast<Probe>(dlsym(RTLD_DEFAULT, "alloc_probe_reset"));
    const auto report = reinterpret_cast<Probe>(dlsym(RTLD_DEFAULT, "alloc_probe_report"));
    if (!reset || !report)
      return 2;
    reset();
    const auto sum = exerciseMetadata();
    report();
    std::printf("metadata_checksum=%llu\n", static_cast<unsigned long long>(sum));
    return sum ? 0 : 1;
  }
#else
  (void)argc;
  (void)argv;
#endif

  constexpr size_t repetitions = 1000;
  std::puts("source,lookups,ns_per_lookup,checksum");
  // Every explicit non-Roman reader and indic sees the same union keys. This
  // includes accepted spellings, compounds, and source-rejected terminals.
  for (size_t id = 0; id <= data::writers.size(); ++id) {
    if (id < data::writers.size() && data::writers[id].getType() == ScriptType::Latin)
      continue;
    const auto* reader = id == data::writers.size() ? &data::readers.back() : &data::readers[id * 2];
    std::string_view name = "indic";
    for (const auto& entry : data::names)
      if (entry.script == id) {
        name = entry.name;
        break;
      }
    const auto start = std::chrono::steady_clock::now();
    const auto sum = reader->source ? exerciseLookup<ReaderPolicy::Explicit>(*reader, repetitions)
                                    : exerciseLookup<ReaderPolicy::Indic>(*reader, repetitions);
    const auto end = std::chrono::steady_clock::now();
    const auto lookups = repetitions * data::readerEntries0.size();
    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    std::printf("%.*s,%zu,%.3f,%llu\n", int(name.size()), name.data(), lookups, double(ns) / lookups,
        static_cast<unsigned long long>(sum));
  }
}
