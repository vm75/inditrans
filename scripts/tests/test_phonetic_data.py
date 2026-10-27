"""Native readings must remain independent of target approximations."""
import copy
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from python.codegen_data import ScriptData, LatinEquivalents
from python.static_scripts import StaticScripts


class PhoneticDataTest(unittest.TestCase):
    def setUp(self):
        data = ScriptData(ROOT / 'scripts/script_data.json', LatinEquivalents(ROOT / 'docs/extended-latin.txt'))
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

    def test_every_writer_covers_inventory(self):
        for script in self.scripts.values():
            for kind,count in [('vowels',22),('vowelMarks',22),('consonants',64),('otherDiacritics',8)]:
                self.assertEqual(len(script.info[kind]),count,(script.name,kind))
                if kind != 'vowelMarks': self.assertTrue(all(script.info[kind]),(script.name,kind))
            if script.category != 'latin':
                for kind in ('vowels','vowelMarks'):
                    self.assertFalse(any('᳴' in s or '\u200b' in s for s in script.info[kind]),script.name)
            self.generator._reader(script)
