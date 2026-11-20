#pragma once
// TideTune 0.4: allocation-free sidechain compressor/envelope controller.
#include <algorithm>
#include <array>
#include <cmath>
namespace tidetune {
struct SidechainSettings {
    bool enabled=false, external=false;
    int target=0; // gain, wet mix, correction strength
    float thresholdDb=-24.0f, ratio=4.0f, attackMs=5.0f, releaseMs=150.0f;
    float kneeDb=6.0f, amount=0.5f, hpfHz=0.0f, lpfHz=0.0f;
};
class SidechainEngine final {
public:
    static constexpr int maxChunk=4096;
    void prepare(double sampleRate) noexcept {
        sr_=std::clamp(sampleRate,8000.0,192000.0);
        envelope_=0.0f; appliedGain_=1.0f; hpPrev_=hpState_=lpState_=0.0f;
        gains_.fill(1.0f);
    }
    float analyze(const float* const* main, int mainN, const float* const* aux,
                  int auxN, int samples, const SidechainSettings& s) noexcept {
        samples=std::clamp(samples,0,maxChunk);
        if(samples==0)return 1.0f;
        const bool usable=s.enabled&&(!s.external||(aux&&auxN>0&&aux[0]));
        const float atk=coef(s.attackMs), rel=coef(s.releaseMs), gainSlew=coef(2.0f);
        const float hp=s.hpfHz>0?std::exp(-2.0f*pi*std::clamp(s.hpfHz,30.0f,1000.0f)/(float)sr_):0.0f;
        const float lp=s.lpfHz>0?std::exp(-2.0f*pi*std::clamp(s.lpfHz,500.0f,18000.0f)/(float)sr_):0.0f;
        const float ratio=std::clamp(s.ratio,1.0f,20.0f);
        const float knee=std::clamp(s.kneeDb,0.0f,18.0f);
        const float threshold=std::clamp(s.thresholdDb,-60.0f,0.0f);
        const float amount=s.enabled?std::clamp(s.amount,0.0f,1.0f):0.0f;
        float sum=0.0f;
        for(int i=0;i<samples;++i) {
            float d=0.0f;
            if(usable) {
                const float* const* channels=s.external?aux:main;
                const int n=s.external?auxN:mainN;
                if(channels&&n>0&&channels[0]) {
                    d=channels[0][i];
                    if(n>1&&channels[1])d=(d+channels[1][i])*0.5f;
                }
            }
            if(s.hpfHz>0.0f) {
                const float y=hp*(hpState_+d-hpPrev_);
                hpPrev_=d;hpState_=y;d=y;
            }
            if(s.lpfHz>0.0f) {
                lpState_=lp*lpState_+(1.0f-lp)*d;d=lpState_;
            }
            const float level=std::abs(d);
            envelope_+=(level>envelope_?atk:rel)*(level-envelope_);
            const float over=20.0f*std::log10(std::max(envelope_,1.0e-8f))-threshold;
            const float factor=1.0f-1.0f/ratio;
            float gr=0.0f;
            if(knee<0.001f)gr=factor*std::max(0.0f,over);
            else if(over>=knee*0.5f)gr=factor*over;
            else if(over>-knee*0.5f) {
                const float x=over+knee*0.5f;
                gr=factor*x*x/(2.0f*knee);
            }
            const float target=std::pow(10.0f,-gr*amount/20.0f);
            appliedGain_+=gainSlew*(target-appliedGain_);
            appliedGain_=std::clamp(appliedGain_,0.0f,1.0f);
            gains_[i]=appliedGain_;sum+=appliedGain_;
        }
        lastReductionDb_=-20.0f*std::log10(std::max(appliedGain_,1.0e-8f));
        return sum/static_cast<float>(samples);
    }
    void applyGain(float* const* channels,int n,int samples) const noexcept {
        for(int ch=0;ch<n;++ch)if(channels[ch])
            for(int i=0;i<samples;++i)channels[ch][i]*=gains_[i];
    }
    float gainAt(int i) const noexcept {return gains_[std::clamp(i,0,maxChunk-1)];}
    float reductionDb() const noexcept{return lastReductionDb_;}
private:
    float coef(float ms) const noexcept {
        return 1.0f-std::exp(-1000.0f/(std::max(ms,0.1f)*static_cast<float>(sr_)));
    }
    static constexpr float pi=3.14159265358979323846f;
    double sr_=48000.0;
    float envelope_=0.0f,appliedGain_=1.0f,hpPrev_=0.0f,hpState_=0.0f,lpState_=0.0f;
    float lastReductionDb_=0.0f;
    std::array<float,maxChunk> gains_{};
};
} // namespace tidetune
