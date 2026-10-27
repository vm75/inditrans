"""Native readings must remain independent of target approximations."""
import copy
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from python.codegen_data import ScriptData, ScriptInfo, RomanEquivalents
from python.static_scripts import StaticScripts


class PhoneticDataTest(unittest.TestCase):
    def setUp(self):
        data = ScriptData(ROOT / 'scripts/script_data.json', RomanEquivalents(ROOT / 'docs/extended-roman.txt'))
        data.prepare_equivalents()
        self.scripts = {s.name: s for s in data.script_info_list}
        self.generator = StaticScripts(data.script_info_list, ROOT / 'scripts/reader_data.json')

    def reading(self, name, text):
        table = self.generator._reader(self.scripts[name])
        match = self.generator._match(table, text, False)
        self.assertEqual(match.length, len(text))
        return [(t.type, t.index) for t in (match.token.lead, *match.token.extra)]

    def test_native_vowels_ignore_writer_approximation_order(self):
        for script, text, identity in [('bengali','এ',11),('assamese','এ',11),('gurmukhi','ਏ',11),('telugu','ఎ',10),('telugu','ఏ',11),('sinhala','ඇ',16),('sinhala','ඈ',17),('devanagari','ऍ',19)]:
            with self.subTest(script=script,text=text):
                self.assertEqual(self.reading(script,text), [(0,identity)])

    def test_new_native_consonants_and_sound_signs(self):
        for script,text,kind,index in [('telugu','ౘ',2,50),('telugu','ౙ',2,63),('malayalam','ഺ',2,52),('tibetan','ཞ',2,53),('gurmukhi','ੰ',3,7),('devanagari','ᳵ',3,4),('kannada','ೲ',3,5),('malayalam','ു്',1,20),('oriya','ଵ',2,28),('oriya','ୱ',2,54),('oriya','ବ଼',2,28)]:
            with self.subTest(script=script,text=text):
                self.assertEqual(self.reading(script,text),[(kind,index)])

    def test_all_flagged_iso_inputs_are_recognized(self):
        for text in ['ŭ','ê','ṃ','n̆','ẖ','ḫ','ĉ','ĉh','ṯ','r̆',"'",'ž','w','s̱','s̤','h̤','t̤','ʻ','ẕ','ż','ẓ']:
            with self.subTest(text=text): self.reading('iso',text)

    def test_invalid_alias_does_not_wrap_or_partially_resolve(self):
        for target in ['c:256','c:-1','c:64','does-not-exist','क?']:
            with self.subTest(target=target):
                script=copy.deepcopy(self.scripts['devanagari'])
                script.info.setdefault('equivalents',{})[target]=['test-alias']
                with self.assertRaises(ValueError): self.generator._reader(script)

    def test_unexplained_collision_is_rejected(self):
        script=copy.deepcopy(self.scripts['devanagari'])
        script.info['vowels'][1]='अ'
        with self.assertRaisesRegex(ValueError,'ambiguous reading'): self.generator._reader(script)

    def test_genuine_native_accent_is_not_a_synthetic_vowel(self):
        table=self.generator._reader(self.scripts['telugu'])
        match=self.generator._match(table,'ె॒',False)
        self.assertEqual(match.length,1)
        self.assertEqual(match.token.lead.index,10)

    def test_source_aliases_accept_canonical_equivalents(self):
        self.assertEqual(self.reading('malayalam','ൌ'), self.reading('malayalam','ൌ'))
        self.assertEqual(self.reading('iso','ê̄'), self.reading('iso','ê̄'))

    def test_grantha_replaces_tamil_extended_and_reads_native_signs(self):
        self.assertNotIn('tamilExtended', self.scripts)
        for text, kind, index in [('𑌏', 0, 11), ('𑌓', 0, 14), ('𑌳', 2, 33),
                                  ('𑍍', 1, 0), ('𑍋', 1, 14), ('𑍌', 1, 15),
                                  ('𑍐', 5, 10), ('𑍞', 6, 1), ('𑍟', 6, 2)]:
            with self.subTest(text=text):
                self.assertEqual(self.reading('grantha', text), [(kind, index)])

    def test_every_writer_covers_inventory(self):
        for script in self.scripts.values():
            for kind,count in [('vowels',22),('vowelMarks',22),('consonants',64),('otherDiacritics',8)]:
                self.assertEqual(len(script.info[kind]),count,(script.name,kind))
                if kind != 'vowelMarks': self.assertTrue(all(script.info[kind]),(script.name,kind))
            if script.category != 'roman':
                for kind in ('vowels','vowelMarks'):
                    self.assertFalse(any('᳴' in s or '\u200b' in s for s in script.info[kind]),script.name)
            self.generator._reader(script)

    def test_family_and_processing_type_are_independent(self):
        groups = json.loads((ROOT / 'scripts/script_data.json').read_text())
        self.assertEqual(set(groups), {'indic', 'roman', 'persoArabic'})
        self.assertEqual(self.scripts['tamil'].category, 'indic')
        self.assertEqual(self.generator.script_type(self.scripts['tamil']), 'Tamil')
        self.assertEqual(self.reading('tamil', 'க'), [(2, 0)])
        token = self.generator._reader(self.scripts['tamil'])['க'].lead
        self.assertEqual(token.script, 'Tamil')
        for script in self.scripts.values():
            expected = {'indic': 'Indic', 'roman': 'Roman', 'persoArabic': 'PersoArabic'}[script.category]
            if script.name == 'tamil': expected = 'Tamil'
            self.assertEqual(self.generator.script_type(script), expected)

    def test_accent_output_capability_is_explicit_for_every_script(self):
        unsupported = {'wx', 'slp1', 'grantha', 'burmese', 'khmer', 'sinhala', 'thai', 'tibetan', 'urdu'}
        for name, script in self.scripts.items():
            with self.subTest(script=name):
                self.assertIs(script.info['supportsVedicAccents'], name not in unsupported)
        # Accent output support must not restrict native input recognition.
        self.assertEqual(self.reading('sinhala', '॒'), [(4, 0)])

    def test_invalid_family_and_capability_metadata_are_rejected(self):
        for family, info in [
            ('vedic', {'supportsVedicAccents': True}),
            ('tamil', {'supportsVedicAccents': True}),
            ('indic', {}),
            ('indic', {'supportsVedicAccents': 'false'}),
            ('indic', {'supportsVedicAccents': True, 'processingType': 'unknown'}),
            ('roman', {'supportsVedicAccents': True, 'processingType': 'tamil'}),
        ]:
            with self.subTest(family=family, info=info):
                with self.assertRaises(ValueError): ScriptInfo(family, 'test', info)
