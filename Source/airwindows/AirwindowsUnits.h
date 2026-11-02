// One factory per Airwindows effect. Each effect is compiled in its own file so their
// global names (kParamA, kNumParameters...) never clash.
#pragma once
#include "audioeffectx.h"
#include <memory>

namespace aw
{
std::unique_ptr<AudioEffectX> makeTube2();
std::unique_ptr<AudioEffectX> makePressure4();
std::unique_ptr<AudioEffectX> makeAir3();
std::unique_ptr<AudioEffectX> makeToTape6();
std::unique_ptr<AudioEffectX> makeEnsemble();
std::unique_ptr<AudioEffectX> makeTapeDelay2();
std::unique_ptr<AudioEffectX> makeGalactic();
}
