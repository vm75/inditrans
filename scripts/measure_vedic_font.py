#!/usr/bin/env python3
# /// script
# requires-python = ">=3.10"
# dependencies = ["fonttools[woff]==4.67.0", "uharfbuzz==0.57.0"]
# ///
"""Measure pinned native and merged Indic font metrics and shaped accent geometry."""
import hashlib
import io
import json
from pathlib import Path
from urllib.request import urlopen
import uharfbuzz as hb
from fontTools.pens.boundsPen import BoundsPen
from fontTools.ttLib import TTFont
from generate_demo_font import ROOT, BASE_URL, SOURCES, SCRIPT_TAGS
from refine_vedic_anchors import accent_glyph_names, TARGET_GAP

SAMPLES = {
 "Devanagari": ["अ॑", "अ॒", "क॑", "क॒", "कि॑", "कु॒", "क्ति॑", "क्त॒"],
 "Bengali": ["অ॑", "অ॒", "ক॑", "ক॒", "কি॑", "কু॒", "ক্তি॑", "ক্ত॒"],
 "Gurmukhi": ["ਅ॑", "ਅ॒", "ਕ॑", "ਕ॒", "ਕਿ॑", "ਕੁ॒", "ਕ੍ਤਿ॑", "ਕ੍ਤ॒"],
 "Gujarati": ["અ॑", "અ॒", "ક॑", "ક॒", "કિ॑", "કુ॒", "ક્તિ॑", "ક્ત॒"],
 "Oriya": ["ଅ॑", "ଅ॒", "କ॑", "କ॒", "କି॑", "କୁ॒", "କ୍ତି॑", "କ୍ତ॒"],
 "Tamil": ["அ॑", "அ॒", "க॑", "க॒", "கி॑", "கு॒", "க்தி॑", "க்த॒", "க²॑", "கி³॒", "க்ஷ⁴॑"],
 "Telugu": ["అ॑", "అ॒", "క॑", "క॒", "కి॑", "కు॒", "క్తి॑", "క్త॒"],
 "Kannada": ["ಅ॑", "ಅ॒", "ಕ॑", "ಕ॒", "ಕಿ॑", "ಕು॒", "ಕ್ತಿ॑", "ಕ್ತ॒"],
 "Malayalam": ["അ॑", "അ॒", "ക॑", "ക॒", "കി॑", "കു॒", "ക്തി॑", "ക്ത॒"],
 "Sinhala": ["අ॑", "අ॒", "ක॑", "ක॒", "කි॑", "කු॒", "ක්ති॑", "ක්ත॒"],
}

def font_bytes(font):
 font.flavor = None
 out = io.BytesIO()
 font.save(out)
 return out.getvalue()

def metrics(font):
 head, hhea, os2 = (font[t] for t in ("head", "hhea", "OS/2"))
 return {"upem":head.unitsPerEm, "head_bbox":[head.yMin,head.yMax],
         "hhea":[hhea.ascent,hhea.descent,hhea.lineGap],
         "typo":[os2.sTypoAscender,os2.sTypoDescender,os2.sTypoLineGap],
         "win":[os2.usWinAscent,os2.usWinDescent], "fsSelection":os2.fsSelection}

def layout(font, script):
 if "GPOS" not in font: return {}
 table = font["GPOS"].table
 cmap = font.getBestCmap()
 result = {}
 for rec in table.ScriptList.ScriptRecord:
  if rec.ScriptTag not in SCRIPT_TAGS[script]: continue
  lang = rec.Script.DefaultLangSys
  if lang is None: continue
  entries = {}
  for fi in lang.FeatureIndex:
   feature = table.FeatureList.FeatureRecord[fi]
   if feature.FeatureTag not in ("mark", "mkmk"): continue
   for li in feature.Feature.LookupListIndex:
    lookup = table.LookupList.Lookup[li]
    for sub in lookup.SubTable:
     sub = getattr(sub,"ExtSubTable",sub)
     marked = getattr(getattr(sub,"MarkCoverage",None),"glyphs",[])
     bases = getattr(getattr(sub,"BaseCoverage",None),"glyphs",[])
     entries.setdefault(feature.FeatureTag,[]).append(
      [li, lookup.LookupType, len(marked), len(bases),
       cmap.get(0x951) in marked, cmap.get(0x952) in marked])
  result[rec.ScriptTag] = entries
 return result

