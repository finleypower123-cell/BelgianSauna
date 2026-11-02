// Compiles the original, unmodified Airwindows Tube2 source (MIT licence, Chris Johnson).
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
#if defined(__wasm__)
 #include <set>
 #include <string>
 #include <math.h>
 #define throw std::abort()   // the browser build has no exceptions; Airwindows only throws on impossible parameter indices
#endif
#define createEffectInstance createEffectInstance_Tube2
#include "Tube2/Tube2.h"
#include "Tube2/Tube2.cpp"
#include "Tube2/Tube2Proc.cpp"
#include "AirwindowsUnits.h"
std::unique_ptr<AudioEffectX> aw::makeTube2() { return std::unique_ptr<AudioEffectX> (new Tube2 (nullptr)); }
