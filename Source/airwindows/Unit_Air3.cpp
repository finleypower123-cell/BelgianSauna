// Compiles the original, unmodified Airwindows Air3 source (MIT licence, Chris Johnson).
#if defined(__GNUC__) || defined(__clang__)
 #pragma GCC diagnostic ignored "-Wall"
 #pragma GCC diagnostic ignored "-Wextra"
 #pragma GCC diagnostic ignored "-Wconversion"
 #pragma GCC diagnostic ignored "-Wshadow"
 #pragma GCC diagnostic ignored "-Wfloat-equal"
 #pragma GCC diagnostic ignored "-Wsign-conversion"
 #pragma GCC diagnostic ignored "-Wdouble-promotion"
 #pragma GCC diagnostic ignored "-Wunused-parameter"
 #pragma GCC diagnostic ignored "-Wold-style-cast"
 #pragma GCC diagnostic ignored "-Wmultichar"
#elif defined(_MSC_VER)
 #pragma warning(push, 0)
#endif
#include "audioeffectx.h"
#define createEffectInstance createEffectInstance_Air3
#include "Air3/Air3.h"
#include "Air3/Air3.cpp"
#include "Air3/Air3Proc.cpp"
#include "AirwindowsUnits.h"
std::unique_ptr<AudioEffectX> aw::makeAir3() { return std::unique_ptr<AudioEffectX> (new Air3 (nullptr)); }