def shape(font, binary, sample, features=None, accent_glyph=None):
 buf = hb.Buffer(); buf.add_str(sample); buf.guess_segment_properties()
 hb.shape(hb.Font(hb.Face(binary)),buf,features or {})
 gs = font.getGlyphSet(); order = font.getGlyphOrder(); cmap = font.getBestCmap()
 classes = font["GDEF"].table.GlyphClassDef.classDefs if "GDEF" in font else {}
 entries = []; x = y = 0
 for info,pos in zip(buf.glyph_infos,buf.glyph_positions):
  name = order[info.codepoint]
  pen = BoundsPen(gs); gs[name].draw(pen)
  rect = None
  if pen.bounds:
   x0,y0,x1,y1 = pen.bounds
   rect = [round(x0+x+pos.x_offset,1),round(y0+y+pos.y_offset,1),
           round(x1+x+pos.x_offset,1),round(y1+y+pos.y_offset,1)]
  entries.append({"glyph":name,"class":classes.get(name,0),"cluster":info.cluster,
                  "offset":[pos.x_offset,pos.y_offset],"advance":[pos.x_advance,pos.y_advance],
                  "rect":rect})
  x += pos.x_advance; y += pos.y_advance
 accent = accent_glyph or cmap.get(ord(sample[-1]))
 forms = accent_glyph_names(font, ord(sample[-1])) if sample and ord(sample[-1]) in TARGET_GAP and ord(sample[-1]) in cmap and not accent_glyph else {accent}
 mark = next((g for g in reversed(entries) if g["glyph"] in forms),None)
 gap = None
 if mark and mark["rect"]:
  others = [g["rect"] for g in entries if g is not mark and g["rect"]]
  if others:
   if sample.endswith(("॑", "᳚")): gap = round(mark["rect"][1]-max(b[3] for b in others),1)
   elif sample.endswith("॒"): gap = round(min(b[1] for b in others)-mark["rect"][3],1)
 return {"text":sample,"missing":any(g["glyph"]==".notdef" for g in entries),
         "dotted":any(g["glyph"]==cmap.get(0x25CC) for g in entries),
         "gap":gap,"glyphs":entries}

def native(family):
 path = ROOT/"out/demo-font-sources"/(family+"-Regular.ttf")
 path.parent.mkdir(parents=True,exist_ok=True)
 if not path.exists():
  with urlopen(f"{BASE_URL}/hinted/ttf/{family}/{path.name}",timeout=60) as resp:
   path.write_bytes(resp.read())
 if hashlib.sha256(path.read_bytes()).hexdigest()!=SOURCES[family]:
  raise ValueError("Source checksum mismatch: "+family)
 return TTFont(path)

