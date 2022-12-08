#ifndef AUDIOCHAIN_H_
#define AUDIOCHAIN_H_

#include <Audio.h>

#define NUM_KNOBS 8
const char *KNOB_LIST[] = {"CV1", "CV2", "CV3", "CV4", "CV5", "CV6", "CV7", "CV8"}; 

class AudioChain {
private:
  int16_t granularMemory[12800];

public:
  AudioPlaySdRaw           playSdWav;
  AudioFilterLadder        filter;
  AudioEffectReverb        reverb;
  AudioEffectGranular      granular;
  AudioAmplifier           amp;

  AudioConnection          *vcoToVcf;
  AudioConnection          *vcfToEffect;
  AudioConnection          *effectToVca;
  AudioConnection          *vcaToOutput;

  char wavFileName[30];
  bool wavPlaying = false;

  int knobs[NUM_KNOBS];
  int vcfFrqMod = -1;
  int vcfResMod = -1;
  int effectMod = -1;
  int vcaMod = -1;
  int selectedEffect = -1;

  AudioChain() {
	  memset(knobs, 0, NUM_KNOBS*sizeof(knobs[0]));
    filter.frequency(440);
    filter.resonance(0.7);
    granular.begin(granularMemory, 12800);
    granular.beginPitchShift(50);
  };

  void setVcfFrq(int f) {
    filter.frequency(f * 300);
  }

  void setVcfRes(int q) {
    filter.resonance(float(q) / 100);
  }

  void setEffectMod(int x) {
    if (selectedEffect == 0) 
      reverb.reverbTime((float)x / 100);
    else if (selectedEffect == 1) 
      granular.setSpeed((float)map(x, 0, 100, 500, 2000) / 1000);
  }

  void setVcaMod(int a) {
    amp.gain((float)a / 100);
  }

  int getKnobValue(const char* newVal) {
     for (int i = 0; i < NUM_KNOBS; i++) 
        if (strcmp(KNOB_LIST[i], newVal) == 0) 
           return i;
     return -1;
  }
};

#endif /* AUDIOCHAIN_H_ */
