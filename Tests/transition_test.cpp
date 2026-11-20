#include "../Source/PitchEngine.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>

// Regression for harmonic singing transitions and consonant-like dropouts.
static void testDropoutAndTransitions() {
    tidetune::PitchEngine engine;
    constexpr double sr=48000.0;
    engine.prepare(sr);
    tidetune::Settings s;
    s.root=0;s.scale=0;s.hard=true;s.retuneMs=1.0f;s.strength=1.0f;s.mix=1.0f;
    float block[256]{};
    float* channels[]={block};
    double phase=0.0;
    float previous=0.0f,maxJump=0.0f;
    int firstTarget=-1;
    for(int begin=0;begin<240000;begin+=256) {
        const double hz=(begin<48000?450.0:begin<96000?310.0:begin<144000?392.0:450.0);
        for(int i=0;i<256;++i) {
            phase+=2.0*tidetune::pi*hz/sr;
            const int t=begin+i;
            const float gate=t<182000?1.0f:t<182256?1.0f-(t-182000)/256.0f:
                             t<183000?0.0f:t<183256?(t-183000)/256.0f:1.0f;
            block[i]=gate*(0.18f*std::sin(phase)+0.06f*std::sin(2*phase));
        }
        engine.process(channels,1,256,s);
        if(begin==181760)firstTarget=engine.targetMidi.load();
        if(begin==182512)assert(engine.targetMidi.load()==firstTarget);
        for(int i=0;i<256;++i) {
            assert(std::isfinite(block[i]));
            if(begin+i>8000)maxJump=std::max(maxJump,std::abs(block[i]-previous));
            previous=block[i];
        }
    }
    std::printf("Harmonic step/dropout max sample jump %.5f\n",maxJump);
    assert(maxJump<0.14f);
}
static void testSilenceRelease() {
    tidetune::PitchEngine engine;
    engine.prepare(48000.0);
    tidetune::Settings s;s.scale=0;
    float block[256]{};
    float* channels[]={block};
    int time=0;
    for(int k=0;k<80;++k) {
        for(int i=0;i<256;++i)
            block[i]=0.3f*std::sin(2.0*tidetune::pi*450.0*time++/48000.0);
        engine.process(channels,1,256,s);
    }
    assert(engine.targetMidi.load()>=0);
    for(int k=0;k<40;++k) {
        std::fill(block,block+256,0.0f);
        engine.process(channels,1,256,s);
    }
    assert(engine.targetMidi.load()<0);
}
int main() {
    testDropoutAndTransitions();
    testSilenceRelease();
    std::puts("Transition regression tests passed.");
}
