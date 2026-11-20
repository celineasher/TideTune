#include "../Source/SidechainEngine.h"
#include <cassert>
#include <cmath>
#include <cstdio>
int main() {
    tidetune::SidechainEngine engine;
    engine.prepare(48000);
    tidetune::SidechainSettings s;
    s.enabled=true; s.external=true; s.thresholdDb=-30.0f;
    s.ratio=8.0f; s.kneeDb=0.0f; s.amount=1.0f;
    s.attackMs=1.0f; s.releaseMs=100.0f;
    float vocal[256], kick[256];
    for(int i=0;i<256;++i){vocal[i]=0.2f;kick[i]=1.0f;}
    const float* main[]={vocal};const float* aux[]={kick};
    float* output[]={vocal};float reduction=1.0f;
    for(int b=0;b<30;++b)reduction=engine.analyze(main,1,aux,1,256,s);
    std::printf("External steady gain %.5f (%.2f dB reduction)\n",
        reduction,engine.reductionDb());
    assert(reduction<0.2f);
    engine.applyGain(output,1,256);
    assert(vocal[255]<0.045f);
    for(int i=0;i<256;++i)kick[i]=0.0f;
    for(int b=0;b<280;++b)reduction=engine.analyze(main,1,aux,1,256,s);
    assert(reduction>0.98f);
    // Without an external bus, loud vocal audio must not trigger ducking.
    for(int b=0;b<280;++b)reduction=engine.analyze(main,1,nullptr,0,256,s);
    assert(reduction>0.99f);
    for(int i=0;i<256;++i)vocal[i]=0.2f;
    s.external=false;
    for(int b=0;b<30;++b)reduction=engine.analyze(main,1,nullptr,0,256,s);
    assert(reduction<0.6f);
    s.enabled=false;
    for(int b=0;b<280;++b)reduction=engine.analyze(main,1,nullptr,0,256,s);
    assert(reduction>0.99f);
    for(int i=0;i<256;++i)assert(std::isfinite(engine.gainAt(i)));
    std::puts("Sidechain DSP tests passed.");
}
