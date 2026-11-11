#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace kbass {

// Three character models, running at 4x with matched linear-phase FIR
// interpolation/decimation. Dry is delayed by the same 32 host samples.
class Saturation {
public:
    enum Type { tube=0,tape=1,transistor=2 };
    static constexpr int factor=4,taps=129,latencySamples=(taps-1)/factor;
    void prepare(double rate,float initialAmount=0,int initialType=tube) noexcept {
        sampleRate=std::clamp(rate,8000.0,384000.0);
        highRate=sampleRate*factor;
        amountCoefficient=1-std::exp(-1/(.012*sampleRate));
        typeCoefficient=1-std::exp(-1/(.020*sampleRate));
        sagCoefficient=1-std::exp(-1/(.050*highRate));
        attackCoefficient=1-std::exp(-1/(.002*highRate));
        releaseCoefficient=1-std::exp(-1/(.050*highRate));
        headCoefficient=1-std::exp(-2*pi*110/highRate);
        edgeCoefficient=1-std::exp(-2*pi*1800/highRate);
        dcPole=std::exp(-2*pi*5/highRate);
        const double denominator=bessel(8.0),centre=(taps-1)*.5;
        double sum=0;
        for(int i=0;i<taps;++i) {
            const double distance=i-centre,relative=distance/centre;
            const double sinc=std::abs(distance)<1e-12 ? .225 : std::sin(2*pi*.1125*distance)/(pi*distance);
            coefficients[size_t(i)]=sinc*bessel(8*std::sqrt(std::max(0.0,1-relative*relative)))/denominator;
            sum+=coefficients[size_t(i)];
        }
        for(auto& c:coefficients) c/=sum;
        reset(initialAmount,initialType);
    }
    void setParameters(float amount,int type) noexcept {
        targetAmount=std::clamp(double(amount),0.0,1.0); targetType=std::clamp(type,0,2);
    }
    void reset(float amount=0,int type=tube) noexcept {
        setParameters(amount,type); currentAmount=targetAmount;
        for(int i=0;i<3;++i) typeWeights[size_t(i)]=i==targetType ? 1.0 : 0.0;
        inputHistory.fill(0); outputHistory.fill(0); dryHistory.fill(0);
        inputIndex=outputIndex=dryIndex=0;
        tubeSag=tapeEnvelope=tapeHead=magnetisation=tapeLow=transistorLow=dcInput=dcOutput=0;
    }
    float process(float input) noexcept {
        currentAmount+=amountCoefficient*(targetAmount-currentAmount);
        if(std::abs(currentAmount-targetAmount)<1e-12) currentAmount=targetAmount;
        for(int i=0;i<3;++i) typeWeights[size_t(i)]+=typeCoefficient*((i==targetType ? 1.0 : 0.0)-typeWeights[size_t(i)]);
        const double dry=dryHistory[size_t(dryIndex)];
        dryHistory[size_t(dryIndex)]=input; if(++dryIndex==latencySamples) dryIndex=0;
        inputHistory[size_t(inputIndex)]=input;
        const double amount=currentAmount,drive=amount*amount;
        const double magneticCoefficient=1-std::exp(-2*pi*std::min(14500-7500*amount,sampleRate*.45)/highRate);
        const double tapeCoefficient=1-std::exp(-2*pi*std::min(17500-9500*amount,sampleRate*.45)/highRate);
        double downsampled=0;
        for(int phase=0;phase<factor;++phase) {
            double high=0; int index=inputIndex;
            for(int tap=phase;tap<taps;tap+=factor) {
                high+=factor*coefficients[size_t(tap)]*inputHistory[size_t(index)];
                if(--index<0) index=int(inputHistory.size())-1;
            }
            tubeSag+=sagCoefficient*(std::abs(high)-tubeSag);
            const double bias=amount*(.20+.03*std::min(tubeSag,3.0));
            const double biasValue=std::tanh(bias);
            const double tubeGain=(1+8*drive)/(1+.45*amount*tubeSag);
            const double tubeSample=(std::tanh(tubeGain*high+bias)-biasValue)/(1-biasValue*biasValue);

            const double envelopeCoefficient=std::abs(high)>tapeEnvelope ? attackCoefficient : releaseCoefficient;
            tapeEnvelope+=envelopeCoefficient*(std::abs(high)-tapeEnvelope);
            tapeHead+=headCoefficient*(high-tapeHead);
            const double tapeGain=(1+7*drive)/(1+.55*amount*tapeEnvelope);
            const double magneticTarget=std::tanh(tapeGain*(high+.18*amount*tapeHead)+.22*amount*magnetisation);
            magnetisation+=magneticCoefficient*(magneticTarget-magnetisation);
            tapeLow+=tapeCoefficient*(magnetisation-tapeLow);

            transistorLow+=edgeCoefficient*(high-transistorLow);
            const double transistorInput=(1+16*drive)*(high+.12*amount*(high-transistorLow));
            const double magnitude=std::abs(transistorInput),excess=std::max(0.0,magnitude-.45);
            const double transistorSample=std::copysign(magnitude<=.45 ? magnitude : .45+.55*excess/(.55+excess),transistorInput);

            const double wet=typeWeights[0]*tubeSample+typeWeights[1]*tapeLow+typeWeights[2]*transistorSample;
            const double blocked=wet-dcInput+dcPole*dcOutput;
            dcInput=wet; dcOutput=std::abs(blocked)<1e-24 ? 0 : blocked;
            outputHistory[size_t(outputIndex)]=dcOutput;
            // Pick phase zero so the complete wet path has exactly the same
            // nominal FIR group delay as the dry path (32 host samples).
            if(phase==0) {
                index=outputIndex;
                for(int tap=0;tap<taps;++tap) {
                    downsampled+=coefficients[size_t(tap)]*outputHistory[size_t(index)];
                    if(--index<0) index=taps-1;
                }
            }
            if(++outputIndex==taps) outputIndex=0;
            flush(tubeSag); flush(tapeEnvelope); flush(tapeHead); flush(magnetisation); flush(tapeLow); flush(transistorLow);
        }
        if(++inputIndex==int(inputHistory.size())) inputIndex=0;
        return float(dry+amount*(downsampled-dry));
    }
private:
    static constexpr double pi=3.14159265358979323846;
    static double bessel(double x) noexcept {
        double sum=1,term=1;
        for(int i=1;i<30;++i) { term*=x*x/(4*i*i); sum+=term; if(term<sum*1e-16) break; }
        return sum;
    }
    static void flush(double& value) noexcept { if(std::abs(value)<1e-24) value=0; }
    std::array<double,taps> coefficients{},outputHistory{};
    std::array<double,(taps+factor-1)/factor> inputHistory{};
    std::array<double,latencySamples> dryHistory{};
    std::array<double,3> typeWeights{{1,0,0}};
    int inputIndex=0,outputIndex=0,dryIndex=0,targetType=tube;
    double sampleRate=48000,highRate=192000,currentAmount=0,targetAmount=0;
    double amountCoefficient=.002,typeCoefficient=.001,sagCoefficient=.0001,attackCoefficient=.002,releaseCoefficient=.0001;
    double headCoefficient=.003,edgeCoefficient=.06,dcPole=.9998;
    double tubeSag=0,tapeEnvelope=0,tapeHead=0,magnetisation=0,tapeLow=0,transistorLow=0,dcInput=0,dcOutput=0;
};
}
