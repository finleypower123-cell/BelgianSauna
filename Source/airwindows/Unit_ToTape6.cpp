// Compiles the original, unmodified Airwindows ToTape6 source (MIT licence, Chris Johnson).
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
#define createEffectInstance createEffectInstance_ToTape6
#include "ToTape6/ToTape6.h"
#include "ToTape6/ToTape6.cpp"
#include "ToTape6/ToTape6Proc.cpp"
#include "AirwindowsUnits.h"
std::unique_ptr<AudioEffectX> aw::makeToTape6() { return std::unique_ptr<AudioEffectX> (new ToTape6 (nullptr)); }
