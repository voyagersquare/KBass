#include "Saturation.h"
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {
constexpr double pi=3.14159265358979323846;
void require(bool condition,const char* text) {
    if(!condition) { std::cerr<<"FAIL: "<<text<<'\n'; std::exit(1); }
}
double rms(const std::vector<float>& v) {
    double sum=0; for(float x:v) sum+=double(x)*x;
    return std::sqrt(sum/v.size());
}
double harmonic(const std::vector<float>& v,double hz,double rate) {
    double real=0,imaginary=0;
    for(size_t i=0;i<v.size();++i) {
        const double phase=2*pi*hz*double(i)/rate;
        real+=v[i]*std::cos(phase); imaginary+=v[i]*std::sin(phase);
    }
    return 2*std::hypot(real,imaginary)/v.size();
}
std::vector<float> tone(int type,double hz=100,double amplitude=.2,double rate=48000,float amount=.8f) {
    kbass::Saturation effect; effect.prepare(rate,amount,type);
    const int count=int(rate),warm=count;
    std::vector<float> audio(size_t(count),0);
    for(int i=0;i<warm+count;++i) {
        const float sample=effect.process(float(amplitude*std::sin(2*pi*hz*i/rate)));
        if(i>=warm) audio[size_t(i-warm)]=sample;
    }
    return audio;
}
}
int main() {
    using Effect=kbass::Saturation;
    // The bypass must be bit-exact, with the same fixed delay for every type.
    for(int type=0;type<3;++type) {
        Effect effect; effect.prepare(48000,0,type);
        std::vector<float> input(4096);
        unsigned seed=17;
        for(size_t i=0;i<input.size();++i) {
            seed=1664525u*seed+1013904223u;
            input[i]=float(double(seed)/4294967295.0-.5);
            const float expected=i<size_t(Effect::latencySamples) ? 0 : input[i-Effect::latencySamples];
            require(effect.process(input[i])==expected,"zero amount is an exact latency-compensated bypass");
        }
        effect.reset();
        for(int i=0;i<1024;++i) require(effect.process(0)==0,"reset clears FIR, dry delay and model memories");
    }
    std::array<std::vector<float>,3> sounds;
    for(int type=0;type<3;++type) {
        sounds[size_t(type)]=tone(type);
        const double fundamental=harmonic(sounds[size_t(type)],100,48000);
        const double second=harmonic(sounds[size_t(type)],200,48000),third=harmonic(sounds[size_t(type)],300,48000);
        std::cout<<"model "<<type<<": rms="<<rms(sounds[size_t(type)])<<" H1="<<fundamental<<" H2="<<second<<" H3="<<third<<'\n';
        require(rms(sounds[size_t(type)])>.2/std::sqrt(2.0)*1.8,"each model boosts and thickens a bass sine");
        require(third>.005,"each saturation model adds audible odd harmonics");
        if(type==Effect::tube) require(second>.01,"asymmetric Tube adds even harmonics");
        else require(second<fundamental*.002,"Tape and Transistor remain predominantly symmetric");
    }
    for(int first=0;first<3;++first) for(int second=first+1;second<3;++second) {
        auto difference=sounds[size_t(first)];
        for(size_t i=0;i<difference.size();++i) difference[i]-=sounds[size_t(second)][i];
        require(rms(difference)>.015,"the three models produce distinct waveforms");
    }
    const double tapeBass=harmonic(sounds[Effect::tape],100,48000);
    const double transistorBass=harmonic(sounds[Effect::transistor],100,48000);
    const double tapeTreble=harmonic(tone(Effect::tape,8000),8000,48000);
    const double transistorTreble=harmonic(tone(Effect::transistor,8000),8000,48000);
    std::cout<<"Tape HF ratio="<<tapeTreble/tapeBass<<" Transistor HF ratio="<<transistorTreble/transistorBass<<'\n';
    require(tapeTreble/tapeBass<.8*transistorTreble/transistorBass,"Tape rolls off the top end more than Transistor");

    // At 48 kHz, an unfiltered 9 kHz third harmonic folds to 21 kHz.
    const auto oversampled=tone(Effect::transistor,9000,.4,48000,1);
    std::vector<float> direct(48000);
    for(size_t i=0;i<direct.size();++i) {
        const double input=17*.4*std::sin(2*pi*9000*double(i)/48000),magnitude=std::abs(input),excess=std::max(0.0,magnitude-.45);
        direct[i]=float(std::copysign(magnitude<=.45 ? magnitude : .45+.55*excess/(.55+excess),input));
    }
    const double alias=harmonic(oversampled,21000,48000),unfilteredAlias=harmonic(direct,21000,48000);
    std::cout<<"Aliased H3: oversampled="<<alias<<" direct="<<unfilteredAlias<<'\n';
    require(alias<unfilteredAlias*.1,"4x FIR processing suppresses folded high-frequency harmonics");

    for(double rate:{8000.0,44100.0,48000.0,96000.0,192000.0,384000.0}) for(int type=0;type<3;++type) {
        Effect switched,reference; switched.prepare(rate,.9f,type); reference.prepare(rate,.9f,type);
        for(int i=0;i<int(rate*.1);++i) {
            switched.process(.2f); reference.process(.2f);
        }
        switched.setParameters(.9f,(type+1)%3);
        double previousDifference=0;
        for(int i=0;i<Effect::latencySamples+int(rate*.001);++i) {
            const float input=.2f;
            const double difference=switched.process(input)-reference.process(input);
            require(std::abs(difference)<.06,"type crossfade starts gradually after FIR latency");
            require(std::abs(difference-previousDifference)<.01,"type change has no abrupt onset step");
            previousDifference=difference;
        }
        switched.setParameters(0,type);
        for(int i=0;i<int(rate*.1);++i) {
            const float output=switched.process(i%2==0 ? 4.0f : -4.0f);
            require(std::isfinite(output) && std::abs(output)<6,"extreme input and amount automation remain finite at all supported rates");
        }
        switched.reset(1,type);
        for(int i=0;i<2048;++i) {
            const float output=switched.process(i%3==0 ? 3.0f : -.75f);
            require(std::isfinite(output) && std::abs(output)<3,"full drive remains stable on asymmetric input");
        }
        switched.reset(1,type);
        for(int i=0;i<256;++i) require(switched.process(0)==0,"all model states reset to silence");
    }
    Effect amount,unchanged; amount.prepare(48000); unchanged.prepare(48000);
    for(int i=0;i<4096;++i) { const float input=float(.2*std::sin(2*pi*100*i/48000)); amount.process(input); unchanged.process(input); }
    amount.setParameters(1,Effect::transistor);
    require(std::abs(amount.process(.2f)-unchanged.process(.2f))<.002,"amount automation starts without a discontinuity");
    for(int i=0;i<24000;++i) {
        const float x=float(.2*std::sin(2*pi*100*i/48000)); amount.process(x); unchanged.process(x);
    }
    amount.setParameters(0,Effect::tube);
    for(int i=0;i<24000;++i) {
        const float x=float(.2*std::sin(2*pi*100*i/48000));
        const float a=amount.process(x),b=unchanged.process(x);
        if(i>12000) require(std::abs(a-b)<1e-6,"smoothed return to zero recovers the exact delayed dry signal");
    }
    std::cout<<"PASS: saturation bypass, reported delay, harmonic character, HF colour, smoothing and stability\n";
}
