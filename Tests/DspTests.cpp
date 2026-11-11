#include "SynthEngine.h"
#include <cstdlib>
#include <iostream>
#include <vector>

static void require(bool condition,const char* name) {
    if(!condition) { std::cerr<<"FAIL: "<<name<<'\n'; std::exit(1); }
}
static std::vector<float> render(kbass::SynthEngine& e,int count) {
    std::vector<float> audio(static_cast<size_t>(count),0); e.render(audio.data(),nullptr,count); return audio;
}
static double rms(const std::vector<float>& a) {
    double sum=0; for(const auto v:a) { require(std::isfinite(v),"finite audio"); sum+=v*v; }
    return std::sqrt(sum/a.size());
}
static double harmonic(int wave,int number) {
    double real=0,imaginary=0;
    constexpr int count=48000;
    for(int i=0;i<count;++i) {
        const double phase=std::fmod(i*100.0/count,1.0);
        const double value=kbass::oscillator(wave,phase,100.0/count);
        real+=value*std::cos(2*kbass::pi*number*phase);
        imaginary+=value*std::sin(2*kbass::pi*number*phase);
    }
    return 2*std::hypot(real,imaginary)/count;
}
int main() {
    double responseAt50ms=0;
    for(const double rate:{8000.0,44100.0,48000.0,96000.0,192000.0}) {
        kbass::CutoffSmoother cutoff;
        cutoff.prepare(rate,20); cutoff.setTarget(18000);
        double previous=std::log2(20.0),at50ms=0;
        for(int i=0;i<int(rate*.15);++i) {
            const double next=cutoff.nextLog2();
            require(next>=previous && next<=std::log2(18000.0)+1e-10,"cutoff sweep is continuous, monotonic and bounded");
            if(i==int(rate*.05)-1) at50ms=next;
            previous=next;
        }
        require(cutoff.currentHz()>17970,"cutoff remains responsive after smoothing");
        if(responseAt50ms==0) responseAt50ms=at50ms;
        else require(std::abs(at50ms-responseAt50ms)<.02,"cutoff response is independent of sample rate");
        cutoff.setTarget(20);
        for(int i=0;i<int(rate*.15);++i) {
            const double next=cutoff.nextLog2();
            require(next<=previous && next>=std::log2(20.0)-1e-10,"falling cutoff sweep has no overshoot");
            previous=next;
        }
    }
    require(harmonic(2,1)>.999 && harmonic(2,2)<1e-6 && harmonic(2,3)<1e-6,"sine oscillator has only its fundamental");
    require(harmonic(0,2)>.3,"saw includes even harmonics");
    require(harmonic(1,2)<.0001 && harmonic(1,3)>.4,"square has odd harmonics");
    kbass::Parameters p;
    p.wave1=p.wave2=2; p.octave=0; p.detune=0; p.mix=0; p.filterEnv=0;
    p.cutoff=18000; p.resonance=0; p.amp={.001f,.01f,1,.04f};
    kbass::SynthEngine e; e.setParameters(p); e.prepare(48000);
    require(rms(render(e,512))==0,"silence before note on");
    e.noteOn(1,69,1); render(e,2400);
    auto audio=render(e,48000); int crossings=0;
    for(size_t i=1;i<audio.size();++i) if(audio[i-1]<0 && audio[i]>=0) ++crossings;
    require(std::abs(crossings-440)<=1,"OSC1 concert pitch");
    p.mix=1; p.octave=2; e.setParameters(p); e.prepare(48000); e.noteOn(1,69,1); render(e,2400);
    audio=render(e,48000); crossings=0;
    for(size_t i=1;i<audio.size();++i) if(audio[i-1]<0 && audio[i]>=0) ++crossings;
    require(std::abs(crossings-110)<=1,"OSC2 two-octave transposition");
    e.noteOff(1,69); render(e,24000); require(!e.active() && rms(render(e,1024))<1e-6,"amp release reaches silence");
    p.glide=.2f; e.setParameters(p); e.prepare(48000); e.noteOn(1,36,1); render(e,2400); e.noteOn(1,48,1);
    render(e,480); require(e.currentPitch()>36 && e.currentPitch()<38,"legato glide moves gradually");
    render(e,96000); require(std::abs(e.currentPitch()-48)<.03,"glide reaches target pitch");
    e.noteOff(1,48); render(e,96000); require(std::abs(e.currentPitch()-36)<.03,"last-note priority returns to a held note");
    e.sustainPedal(1,true); e.noteOff(1,36); render(e,12000); require(e.active(),"sustain pedal holds released note");
    e.sustainPedal(1,false); render(e,24000); require(!e.active(),"pedal up releases voice");
    e.noteOn(1,36,1); render(e,2048); e.noteOff(1,36); render(e,10); e.noteOn(1,60,1);
    require(e.currentPitch()==60,"a fresh non-legato note does not glide from a release tail");
    e.reset();
    e.noteOn(1,36,1); e.noteOn(2,48,1); e.allNotesOff(1,true); render(e,256); require(e.active(),"channel-specific panic preserves another channel");
    e.allNotesOff(2,true); require(rms(render(e,512))==0,"all sound off clears current voice immediately");
    p.wave1=0; p.mix=0; p.cutoff=18000; e.setParameters(p); e.prepare(48000); e.noteOn(1,60,1); render(e,2400);
    const auto bright=rms(render(e,12000)); p.cutoff=60; e.setParameters(p); render(e,4800);
    require(rms(render(e,12000))<bright*.1,"low-pass cutoff removes high frequency energy");
    p.cutoff=1000; p.filterLfo=3; p.lfoSpeed=5; e.setParameters(p); e.prepare(48000); e.noteOn(1,48,1);
    double minimum=1,maximum=0;
    for(int i=0;i<40;++i) { const auto energy=rms(render(e,1200)); minimum=std::min(minimum,energy); maximum=std::max(maximum,energy); }
    require(maximum>minimum*2,"LFO audibly modulates filter");
    {
        kbass::Parameters stable;
        stable.wave1=2; stable.mix=0; stable.cutoff=18000; stable.filterEnv=0;
        stable.amp={.001f,.01f,1,.1f};
        kbass::SynthEngine unchanged,edited;
        unchanged.setParameters(stable); edited.setParameters(stable);
        unchanged.prepare(48000); edited.prepare(48000);
        unchanged.noteOn(1,69,1); edited.noteOn(1,69,1);
        render(unchanged,48537); render(edited,48537);
        stable.amp.sustain=0; edited.setParameters(stable);
        require(std::abs(render(unchanged,1)[0]-render(edited,1)[0])<.002,"live sustain adjustment does not jump the audio level");
        stable.amp.sustain=1; stable.cutoff=60;
        unchanged.setParameters(stable); edited.setParameters(stable);
        render(unchanged,24000); render(edited,24000);
        stable.cutoff=12000; edited.setParameters(stable);
        require(std::abs(render(unchanged,1)[0]-render(edited,1)[0])<.00001,"a large cutoff step starts without an audio discontinuity");
    }
    kbass::Envelope amp,filter; kbass::Adsr a{.001f,.01f,.8f,.01f}, f{.001f,.01f,.4f,.5f};
    amp.on(); filter.on(); for(int i=0;i<4800;++i) { amp.next(a,48000); filter.next(f,48000); }
    amp.off(a.release,48000); filter.off(f.release,48000);
    for(int i=0;i<4800;++i) { amp.next(a,48000); filter.next(f,48000); }
    require(!amp.active() && filter.active(),"independent amp and filter release times");
    for(double rate:{8000.0,44100.0,48000.0,96000.0,192000.0})
        for(int wave1=0;wave1<3;++wave1) for(int wave2=0;wave2<3;++wave2) {
            p.wave1=wave1; p.wave2=wave2; p.cutoff=18000; p.resonance=1; p.filterEnv=6; p.filterLfo=4; p.pitchLfo=2; p.lfoSpeed=30;
            e.setParameters(p); e.prepare(rate); e.noteOn(1,127,1);
            for(const float v:render(e,4096)) require(std::isfinite(v) && std::abs(v)<=1,"extreme settings remain finite and bounded");
        }
    std::cout<<"PASS: wave spectra, oscillator tuning, envelopes, glide, MIDI, filter, LFO and sample-rate stability\n";
}
