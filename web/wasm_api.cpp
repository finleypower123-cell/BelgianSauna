// WebAssembly entry points for the browser version of Belgian Sauna. Same engine as the plug-in.
#include "SaunaEngine.h"

static sauna::Engine* E = nullptr;
static sauna::Values U = sauna::defaults();
static float steam = 0.5f;
static float bufL[4096], bufR[4096];
static float meters[3];

extern "C" {
__attribute__((export_name("sauna_init"))) void sauna_init (double sr)
{
    delete E; E = new sauna::Engine(); E->prepare (sr, 4096); E->setValues (sauna::applySteam (U, steam));
}
__attribute__((export_name("sauna_set"))) void sauna_set (int knob, float v) { if (knob >= 0 && knob < sauna::NUM_KNOBS) U.v[knob] = v; }
__attribute__((export_name("sauna_set_on"))) void sauna_set_on (int s, int on) { if (s >= 0 && s < sauna::NUM_SECTIONS) U.on[s] = on != 0; }
__attribute__((export_name("sauna_set_steam"))) void sauna_set_steam (float v) { steam = v; }
__attribute__((export_name("sauna_commit"))) void sauna_commit() { if (E) E->setValues (sauna::applySteam (U, steam)); }
__attribute__((export_name("sauna_set_bypass"))) void sauna_set_bypass (int b) { if (E) E->setBypassed (b != 0); }
__attribute__((export_name("sauna_buf_l"))) float* sauna_buf_l() { return bufL; }
__attribute__((export_name("sauna_buf_r"))) float* sauna_buf_r() { return bufR; }
__attribute__((export_name("sauna_meters"))) float* sauna_meters() { return meters; }
__attribute__((export_name("sauna_process"))) void sauna_process (int n)
{
    if (! E || n <= 0 || n > 4096) return;
    E->process (bufL, bufR, bufL, bufR, n);
    const auto m = E->takeMeters(); meters[0] = m.in; meters[1] = m.out; meters[2] = m.gr;
}
}
