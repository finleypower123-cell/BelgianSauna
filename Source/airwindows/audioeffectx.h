// Minimal stand-in for the parts of Steinberg's old VST2 AudioEffectX class that the
// Airwindows effects use, so their original source compiles unchanged inside a JUCE plug-in.
// Written for Belgian Sauna; contains no Steinberg code.
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>

typedef int32_t VstInt32;
typedef intptr_t (*audioMasterCallback) (void*, int32_t, int32_t, intptr_t, void*, float);

enum VstPlugCategory { kPlugCategUnknown = 0, kPlugCategEffect, kPlugCategSynth };
enum { kVstMaxProgNameLen = 24, kVstMaxParamStrLen = 8, kVstMaxVendorStrLen = 64,
       kVstMaxProductStrLen = 64, kVstMaxEffectNameLen = 32 };

#ifndef __audioeffect__
#define __audioeffect__
#endif

inline char* vst_strncpy (char* dst, const char* src, std::size_t maxLen)
{
    std::strncpy (dst, src, maxLen); dst[maxLen] = 0; return dst;
}
inline void float2string (float v, char* text, std::size_t maxLen) { std::snprintf (text, maxLen + 1, "%.3f", v); }
inline void int2string (VstInt32 v, char* text, std::size_t maxLen) { std::snprintf (text, maxLen + 1, "%d", (int) v); }
inline void dB2string (float v, char* text, std::size_t maxLen)
{
    if (v <= 0) vst_strncpy (text, "-inf", maxLen);
    else std::snprintf (text, maxLen + 1, "%.2f", 20.0 * std::log10 (v));
}

class AudioEffect
{
public:
    AudioEffect (audioMasterCallback, VstInt32, VstInt32) {}
    virtual ~AudioEffect() = default;
    virtual void processReplacing (float**, float**, VstInt32) {}
    virtual void processDoubleReplacing (double**, double**, VstInt32) {}
    virtual float getParameter (VstInt32) { return 0; }
    virtual void setParameter (VstInt32, float) {}
    virtual void getParameterName (VstInt32, char* t) { t[0] = 0; }
    virtual void getParameterDisplay (VstInt32, char* t) { t[0] = 0; }
    virtual void getParameterLabel (VstInt32, char* t) { t[0] = 0; }
    void setSampleRate (double sr) { sr_ = sr; }
    float getSampleRate() const { return (float) sr_; }
    void setNumInputs (VstInt32) {}
    void setNumOutputs (VstInt32) {}
    void setUniqueID (VstInt32) {}
    void canProcessReplacing (bool = true) {}
    void canDoubleReplacing (bool = true) {}
    void programsAreChunks (bool = true) {}
protected:
    double sr_ = 44100.0;
};

class AudioEffectX : public AudioEffect
{
public:
    using AudioEffect::AudioEffect;
    virtual bool getEffectName (char*) { return false; }
    virtual VstPlugCategory getPlugCategory() { return kPlugCategEffect; }
    virtual bool getProductString (char*) { return false; }
    virtual bool getVendorString (char*) { return false; }
    virtual VstInt32 getVendorVersion() { return 1000; }
    virtual void getProgramName (char* n) { n[0] = 0; }
    virtual void setProgramName (char*) {}
    virtual VstInt32 getChunk (void**, bool) { return 0; }
    virtual VstInt32 setChunk (void*, VstInt32, bool) { return 0; }
    virtual VstInt32 canDo (char*) { return 0; }
};
