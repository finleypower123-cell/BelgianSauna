// Prints the knob, section and preset tables as JSON for the browser version, so both share one source.
#include "SaunaModel.h"
#include <cstdio>
static void js (const char* s) { std::putchar ('"'); for (; *s; ++s) { if (*s == '"' || *s == '\\') std::putchar ('\\'); std::putchar (*s); } std::putchar ('"'); }
int main()
{
    std::printf ("{\"knobs\":[");
    for (int i = 0; i < sauna::NUM_KNOBS; ++i)
    {
        const auto& k = sauna::KNOBS[i];
        std::printf ("%s{\"id\":", i ? "," : ""); js (k.id);
        std::printf (",\"unit\":%d,\"joke\":", k.unit); js (k.joke); std::printf (",\"what\":"); js (k.what);
        std::printf (",\"def\":%g,\"fmt\":%d,\"steamTarget\":%g,\"steamIsWet\":%s}", k.def, k.fmt, k.steamTarget, k.steamIsWet ? "true" : "false");
    }
    std::printf ("],\"sections\":[");
    for (int s = 0; s < sauna::NUM_SECTIONS; ++s)
    {
        const auto& d = sauna::SECTIONS[s];
        std::printf ("%s{\"id\":", s ? "," : ""); js (d.onId); std::printf (",\"name\":"); js (d.name); std::printf (",\"what\":"); js (d.what);
        std::printf (",\"first\":%d,\"count\":%d}", d.firstKnob, d.numKnobs);
    }
    std::printf ("],\"presets\":[");
    for (int p = 0; p < sauna::NUM_PRESETS; ++p)
    {
        const auto v = sauna::presetValues (p);
        std::printf ("%s{\"name\":", p ? "," : ""); js (sauna::PRESETS[p].name); std::printf (",\"joke\":"); js (sauna::PRESETS[p].joke);
        std::printf (",\"steam\":%g,\"v\":[", sauna::PRESETS[p].steam);
        for (int i = 0; i < sauna::NUM_KNOBS; ++i) std::printf ("%s%g", i ? "," : "", v.v[i]);
        std::printf ("]}");
    }
    std::printf ("]}\n");
}
