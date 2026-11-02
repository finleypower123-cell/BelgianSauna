// Belgian Sauna — controls, presets and the STEAM macro.
// Every knob drives one parameter of an original Airwindows effect (0..1, exactly as Airwindows uses it).
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace sauna
{
enum Unit { TUBE2, PRESSURE4, AIR3, TOTAPE6, ENSEMBLE, TAPEDELAY2, GALACTIC, OUTPUT, NUM_UNITS };

// fmt: 0 = percent, 1 = +/-12 dB centred, 2 = voices, 3 = bipolar -1..+1, 4 = dB from gain (0..1 -> -inf..+6), 5 = delay time
struct KnobDef
{
    const char* id;
    int unit;
    int awParam;        // Airwindows parameter index (A = 0, B = 1 ...), -1 for our own controls
    const char* joke;
    const char* what;
    float def;
    int fmt;
    float steamTarget;  // where STEAM 100% pushes this knob; < 0 means STEAM leaves it alone
    bool steamIsWet;    // wet amounts fall to 0 as STEAM goes to 0 (the dry, un-steamed voice)
};

struct SectionDef { const char* onId; const char* name; const char* what; int unit; int firstKnob, numKnobs; };

static const KnobDef KNOBS[] = {
    // THE STOVE — Airwindows Tube2
    { "stoveInput",  TUBE2, 0, "Throw water on the stones", "Tube2 input",  0.85f, 0, -1.0f,  false },
    { "stoveTube",   TUBE2, 1, "Steam of consciousness",    "Tube2 tube",   0.45f, 0, 0.80f,  false },
    // TOWEL SNAP — Airwindows Pressure4
    { "towelPress",  PRESSURE4, 0, "It's called a sauna for a reason", "Pressure4 pressure", 0.35f, 0, 0.60f, false },
    { "towelSpeed",  PRESSURE4, 1, "Towel-snap speed",          "Pressure4 speed",    0.25f, 0, -1.0f, false },
    { "towelMew",    PRESSURE4, 2, "Cat in the changing room",  "Pressure4 mewiness", 0.50f, 3, -1.0f, false },
    { "towelOut",    PRESSURE4, 3, "Sweat it out",              "Pressure4 output",   1.00f, 0, -1.0f, false },
    // BIRCH WHISK — Airwindows Air3
    { "birchAir",    AIR3, 0, "Fresh air from Ostend",          "Air3 air",    0.62f, 0, 0.75f, false },
    { "birchGnd",    AIR3, 1, "Grounded (rarely)",              "Air3 ground", 0.50f, 0, -1.0f, false },
    // CASSETTE IN THE CHANGING ROOM — Airwindows ToTape6
    { "tapeIn",      TOTAPE6, 0, "Melted in the heat",          "ToTape6 input",   0.55f, 1, -1.0f, false },
    { "tapeSoften",  TOTAPE6, 1, "Soft focus",                  "ToTape6 soften",  0.55f, 0, 0.75f, false },
    { "tapeHead",    TOTAPE6, 2, "Head in the clouds",          "ToTape6 head bump", 0.40f, 0, -1.0f, false },
    { "tapeFlutter", TOTAPE6, 3, "Wobbly from the heat",        "ToTape6 flutter", 0.35f, 0, 0.70f, false },
    { "tapeOut",     TOTAPE6, 4, "Rewind",                      "ToTape6 output",  0.50f, 1, -1.0f, false },
    { "tapeMix",     TOTAPE6, 5, "Mixtape for my ex",           "ToTape6 dry/wet", 1.00f, 0, -1.0f, true  },
    // STEAM CHOIR — Airwindows Ensemble
    { "choirVoices", ENSEMBLE, 0, "Voices in the steam",        "Ensemble voices",   0.20f, 2, -1.0f, false },
    { "choirFull",   ENSEMBLE, 1, "Fuller than a waffle",       "Ensemble fullness", 0.60f, 0, -1.0f, false },
    { "choirBright", ENSEMBLE, 2, "Fog lights",                 "Ensemble brighten", 0.45f, 0, -1.0f, false },
    { "choirMix",    ENSEMBLE, 3, "Everyone's in here now",     "Ensemble dry/wet",  0.30f, 0, 0.55f, true  },
    // ECHO IN THE SHOWERS — Airwindows TapeDelay2
    { "echoTime",    TAPEDELAY2, 0, "Waiting for the rain to stop", "TapeDelay2 time",  0.45f, 5, -1.0f, false },
    { "echoRegen",   TAPEDELAY2, 1, "Repeats itself in two languages", "TapeDelay2 regen", 0.45f, 0, 0.65f, false },
    { "echoFreq",    TAPEDELAY2, 2, "Heard through the wall",   "TapeDelay2 freq",    0.40f, 0, -1.0f, false },
    { "echoReso",    TAPEDELAY2, 3, "Ringing tiles",            "TapeDelay2 reso",    0.15f, 0, -1.0f, false },
    { "echoFlutter", TAPEDELAY2, 4, "Pedalboard wobble",        "TapeDelay2 flutter", 0.35f, 0, 0.65f, false },
    { "echoMix",     TAPEDELAY2, 5, "Echoes from the cold plunge", "TapeDelay2 dry/wet", 0.22f, 0, 0.40f, true },
    // COSMIC SAUNA — Airwindows Galactic
    { "cosmosReplace", GALACTIC, 0, "Replace me with the room", "Galactic replace",    0.55f, 0, 0.75f, false },
    { "cosmosBright",  GALACTIC, 1, "Stained glass",            "Galactic brightness", 0.45f, 0, -1.0f, false },
    { "cosmosDetune",  GALACTIC, 2, "Out of tune on purpose",   "Galactic detune",     0.50f, 0, 0.75f, false },
    { "cosmosBig",     GALACTIC, 3, "Size of the sauna",        "Galactic bigness",    0.85f, 0, 1.00f, false },
    { "cosmosMix",     GALACTIC, 4, "Lost in the haze",         "Galactic level",      0.30f, 0, 0.60f, true  },
    // TOWEL OFF — ours
    { "outMix",      OUTPUT, -1, "Still fully clothed",         "Dry / wet",  1.00f, 0, -1.0f, false },
    { "outGain",     OUTPUT, -1, "Volume of the sigh",          "Output",     0.80f, 4, -1.0f, false },
};
static constexpr int NUM_KNOBS = (int) (sizeof (KNOBS) / sizeof (KNOBS[0]));

static const SectionDef SECTIONS[] = {
    { "stoveOn",  "The Stove",                     "Tube preamp (Airwindows Tube2)",        TUBE2,      0, 2 },
    { "towelOn",  "Towel Snap",                    "Compressor (Airwindows Pressure4)",     PRESSURE4,  2, 4 },
    { "birchOn",  "Birch Whisk",                   "Air EQ (Airwindows Air3)",              AIR3,       6, 2 },
    { "tapeOn",   "Cassette in the Changing Room", "Tape (Airwindows ToTape6)",             TOTAPE6,    8, 6 },
    { "choirOn",  "Steam Choir",                   "Ensemble (Airwindows Ensemble)",        ENSEMBLE,  14, 4 },
    { "echoOn",   "Echo in the Showers",           "Tape delay (Airwindows TapeDelay2)",    TAPEDELAY2, 18, 6 },
    { "cosmosOn", "Cosmic Sauna",                  "Reverb (Airwindows Galactic)",          GALACTIC,  24, 5 },
    { "outOn",    "Towel Off",                     "Mix and output",                        OUTPUT,    29, 2 },
};
static constexpr int NUM_SECTIONS = (int) (sizeof (SECTIONS) / sizeof (SECTIONS[0]));

struct Values { float v[NUM_KNOBS]; bool on[NUM_SECTIONS]; };

inline Values defaults()
{
    Values x {};
    for (int i = 0; i < NUM_KNOBS; ++i) x.v[i] = KNOBS[i].def;
    for (int s = 0; s < NUM_SECTIONS; ++s) x.on[s] = true;
    return x;
}

inline int knobIndex (const char* id)
{
    for (int i = 0; i < NUM_KNOBS; ++i) if (std::string (KNOBS[i].id) == id) return i;
    return -1;
}

struct PresetValue { const char* id; float value; };
struct PresetDef { const char* name; const char* joke; float steam; PresetValue values[16]; };

static const PresetDef PRESETS[] = {
    { "Belgian Sauna", "The signature. Warm stove, wet towel, a choir in the fog.", 0.5f, { { nullptr, 0 } } },
    { "Heaven or Ostend", "Breathy, bright, floating three feet above the North Sea.", 0.55f, {
        { "stoveTube", 0.35f }, { "birchAir", 0.75f }, { "tapeSoften", 0.4f }, { "choirVoices", 0.3f }, { "choirMix", 0.4f },
        { "choirBright", 0.6f }, { "echoTime", 0.55f }, { "echoMix", 0.25f }, { "cosmosBright", 0.6f }, { "cosmosMix", 0.38f }, { nullptr, 0 } } },
    { "Loveless in Leuven", "A wall of steam. You can't hear the words. That's the point.", 0.6f, {
        { "stoveTube", 0.7f }, { "towelPress", 0.55f }, { "tapeFlutter", 0.6f }, { "tapeSoften", 0.75f }, { "choirVoices", 0.5f },
        { "choirMix", 0.5f }, { "echoRegen", 0.6f }, { "echoFlutter", 0.6f }, { "cosmosReplace", 0.75f }, { "cosmosDetune", 0.8f },
        { "cosmosBig", 1.0f }, { "cosmosMix", 0.5f }, { nullptr, 0 } } },
    { "Fog Over Ghent", "Dark, soft and far away. Bring a scarf.", 0.5f, {
        { "birchAir", 0.45f }, { "tapeSoften", 0.85f }, { "tapeHead", 0.6f }, { "choirBright", 0.25f }, { "echoFreq", 0.25f },
        { "echoMix", 0.28f }, { "cosmosBright", 0.25f }, { "cosmosMix", 0.42f }, { nullptr, 0 } } },
    { "Whisper in the Changing Room", "Close, intimate, a little too close.", 0.4f, {
        { "stoveTube", 0.3f }, { "towelPress", 0.5f }, { "towelSpeed", 0.4f }, { "birchAir", 0.72f }, { "choirMix", 0.15f },
        { "echoTime", 0.3f }, { "echoMix", 0.12f }, { "cosmosBig", 0.6f }, { "cosmosMix", 0.18f }, { nullptr, 0 } } },
    { "Tape from the Attic", "Found it behind the sauna. Still plays. Mostly.", 0.5f, {
        { "tapeIn", 0.65f }, { "tapeSoften", 0.7f }, { "tapeHead", 0.65f }, { "tapeFlutter", 0.7f }, { "choirMix", 0.2f },
        { "echoFlutter", 0.75f }, { "echoFreq", 0.3f }, { "echoReso", 0.3f }, { "cosmosMix", 0.22f }, { nullptr, 0 } } },
    { "Antwerp Cathedral", "Enormous. Every note lasts until Tuesday.", 0.55f, {
        { "choirMix", 0.25f }, { "echoMix", 0.15f }, { "cosmosReplace", 0.85f }, { "cosmosBig", 1.0f }, { "cosmosBright", 0.55f },
        { "cosmosMix", 0.5f }, { nullptr, 0 } } },
    { "Dry Towel", "Just the preamp. For when the band says 'less reverb, Fin'.", 0.5f, {
        { "stoveTube", 0.5f }, { "towelPress", 0.45f }, { "birchAir", 0.65f }, { "tapeSoften", 0.4f }, { "tapeFlutter", 0.15f },
        { "choirMix", 0.0f }, { "echoMix", 0.0f }, { "cosmosMix", 0.0f }, { nullptr, 0 } } },
};
static constexpr int NUM_PRESETS = (int) (sizeof (PRESETS) / sizeof (PRESETS[0]));

inline Values presetValues (int index)
{
    Values x = defaults();
    for (const auto& pv : PRESETS[index].values)
    {
        if (pv.id == nullptr) break;
        const int k = knobIndex (pv.id);
        if (k >= 0) x.v[k] = pv.value;
    }
    return x;
}

// STEAM: 0 = dry room (all wet sends off, colour stays), 0.5 = knobs exactly as set, 1 = full steam.
inline Values applySteam (const Values& user, float steam)
{
    Values e = user;
    const bool lo = steam <= 0.5f;
    const float t = lo ? steam / 0.5f : (steam - 0.5f) / 0.5f, tp = std::pow (t, 1.3f);
    for (int i = 0; i < NUM_KNOBS; ++i)
    {
        const auto& k = KNOBS[i];
        if (k.steamTarget < 0) continue;
        const float v = user.v[i];
        if (lo) e.v[i] = k.steamIsWet ? v * t : v;
        else    e.v[i] = v + std::max (0.0f, k.steamTarget - v) * tp;
    }
    // Steam adds a lot of wet energy; pull the output down as it rises so louder never wins the A/B.
    if (! lo) { const int g = knobIndex ("outGain"); e.v[g] = user.v[g] * (float) std::pow (10.0, -5.0 * t / 20.0); }
    return e;
}

inline std::string formatValue (const KnobDef& k, float v)
{
    char b[32];
    switch (k.fmt)
    {
        case 1: std::snprintf (b, sizeof b, "%+.1f dB", (v - 0.5) * 24.0); break;
        case 2: std::snprintf (b, sizeof b, "%d voices", (int) std::floor (v * 46.0 + 2.9)); break;
        case 3: std::snprintf (b, sizeof b, "%+.2f", v * 2.0 - 1.0); break;
        case 4:
        {
            const double g = (double) v * 1.25;   // 0.8 = unity, 1.0 = +1.9 dB
            if (g < 1e-4) return "-inf dB";
            std::snprintf (b, sizeof b, "%+.1f dB", 20.0 * std::log10 (g)); break;
        }
        case 5: std::snprintf (b, sizeof b, "%d%% tape", (int) std::lround (v * 100)); break;
        default: std::snprintf (b, sizeof b, "%d%%", (int) std::lround (v * 100)); break;
    }
    return b;
}

inline const char* steamStage (float v)
{
    if (v < 0.12f) return "DRY ROOM";
    if (v < 0.37f) return "WARMING UP";
    if (v < 0.62f) return "PROPER SAUNA";
    if (v < 0.87f) return "CAN'T SEE";
    return "PURE STEAM";
}
} // namespace sauna
