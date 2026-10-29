"""Parse IPA into the engine's shared token inventory, independent of source script."""
import re
import unicodedata


class IpaReader:
    def __init__(self, table, aliases=None):
        mapping = {}
        excluded = set(table.get('readerExclusions', ()))
        for kind, field in [('v', 'vowels'), ('c', 'consonants')]:
            for index, spelling in enumerate(table[field]):
                if f'{kind}:{index}' not in excluded:
                    mapping.setdefault(unicodedata.normalize('NFD', spelling), (kind, index))
        for key, alternatives in table.get('equivalents', {}).items():
            if re.fullmatch('[vc]:[0-9]+', key):
                kind, index = key.split(':')
                target = (kind, int(index))
            elif unicodedata.normalize('NFD', key) in mapping:
                target = mapping[unicodedata.normalize('NFD', key)]
            else:
                continue
            for spelling in alternatives:
                mapping[unicodedata.normalize('NFD', spelling)] = target
        for spelling, token in (aliases or {}).items():
            kind, index = token.split(':')
            index = int(index)
            if kind not in ('v', 'c') or not 0 <= index < len(table['vowels' if kind == 'v' else 'consonants']):
                raise ValueError('invalid IPA compatibility alias: ' + token)
            mapping[unicodedata.normalize('NFD', spelling)] = (kind, index)
        self.starts = {}
        for symbol, token in sorted(mapping.items(), key=lambda item: (-len(item[0]), item[0])):
            self.starts.setdefault(symbol[0], []).append((symbol, token))
        self.aspirated = {}
        for spelling, (kind, index) in mapping.items():
            if kind == 'c' and spelling[-1:] in ('ʰ', 'ʱ') and spelling[:-1] in mapping:
                base_kind, base_index = mapping[spelling[:-1]]
                if base_kind == 'c':
                    self.aspirated[base_index] = index

    def parse(self, text):
        text = unicodedata.normalize('NFD', text.strip())
        if len(text) >= 2 and (text[0], text[-1]) in {('/', '/'), ('[', ']')}:
            text = text[1:-1].strip()
        text = text.translate(str.maketrans({'ˈ': ' ', 'ˌ': ' ', '.': ' '}))
        text = text.replace('\u0303ː', 'ː\u0303')
        result = []
        last_vowel = False
        for chunk in text.split():
            offset = 0
            while offset < len(chunk):
                char = chunk[offset]
                if char == '\u0303':
                    if not last_vowel or 'o' in result[-1]:
                        raise ValueError('nasalization without a vowel')
                    result[-1]['o'] = 0
                    offset += 1
                    continue
                if char in ('ʰ', 'ʱ') and result and 'c' in result[-1] and result[-1].get('m') == 0:
                    index = result[-1]['c']
                    if index not in self.aspirated:
                        raise ValueError('aspiration on unsupported consonant')
                    result[-1]['c'] = self.aspirated[index]
                    offset += 1
                    continue
                if char == 'ː' and result and 'c' in result[-1] and result[-1].get('m') == 0:
                    result.append(dict(result[-1]))
                    offset += 1
                    continue
                match = next(((symbol, token) for symbol, token in self.starts.get(char, ())
                              if chunk.startswith(symbol, offset)), None)
                if match is None:
                    raise ValueError('unsupported IPA symbol ' + repr(chunk[offset:offset + 4]))
                symbol, (kind, index) = match
                offset += len(symbol)
                if kind == 'c':
                    result.append({'c': index, 'm': 0})
                    last_vowel = False
                else:
                    if result and 'c' in result[-1] and result[-1].get('m') == 0 and 'o' not in result[-1]:
                        if index:
                            result[-1]['m'] = index
                        else:
                            del result[-1]['m']
                    else:
                        result.append({'v': index})
                    last_vowel = True
        if not result:
            raise ValueError('empty IPA reading')
        return result