def main():
 combined = TTFont(ROOT/"fonts/inditrans-indic-fallback.woff2")
 cb = font_bytes(combined)
 data = {"combined_metrics":metrics(combined),"scripts":{}}
 print("COMBINED METRICS",json.dumps(metrics(combined)))
 for script,cases in SAMPLES.items():
  source = native("NotoSerif"+script); nb = font_bytes(source)
  report = {"native_metrics":metrics(source),
            "native_shared_coverage":[hex(cp) for cp in (0x951,0x952) if cp in source.getBestCmap()],
            "gpos_combined":layout(combined,script),"gpos_native":layout(source,script),
            "cases":[]}
  print("SCRIPT",script,"native",json.dumps(report["native_metrics"]),"coverage",report["native_shared_coverage"])
  glyphs = {}
  for char in ("॑","॒"):
   trial = cases[0][0]+char
   glyphs[char] = (shape(combined,cb,trial)["glyphs"][-1]["glyph"],
                   shape(source,nb,trial)["glyphs"][-1]["glyph"])
  for sample in cases:
   char = sample[-1]
   c = shape(combined,cb,sample,accent_glyph=glyphs[char][0]); n = shape(source,nb,sample,accent_glyph=glyphs[char][1])
   report["cases"].append({"combined":c,"native":n})
   print(" CASE",repr(sample),"gap merged/native",c["gap"],n["gap"],
         "missing",c["missing"],n["missing"],"dotted",c["dotted"],n["dotted"],
         "glyphs",[(g["glyph"],g["offset"][1],g["rect"]) for g in c["glyphs"]])
  print(" GPOS",json.dumps(report["gpos_combined"]))
  for label, face, binary in (("merged",combined,cb),("native",source,nb)):
   tags = {}
   if "GPOS" in face:
    table = face["GPOS"].table
    for rec in table.ScriptList.ScriptRecord:
     if rec.ScriptTag not in SCRIPT_TAGS[script] or not rec.Script.DefaultLangSys: continue
     for index in rec.Script.DefaultLangSys.FeatureIndex:
      f = table.FeatureList.FeatureRecord[index]
      tags.setdefault(f.FeatureTag, []).extend(f.Feature.LookupListIndex)
   variants = {}
   for char in ("॑","॒"):
    sample = cases[0][0]+char
    nofeatures = shape(face,binary,sample,{tag:False for tag in tags})
    selective = {tag:shape(face,binary,sample,{tag:False}) for tag in tags}
    variants[char] = {"default":shape(face,binary,sample),
                       "no_gpos":nofeatures, "disabled_one":selective}
   print(" FEATURES",script,label,json.dumps({k:v for k,v in tags.items()}))
   for tag, indices in tags.items():
    if tag not in ("abvm","blwm","mark","mkmk"): continue
    for index in sorted(set(indices)):
     lookup=table.LookupList.Lookup[index]
     for sub in lookup.SubTable:
      sub=getattr(sub,"ExtSubTable",sub)
      cov=getattr(getattr(sub,"Coverage",None),"glyphs",[])
      if not cov: cov=getattr(getattr(sub,"MarkCoverage",None),"glyphs",[])
      sample_glyphs = [g["glyph"] for g in shape(face,binary,cases[0][0]+"॑")["glyphs"][-1:]]
      sample_glyphs += [g["glyph"] for g in shape(face,binary,cases[0][0]+"॒")["glyphs"][-1:]]
      print(" LOOKUP",script,label,tag,index,"type",lookup.LookupType,
            "subclass",type(sub).__name__,"coverage",len(cov),
            "mark_hits",[(g,cov.index(g)) for g in sample_glyphs if g in cov],
            "mapping_hits",[(g,str(getattr(sub,"mapping",{}).get(g))) for g in sample_glyphs if g in getattr(sub,"mapping",{})])
   for char,row in variants.items():
    base=row["default"]["glyphs"]
    markoffset=lambda result: [(g["glyph"],g["offset"]) for g in result["glyphs"] if g["class"]==3]
    print(" EFFECT",script,label,repr(char),"default",markoffset(row["default"]),
          "none",markoffset(row["no_gpos"]),"by_tag",
          {tag:markoffset(value) for tag,value in row["disabled_one"].items()})
  data["scripts"][script]=report
 dest = ROOT/"out/vedic-font-measurements.json"
 dest.parent.mkdir(parents=True,exist_ok=True)
 dest.write_text(json.dumps(data,indent=2,ensure_ascii=False),encoding="utf8")
 print("Wrote",dest)

if __name__=="__main__":
 main()
