#ifndef AUDIOCHAIN_H_
#define AUDIOCHAIN_H_

#include <Audio.h>

#define NUM_KNOBS 9
const char *KNOB_LIST[] = {"VCO Knob", "VCF Knob", "VCA Knob", 
  "Vol Knob", "X", "Y", "Angle", "Radius", "MIDI"}; 

class AudioChain {
private:
  int16_t granularMemory[12800];

public:
  AudioPlaySdRaw           playSdWav;
  AudioSynthWaveform       waveform;
  AudioFilterLadder        filter;
  AudioEffectReverb        reverb;
  AudioEffectGranular      granular;
  AudioAmplifier           amp;
  AudioAnalyzeFFT1024      fft;

  AudioConnection          *vcoToVcf;
  AudioConnection          *vcfToEffect;
  AudioConnection          *effectToVca;
  AudioConnection          *vcaToOutput;
  AudioConnection          *vcaToFFT;

  char wavFileName[30];
  bool wavPlaying = false;

  int knobs[NUM_KNOBS];
  int vcoFrqMod = -1;
  int vcfFrqMod = -1;
  int vcfResMod = -1;
  int effectMod = -1;
  int vcaMod = -1;
  int selectedEffect = -1;

  AudioChain() {
	  memset(knobs, 0, NUM_KNOBS*sizeof(knobs[0]));
    waveform.amplitude(0.5);
    waveform.frequency(440);
    filter.frequency(440);
    filter.resonance(0.7);
    granular.begin(granularMemory, 12800);
    granular.beginPitchShift(50);
    vcaToFFT = new AudioConnection(amp, fft);
  };

  void setVcoFrq(int f) {
    waveform.frequency(100 + f * 9);
  }

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

  void updateKnobVal(int knobNum, int newVal) {
     if (knobs[knobNum] != newVal) {
        if (knobNum == vcoFrqMod)
          setVcoFrq(newVal);
        else if (knobNum == vcfFrqMod)
          setVcfFrq(newVal);
        else if (knobNum == vcfResMod)
          setVcfRes(newVal);
        else if (knobNum == effectMod)
          setEffectMod(newVal);
        else if (knobNum == vcaMod)
          setVcaMod(newVal);

        knobs[knobNum] = newVal;
     }
  }

  int getKnobValue(const char* newVal) {
     for (int i = 0; i < NUM_KNOBS; i++) 
        if (strcmp(KNOB_LIST[i], newVal) == 0) 
           return i;
     return -1;
  }
};

#endif /* AUDIOCHAIN_H_ */
