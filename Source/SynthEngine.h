#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include "Saturation.h"

namespace kbass {
constexpr double pi = 3.14159265358979323846;
struct Adsr { float attack=.005f, decay=.25f, sustain=.75f, release=.16f; };
struct Parameters {
    int wave1=0, wave2=2, octave=1, lfoWave=1;
    float detune=4, glide=.06f, pitchLfo=0, mix=.35f;
    Adsr amp{}, filter{.002f,.22f,.05f,.15f};
    float lfoSpeed=4, cutoff=650, resonance=.22f, filterEnv=3, filterLfo=0, volume=.7f;
    float saturation=0;
    int saturationType=0;
};

class Envelope {
public:
    enum Stage { idle, attack, decay, sustain, release };
    void reset() noexcept { level=0; stage=idle; }
    void on() noexcept { stage=attack; }
    void off(float seconds, double rate) noexcept {
        if (stage==idle || stage==release) return;
        releaseStep=level/std::max(1.0,seconds*rate); stage=release;
    }
    float next(const Adsr& p,double rate) noexcept {
        switch(stage) {
            case attack: level+=float(1.0/std::max(1.0,p.attack*rate));
                if(level>=1) { level=1; stage=decay; } break;
            case decay: level-=float((1-p.sustain)/std::max(1.0,p.decay*rate));
                if(level<=p.sustain) { level=p.sustain; stage=sustain; } break;
            case sustain: level=p.sustain; break;
            case release: level-=releaseStep; if(level<=0) reset(); break;
            case idle: break;
        }
        return level;
    }
    bool active() const noexcept { return stage!=idle; }
    bool releasing() const noexcept { return stage==release; }
private:
    Stage stage=idle;
    float level=0,releaseStep=0;
};

inline double polyBlep(double phase,double step) noexcept {
    if(step<=0) return 0;
    if(phase<step) { const auto t=phase/step; return t+t-t*t-1; }
    if(phase>1-step) { const auto t=(phase-1)/step; return t*t+t+t+1; }
    return 0;
}
inline double oscillator(int wave,double phase,double step) noexcept {
    if(wave==2) return std::sin(2*pi*phase);
    if(wave==1) {
        double shifted=phase+.5; if(shifted>=1) shifted-=1;
        return (phase<.5 ? 1.0 : -1.0)+polyBlep(phase,step)-polyBlep(shifted,step);
    }
    return 2*phase-1-polyBlep(phase,step);
}
inline float lfoWaveform(int wave,double phase) noexcept {
    if(wave==1) return float(1-4*std::abs(phase-.5));
    if(wave==2) return phase<.5 ? 1.0f : -1.0f;
    return float(2*phase-1);
}

// Two sample-rate-independent stages in octave space give continuous slopes
// across the full frequency range without blurring the envelope/LFO signal.
class CutoffSmoother {
public:
    void prepare(double rate,double hz) noexcept {
        coefficient=1-std::exp(-1/(.012*std::clamp(rate,8000.0,384000.0)));
        reset(hz);
    }
    void reset(double hz) noexcept { first=second=destination=std::log2(std::clamp(hz,20.0,18000.0)); }
    void setTarget(double hz) noexcept { destination=std::log2(std::clamp(hz,20.0,18000.0)); }
    double nextLog2() noexcept {
        first+=coefficient*(destination-first);
        second+=coefficient*(first-second);
        return second;
    }
    double currentHz() const noexcept { return std::exp2(second); }
private:
    double coefficient=.0018,first=9.3442959079,second=9.3442959079,destination=9.3442959079;
};

class SynthEngine {
public:
    void prepare(double rate) noexcept {
        sampleRate=std::clamp(rate,8000.0,384000.0);
        smoothing=float(1-std::exp(-1/(.012*sampleRate)));
        lfoSmoothing=float(1-std::exp(-1/(.002*sampleRate)));
        cutoffSmoother.prepare(sampleRate,target.cutoff);
        saturator.prepare(sampleRate,target.saturation,target.saturationType);
        reset();
    }
    void setParameters(const Parameters& p) noexcept {
        target=p; cutoffSmoother.setTarget(p.cutoff);
        saturator.setParameters(p.saturation,p.saturationType);
    }
    void reset() noexcept {
        for(auto& c:held) c.fill(Held{});
        pedals.fill(false); bends.fill(0); smoothedBends.fill(0); voice=Voice{}; order=0;
        smooth=target; dcIn=dcOut=0; lfoPhase=0; lfoValue=0;
        cutoffSmoother.reset(target.cutoff);
        saturator.reset(target.saturation,target.saturationType);
        for(int i=0;i<3;++i) {
            weights1[size_t(i)]=target.wave1==i ? 1.0f : 0.0f;
            weights2[size_t(i)]=target.wave2==i ? 1.0f : 0.0f;
        }
    }
    void noteOn(int channel,int note,float velocity) noexcept {
        if(!valid(channel,note)) return;
        if(velocity<=0) { noteOff(channel,note); return; }
        held[size_t(channel-1)][size_t(note)]={true,false,std::clamp(velocity,0.0f,1.0f),++order};
        start(channel,note,velocity,voice.amp.active() && !voice.amp.releasing());
    }
    void noteOff(int channel,int note) noexcept {
        if(!valid(channel,note)) return;
        auto& h=held[size_t(channel-1)][size_t(note)];
        h.down=false; h.sustained=pedals[size_t(channel-1)] && h.order!=0;
        if(voice.channel==channel && voice.note==note) selectLast();
    }
    void sustainPedal(int channel,bool down) noexcept {
        if(channel<1 || channel>16) return;
        pedals[size_t(channel-1)]=down;
        if(!down) {
            for(auto& h:held[size_t(channel-1)]) h.sustained=false;
            selectLast();
        }
    }
    void pitchWheel(int channel,int value) noexcept {
        if(channel>=1 && channel<=16)
            bends[size_t(channel-1)]=2.0f*float(std::clamp(value,0,16383)-8192)/8192.0f;
    }
    void allNotesOff(int channel,bool immediate) noexcept {
        if(channel<1 || channel>16) return;
        held[size_t(channel-1)].fill(Held{}); pedals[size_t(channel-1)]=false;
        if(voice.channel==channel && immediate) {
            voice=Voice{}; dcIn=dcOut=0; saturator.reset(target.saturation,target.saturationType);
        }
        selectLast();
    }
    float currentPitch() const noexcept { return voice.pitch; }
    bool active() const noexcept { return voice.amp.active(); }
    void render(float* left,float* right,int count) noexcept {
        const double internalRate=sampleRate*2;
        const float dcPole=float(std::exp(-2*pi*5/sampleRate));
        for(int i=0;i<count;++i) {
            approach(smooth.mix,target.mix); approach(smooth.detune,target.detune);
            const double baseCutoffLog=cutoffSmoother.nextLog2();
            approach(smooth.resonance,target.resonance);
            approach(smooth.pitchLfo,target.pitchLfo); approach(smooth.filterLfo,target.filterLfo);
            approach(smooth.filterEnv,target.filterEnv); approach(smooth.volume,target.volume);
            approach(smooth.lfoSpeed,target.lfoSpeed);
            approach(smooth.glide,target.glide);
            smoothEnvelope(smooth.amp,target.amp); smoothEnvelope(smooth.filter,target.filter);
            for(int w=0;w<3;++w) {
                approach(weights1[size_t(w)],target.wave1==w ? 1.0f : 0.0f);
                approach(weights2[size_t(w)],target.wave2==w ? 1.0f : 0.0f);
            }
            lfoValue+=lfoSmoothing*(lfoWaveform(target.lfoWave,lfoPhase)-lfoValue);
            advance(lfoPhase,smooth.lfoSpeed/sampleRate);
            float sample=0;
            if(voice.amp.active()) {
                const float glide=smooth.glide>.0001f ? float(1-std::exp(-1/(smooth.glide*sampleRate))) : 1;
                voice.pitch+=(float(voice.note)-voice.pitch)*glide;
                approach(voice.velocity,voice.targetVelocity);
                approach(smoothedBends[size_t(voice.channel-1)],bends[size_t(voice.channel-1)]);
                const double pitch=voice.pitch+smoothedBends[size_t(voice.channel-1)]+smooth.pitchLfo*lfoValue;
                const double hz=440*std::exp2((pitch-69)/12);
                const double step1=std::min(.45,hz/internalRate);
                const double step2=std::min(.45,hz*std::exp2(-target.octave+smooth.detune/1200)/internalRate);
                const float amp=voice.amp.next(smooth.amp,sampleRate);
                const float envelope=voice.filter.next(smooth.filter,sampleRate);
                const double cutoff=std::clamp(std::exp2(baseCutoffLog+smooth.filterEnv*envelope+smooth.filterLfo*lfoValue),
                    20.0,std::min(18000.0,sampleRate*.4));
                const double g=std::tan(pi*cutoff/internalRate);
                const double resonance=2.0-1.85*std::clamp(double(smooth.resonance),0.0,1.0);
                for(int sub=0;sub<2;++sub) {
                    double first=0,second=0;
                    for(int w=0;w<3;++w) {
                        first+=weights1[size_t(w)]*oscillator(w,voice.phase1,step1);
                        second+=weights2[size_t(w)]*oscillator(w,voice.phase2,step2);
                    }
                    const double raw=first*(1-smooth.mix)+second*smooth.mix;
                    const double low=voice.pole1.process(raw,g,resonance);
                    sample+=float(.5*voice.pole2.process(low,g,1.41421356237));
                    advance(voice.phase1,step1); advance(voice.phase2,step2);
                }
                sample*=amp*voice.velocity;
                if(!voice.amp.active()) { voice.pole1={}; voice.pole2={}; }
            }
            const float blocked=sample-dcIn+dcPole*dcOut;
            dcIn=sample; dcOut=std::abs(blocked)<1e-20f ? 0 : blocked;
            // Preserve the original voicing before the new oversampled stage,
            // avoiding another nonlinear operation after decimation.
            const float coloured=saturator.process(float(std::tanh(blocked*.85)));
            const float out=coloured*smooth.volume;
            left[i]+=out; if(right) right[i]+=out;
        }
    }
private:
    struct Held { bool down=false,sustained=false; float velocity=0; uint64_t order=0; };
    struct Filter {
        double ic1=0,ic2=0;
        double process(double input,double g,double damping) noexcept {
            const double band=(ic1+g*(input-ic2))/(1+g*(g+damping));
            const double low=ic2+g*band;
            ic1=2*band-ic1; ic2=2*low-ic2;
            if(std::abs(ic1)<1e-25) ic1=0;
            if(std::abs(ic2)<1e-25) ic2=0;
            return low;
        }
    };
    struct Voice {
        int note=-1,channel=1; float pitch=0,velocity=0,targetVelocity=0;
        double phase1=0,phase2=.23; Envelope amp,filter; Filter pole1,pole2;
    };
    static bool valid(int channel,int note) noexcept { return channel>=1 && channel<=16 && note>=0 && note<128; }
    static void advance(double& phase,double step) noexcept { phase+=step; phase-=std::floor(phase); }
    void approach(float& value,float destination) const noexcept { value+=smoothing*(destination-value); }
    void smoothEnvelope(Adsr& value,const Adsr& destination) const noexcept {
        approach(value.attack,destination.attack); approach(value.decay,destination.decay);
        approach(value.sustain,destination.sustain); approach(value.release,destination.release);
    }
    void start(int channel,int note,float velocity,bool legato) noexcept {
        if(!voice.amp.active()) { voice=Voice{}; voice.pitch=float(note); voice.velocity=velocity; }
        if(!voice.amp.active() || voice.channel!=channel) smoothedBends[size_t(channel-1)]=bends[size_t(channel-1)];
        if(!legato) { voice.pitch=float(note); voice.amp.on(); voice.filter.on(); }
        voice.channel=channel; voice.note=note; voice.targetVelocity=velocity;
    }
    void selectLast() noexcept {
        uint64_t newest=0; int channel=0,note=0;
        for(size_t c=0;c<held.size();++c)
            for(size_t n=0;n<held[c].size();++n)
                if((held[c][n].down || held[c][n].sustained) && held[c][n].order>newest)
                    { newest=held[c][n].order; channel=int(c)+1; note=int(n); }
        if(newest) {
            if(channel!=voice.channel || note!=voice.note || !voice.amp.active())
                start(channel,note,held[size_t(channel-1)][size_t(note)].velocity,voice.amp.active());
        } else {
            voice.amp.off(smooth.amp.release,sampleRate);
            voice.filter.off(smooth.filter.release,sampleRate);
        }
    }
    Parameters target{},smooth{};
    Voice voice{};
    std::array<std::array<Held,128>,16> held{};
    std::array<bool,16> pedals{};
    std::array<float,16> bends{},smoothedBends{};
    std::array<float,3> weights1{},weights2{};
    uint64_t order=0;
    double sampleRate=44100,lfoPhase=0;
    CutoffSmoother cutoffSmoother;
    Saturation saturator;
    float smoothing=.003f,lfoSmoothing=.01f,lfoValue=0,dcIn=0,dcOut=0;
};
}
