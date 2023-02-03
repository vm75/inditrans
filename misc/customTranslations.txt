function customTranslations (text, to) {
    // devanagari - https://github.com/Sandhi-IITBombay/Shobhika/issues/9#issuecomment-491210185
    if (to == "devanagari") {
        //  श and not श्र in "s.r " (sru in skt) ;
        text = text.replace(/शृ/g,"श‍ृ");
    } else if (to == "bengali") {
        // bengali ya - change virama plus য় to virama plus য
        text = text.replace(/্য়/g,"্য");
    } else if (to == "kannada") {
        // kannada - change panchama varna to anusvar, except n
        // ~n, ~N, N to M ;
        text = text.replace(/ಙ್(ಕ|ಖ|ಗ|ಘ)/g,"ಂ$1");
        text = text.replace(/ಞ್(ಚ|ಛ|ಜ|ಝ)/g,"ಂ$1");
        text = text.replace(/ಣ್(ಟ|ಠ|ಡ|ಢ)/g,"ಂ$1");
        text = text.replace(/ನ್(ತ|ಥ|ದ|ಧ)/g,"ಂ$1");
        text = text.replace(/ಮ್(ಪ|ಫ|ಬ|ಭ)/g,"ಂ$1");
    } else if (to == "telugu") {
        text = text.replace(/ఙ్(క|ఖ|గ|ఘ)/g,"ಂ$1");
        text = text.replace(/ఞ్(చ|ఛ|జ|ఝ)/g,"ಂ$1");
        text = text.replace(/ణ్(ట|ఠ|డ|ఢ)/g,"ಂ$1");
        text = text.replace(/న్(త|థ|ద|ధ)/g,"ಂ$1");
        text = text.replace(/మ్(ప|ఫ|బ|భ)/g,"ಂ$1");
    } else if (to == "malayalam") {
        // Enable Malayalam Chillu Support - code to be streamlined after rules are defined correctly
        // m to M ;
        text = text.replace(/മ്/g,"ം");
        // use ZWJ to create chillus for N, n, r, l, L
        // text = text.replace(/(ണ്|ന്|ര്|ല്|ള്)/g,"$1\u200D")
        // re-change to glyph when followed by p or m,
        text = text.replace(/ംമ/g,"മ്മ");
        text = text.replace(/ംപ/g,"മ്പ");
        // or when following t as in gm tm  nm mm Nm
        text = text.replace(/ഗ്ം/g,"ഗ്മ്");
        text = text.replace(/ത്ം/g,"ത്മ്");
        text = text.replace(/ൻം/g,"ന്മ്");
        text = text.replace(/ംം/g,"മ്മ്");
        text = text.replace(/ൺം/g,"ണ്മ്");
        // fix NTa-NNa
        text = text.replace(/ണ്‍ട/g,"ണ്ട");
        text = text.replace(/ണ്‍ഠ/g,"ണ്ഠ");
        text = text.replace(/ണ്‍ഡ/g,"ണ്ഡ");
        text = text.replace(/ണ്‍ഢ/g,"ണ്ഢ");
        text = text.replace(/ണ്‍ണ/g,"ണ്ണ");
        // fix nta-nna
        text = text.replace(/ന്‍ത/g,"ന്ത");
        text = text.replace(/ന്‍ഥ/g,"ന്ഥ");
        text = text.replace(/ന്‍ദ/g,"ന്ദ");
        text = text.replace(/ന്‍ധ/g,"ന്ധ");
        text = text.replace(/ന്‍ന/g,"ന്ന");
        // remove ZWJ when followed by ya la va
        text = text.replace(/\u200D(യ|വ|ല)/g,"$1");
        // fix for ര്‍വ.  -  r^va
        text = text.replace(/ര്വ/g,"ര്‍വ");
        // chillu k not used much
        // text = text.replace(/ക്/g,"ക്‍")
        // glyph not defined for this yet
        // text = text.replace(/ൻ്റ/g,"ൻ്റ) ")
    } else if (to == "tamil_print") {
        // Enable Tamil Accents Support
        text = text
            .replace(/(.)(²|³|⁴)(ா|ி|ீ|ு|ூ|ெ|ே|ை|ொ|ோ|ௌ)/g,"$1$3$2")
            .replace(/(.)(²|³|⁴)(்|:|ʼ|॒|॑|᳚)/g,"$1$3$2")
            .replace(/(.)(²|³|⁴|ʼ)(॒|॑|᳚)/g,"$1$3$2")
            .replace(/(.)(:)(॒|॑)/g,"$1$3$2");
        text = text
            .replace(/(த(்|ா|ி|ீ|ு|ூ|ெ|ே|ை|ொ|ோ|ௌ)?(²|³|⁴)?)ன/g, "$1ந")
            .replace(/ன((்|ா|ி|ீ|ு|ூ|ெ|ே|ை|ொ|ோ|ௌ)?(²|³|⁴)?)த/g, "ந$1த")
            .replace(/(^|\s)ன/g,"$1ந")
            .replace(/பழனி/g,"பழநி")
            .replace(/யுக³ல/g,"யுக³ள")
            .replace(/துலஸீ/g,"துளஸீ")
            .replace(/தும்ப³ர-தால/g,"தும்ப³ர-தாள")
            .replace(/ராக³தால/g,"ராக³தாள")
            .replace(/மங்க³ல/g,"மங்க³ள")
            .replace(/மம்க³ல/g,"மங்க³ள")
            .replace(/த⁴வல/g,"த⁴வள")
            .replace(/மஞ்ஜுல/g,"மஞ்ஜுள")
            .replace(/கீர்தநாவல/g,"கீர்தநாவள")
            .replace(/முக்தாவல/g,"முக்தாவள")
            .replace(/நாமாவல/g,"நாமாவள")
            .replace(/புஷ்பாவல/g,"புஷ்பாவள")
            .replace(/ரத்நாவல/g,"ரத்நாவள")
            .replace(/ஸ்தோத்ராவல/g,"ஸ்தோத்ராவள")
            .replace(/சரணயுக³லாய/g,"சரணயுக³ளாய")
            .replace(/ப⁴க்திபரிமலாய/g,"ப⁴க்திபரிமளாய")
            .replace(/பத³யுக³லாய/g,"பத³யுக³ளளாய")
            .replace(/துலஸீத³ல/g,"துலஸீத³ள")
            .replace(/ப⁴க்திபரிமலித/g,"ப⁴க்திபரிமளித")
            .replace(/மஞ்ஜுலதம/g,"மஞ்ஜுளதம")
            .replace(/சரணயுக³ல/g,"சரணயுக³ள")
            .replace(/சரணயுக³லத்³ருʼட⁴/g,"சரணயுக³ளத்³ருʼட⁴")
            .replace(/க்ஷராவலி/g,"க்ஷராவளி")
            .replace(/கிரணாவலி/g,"கிரணாவளி");
        text = text
            .replace(/மங்க³ள்ய/g,"மங்க³ல்ய")
            .replace(/ம்க/g,"ங்க")
            .replace(/ம்ச/g,"ஞ்ச")
            .replace(/ம்ஜ/g,"ஞ்ஜ")
            .replace(/ஞ்ஜ்ஞ/g,"ம்ஜ்ஞ")
            .replace(/ம்த/g,"ந்த")
            .replace(/ம்ட/g,"ண்ட");
    } else if (to == "grantamil") {
        text = text
            .replace(/(খ|গ|ঘ|ছ|ঝ|ঠ|ড|ঢ|থ|দ|ধ|ফ|ব|ভ|ৰ)(ே)/g, "◌ே$1") //Add invisible \u25CC for it to work
            .replace(/(খ|গ|ঘ|ছ|ঝ|ঠ|ড|ঢ|থ|দ|ধ|ফ|ব|ভ|ৰ)(ோ)/g, "◌ே$1া") //Add invisible \u25CC for it to work
            .replace(/(খ|গ|ঘ|ছ|ঝ|ঠ|ড|ঢ|থ|দ|ধ|ফ|ব|ভ|ৰ)(ை)/g, "◌ை$1") //Add invisible ◌ at beginning
            .replace(/(খ|গ|ঘ|ছ|ঝ|ঠ|ড|ঢ|থ|দ|ধ|ফ|ব|ভ|ৰ)(ௌ)/g, "◌ெ$1ள"); //Add invisible ◌ at beginning
        text = text
            .replace(/ம்க/g,"ங்க")
            .replace(/ம்ச/g,"ஞ்ச")
            .replace(/ம்ஜ/g,"ஞ்ஜ")
            .replace(/ம்த/g,"ந்த")
            .replace(/ம்ட/g,"ண்ட")
            .replace(/ராগதால/g,"ராগதாள")
            .replace(/நாமாவல/g,"நாமாவள")
            .replace(/புஷ்பாவல/g,"புஷ்பாவள")
            .replace(/மங்গல/g,"மங்গள")
            .replace(/மம்গல/g,"மங்গள");
    }
}

(function () { customTranslations("", "", ""); })();
