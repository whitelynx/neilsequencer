#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <cstdio>
#include <stdlib.h>
#include <math.h>
#include <string.h>

//#include "gAHDEnv.h"
#include "CGrain.h"
#include "themi.h"

class CGrain;

inline int b2n(int b) {
  int n = ((int(b / 16) * 12) + (b % 16));
  return n -= 49;
}

float HackValue0520a(int maxv, int minv, int value) {
  int rng = maxv - minv;
  if (value < rng / 2) 
    return (((0.5f / (rng / 2.0f)) * value) + 0.5f);
  else return ((float)value / ((float)rng / 2.0f));
} 

float HackValue0520(int maxv, int minv, int value) {
  int rng = maxv - minv;
  if (value < rng / 2) 
    return (((0.75f / (rng / 2.0f)) * value) + 0.25f);
  else return (((float)value + ((value - 127) * 2)) / ((float)rng / 2.0f));
}

struct acloud::WAVESEL {
  int wnum;
  int waveslot;
};

acloud::acloud() {
  global_values = &gval;
}

acloud::~acloud() {

}

void acloud::process_events() {
  int o = _master_info->samples_per_tick;
  if (gval.note != zzub::note_value_none) {
    if (gval.note != 255) {
      if (!cloud_is_playing) {
	for (int j = 0; j < maxgrains; j++) 
	  Grain[j].IsActive = 0;
	gnext = SetNextGrain(SetDens(densMode));
	gcount = 0;
      }
      cloud_is_playing = true;
      int n = b2n(gval.note);
      nrate = powf(2.0f, ((n) / 12.0f));
      offsCount = 0;
    }
    else{
      cloud_is_playing = false;
    }
  }
  if (gval.seed != 0xFFFF) 
    srand(gval.seed);
  if (gval.wnumber1 != zzub::wavetable_index_value_none) 
    wavenum1 = gval.wnumber1;
  if (gval.offset1 != 0xFFFF) 
    offset1 = (float)gval.offset1 / 0xFFFE;
  if (gval.offset2 != 0xFFFF) 
    offset2 = (float)gval.offset2 / 0xFFFE;
  if (gval.wnumber2 != zzub::wavetable_index_value_none) 
    wavenum2 = gval.wnumber2;
  if (gval.offstype != 0xFF) 
    offstype = gval.offstype;
  if (gval.offsmode != 0xFF){
    offsMode = gval.offsmode;
    offsCount = 0;
  }
  if (gval.skiprate != 0xFF) 
    skiprate = HackValue0520(254, 0, gval.skiprate);
  if (gval.w2offset1 != 0xFFFF) 
    w2offset1 = (float)gval.w2offset1 / 0xFFFE;
  if (gval.w2offset2 != 0xFFFF) 
    w2offset2 = (float)gval.w2offset2 / 0xFFFE;
  if (gval.wavemix != 0xFFFF) 
    wavemix = gval.wavemix;
  if (gval.gduration != 0xFFFF) 
    gdur = gval.gduration * (_master_info->samples_per_second / 44100);
  if (gval.gdrange != 0xFFFF) 
    gdrange = gval.gdrange;
  if (gval.amp != 0xFFFF){
    iamp = (int)gval.amp;
    if (iamp > 0x8000){
      ampt = 1.0f;
      ampb = (float)(iamp - 0x8000) / 0x8000;
    }
    else{
      ampb = 0.000001f;
      ampt = (float)iamp / 0x8000;
    }
  }
  if (gval.rate != 0xFF) 
    rate = HackValue0520(254, 0, gval.rate);
  if (gval.rrate != 0xFF) 
    rrate = gval.rrate;
  if (gval.envamt != 0xFF) 
    envamt = (float)gval.envamt / 512.0f;
  if (gval.envq != 0xFF) 
    envq = (float)gval.envq/128.0f;
  if (gval.lpan != 0xFF) 
    lpan = (float)gval.lpan;
  if (gval.rpan != 0xFF) 
    rpan = (float)gval.rpan;
  //Cloud Params
  if (gval.densmode != 0xFF) 
    densMode = gval.densmode;
  if (gval.density != 0xFFFF) 
    density = gval.density; 
  densfactor = (float)density / 1000.0f;
  if (gval.maxgrains != 0xFF) 
    maxgrains = gval.maxgrains;
}

void acloud::init(zzub::archive *arc) {

  // ThisMachine = _host->get_metaplugin();
  // _host->set_track_count(ThisMachine,2);

  //mi vars
  wavenum1 = wavenum2 = 1;
  wavemix = 0x4000; 
  maxgrains = 20;
  gdur = 1000;
  gdrange = 0;
  rate =  nrate = 1.0f;
  rrate = 0;
  cloud_is_playing = false;
  gcount = 0;
  gnext = SetNextGrain(20);
  offset1 = offset2 = 0.0f;
  offstype = 0;
  w2offset1 = w2offset2 = 0.0f;
  envamt = envq = 0.0f;
  waveslot = 1;
  offsCount = 0;
  offsInc = 0;
  offsMode = 0;
  skiprate = 1.0f;
  density = 20;
  //init grain class
  for(int i = 0; i < 127; i++) {
    Grain[i].SetMiPointers(&_master_info->samples_per_second);
    Grain[i].Init();
  }
  iamp = 0xFFFF;
  ampt = ampb = 0.0f;
  envamt = envq = 0.0f;
  lpan = rpan = 0.0f;
  skiprate = 1.0f;
  process_events();
  cloud_is_playing = false;
}


bool acloud::process_stereo(float **pin, float **pout, 
			    int numsamples, int mode) {
  float psamples[256 * 2];
  int g = -1;
  int w = 1;
  int last_gnext = 0;
  bool IsFirstInWork = true;
  last_gnext = gnext - (int)gcount;
  gcount += numsamples;
  if (gcount > gnext && cloud_is_playing) {
    do {
      g = FindGrain(maxgrains);		 
      if (g >= 0) {
	Grain[g].IsActive = 0;		
	w = SelectWave(wavemix);
	if (_host->get_wave_level(w, 0)) {	
	  Grain[g].CDown = last_gnext;
	  Grain[g].Set(SetGDRange(), SetOffset(waveslot, w), 1, 
		       nrate * rate * GetRandRate(), GetRandPan());
	  Grain[g].SetWave(w, (_host->get_wave(w)->flags & 
			       zzub::wave_flag_stereo), 
			   _host->get_wave_level(w, 0));
	  Grain[g].SetEnv(Grain[g].Duration, envamt, envq);
	  Grain[g].SetAmp(ampt, ampb, _host->get_wave(w)->volume);
	  Grain[g].IsActive = 1;
	}
      }		
      gnext = SetNextGrain(SetDens(densMode));
      last_gnext += gnext;
      offsInc += gnext;			
    } while (last_gnext < numsamples);
    gcount = numsamples - (last_gnext - gnext);
  }
  if (!CheckActivGrains(maxgrains)) {
    memset(pout[0], 0, (numsamples) * sizeof(float));
    memset(pout[1], 0, (numsamples) * sizeof(float));
    return false;
  }
  memset(pout[0], 0, (numsamples) * sizeof(float));
  memset(pout[1], 0, (numsamples) * sizeof(float));
  for (int j = 0; j < maxgrains; j++) {
    if (Grain[j].IsActive == 1 && IsFirstInWork) 
      Grain[j].Generate(psamples, numsamples, 
			_host->get_wave_level(Grain[j].Wave, 0));
    if(Grain[j].IsActive == 1 && !IsFirstInWork) 
      Grain[j].GenerateAdd(psamples, numsamples, 
			   _host->get_wave_level(Grain[j].Wave, 0));
    IsFirstInWork = false;
  }
  for (int i = 0; i < numsamples; i++) {
    pout[0][i] = psamples[2 * i] * downscale;
    pout[1][i] = psamples[2 * i + 1] * downscale;
  }
  return true;

}

void acloud::command(int i) {
  int j;
  switch (i) {
  case 0:
    break;
  case 1:
    cloud_is_playing = false;
    for(j = 0; j < maxgrains; j++) {
      Grain[j].IsActive = 0;
      Grain[j].Init();
    }
    break;
  case 2:
    break;
  default:
    break;
  }
}

const char * acloud::describe_value(int param, int value) {
  static char txt[16];
  float t, p;
  switch(param) {
  case 0:				
    return NULL;
    break;
  case 2:
    sprintf(txt, "%d %s", value, _host->get_wave_name(value));
    return txt;
    break;
  case 3:		
    sprintf(txt, "%X %.1f%%", value, ((float)value / 65534.0f) * 100);
    return txt;
    break;
  case 4:		
    sprintf(txt, "%X %.1f%%", value, ((float)value / 65534.0f) * 100);
    return txt;
    break;
  case 5:
    sprintf(txt, "%d %s", value, _host->get_wave_name(value));
    return txt;
    break;
  case 6:		
    sprintf(txt, "%X %.1f%%", value, ((float)value / 65534.0f) * 100);
    return txt;
    break;
  case 7:		
    sprintf(txt, "%X %.1f%%", value, ((float)value / 65534.0f) * 100);
    return txt;
    break;
  case 8:
    if (value == 0) 
      return "Off [!Slaved]";
    if (value == 1) 
      return "On [Slaved]";
    else 
      return "N00b";
    break;
  case 9:
    if (value == 0) 
      return "Random";
    if (value == 1) 
      return "Forwards";
    else 
      return "Backwards";
    break;
  case 10:
    sprintf(txt, "%.2f", HackValue0520(254, 0, value));
    return txt;
    break;
  case 11:		
    sprintf(txt, "%.1f%% / %.1f%%", 100 - ((value / 32767.0f) * 100), 
	    (value / 32767.0f) * 100);
    return txt;
    break;
  case 12:		
    sprintf(txt, "%.1fms", 
	    (value / (float)_master_info->samples_per_second) * 1000);
    return txt;
    break;
  case 13:		
    sprintf(txt, "%.1fms", 
	    (value / (float)_master_info->samples_per_second) * 1000);
    return txt;
    break;
  case 14:	
    t = ((float)value / 32767.0f) - 1.0f;
    if (t < 0.0f) 
      t = 0.0f;
    p = ((float)(value / 32767.0f));
    if (p > 1.0f) 
      p = 1.0f;
    sprintf(txt, "B%.2f / T%.2f", t, p);
    return txt;
    break;
  case 15:		
    sprintf(txt, "%.2f", HackValue0520(254, 0, value));
    return txt;
    break;
  case 16:		
    sprintf(txt, "%.1f semi", (float)value / 10.0f);
    return txt;
    break;
  case 17:		
    sprintf(txt, "%.1f%%", ((float)value / 254.0f) * 100);
    return txt;
    break;
  case 18:		
    sprintf(txt, "%.2f", ((float)value / 127.0f) - 1.0f);
    return txt;
    break;
  case 19:
    if (value <= 64) 
      sprintf(txt, "L %.2f", ((float)value / 64.0f) - 1.0f);
    else 
      sprintf(txt, "R %.2f", ((float)value / 64.0f) - 1.0f);
    return txt;
    break;
  case 20:
    if (value < 64) 
      sprintf(txt, "L %.2f", ((float)value / 64.0f) - 1.0f);
    else 
      sprintf(txt, "R %.2f", ((float)value / 64.0f) - 1.0f);
    return txt;
    break;
  case 21:
    if (value == 0) 
      return "Avr.Grs pSec";
    if (value == 1) 
      return "Perceived";
    else 
      return "N00b";
    break;
  case 22:
    sprintf(txt, "%d/%.2f%%", value, (float)value / 10.0f);
    return txt;
    break;
  default: return NULL;
    break;
  };
}

void acloud::set_track_count(int n) {
  //numTracks = n;
}

void acloud::stop() {
  cloud_is_playing = false;
  for(int j = 0; j < maxgrains; j++){
    Grain[j].IsActive = 0;
    Grain[j].Init();
  }
}

void acloud::destroy() { 
  delete this; 
}

void acloud::attributes_changed() {
  // Do nothing.
}

const char *zzub_get_signature() {
  return ZZUB_SIGNATURE; 
}

inline int f2i(double d) {
  const double magic = 6755399441055744.0; // 2^51 + 2^52
  double tmp = (d - 0.5) + magic;
  return *(int*)&tmp;
}

inline int acloud::FindGrain(int maxg) {
  for (int i = 0; i < maxg; i++) 
    if (Grain[i].IsActive == 0) 
      return i;
  return -1;
}

inline int acloud::SetNextGrain(int dens) {
  float r = ((float)rand() * 2.0f / RAND_MAX);
  int t = (int)((_master_info->samples_per_second / dens) * r);
  return t + 1;
}

inline int acloud::CountGrains() {
  int c = 0;
  for (int i = 0; i < maxgrains; i++)
    if (Grain[i].IsActive) c++;

  return c;
}

inline int acloud::SetDens(int mode) {
  if (mode == 0) 
    return density;
  return (int)((220500.0f / gdur) * densfactor) + 1;
}

inline bool acloud::CheckActivGrains(int maxg) {
  for (int i = 0; i < maxg; i++) 
    if (Grain[i].IsActive == 1) 
      return true;
  return false;
}

double acloud::SetOffset(int wave, int wnum) {
  float s = 0.0f;
  float e = 0.0f;
  double rtn;
  const zzub::wave_level *pwl = _host->get_wave_level(wnum,0);
  int wnums = pwl->sample_count;
	
  s = offset1;
  e = offset2;

  if (wave == 2 && offstype == 0) {
    s = w2offset1;
    e = w2offset2;
  }


  offsCount += (offsInc * skiprate * ((float)pwl->samples_per_second / (float)_master_info->samples_per_second));
  offsInc = 0; //191205 to reset now incremental counter.

  if(offsMode == 0) {
    if (e == 0.0f) return s * wnums;//check if no range
    if (e > (1.0f-s)) e = (1.0f-s);//check if range greater than available range
    float r = (float)rand()/RAND_MAX;//random factor
    return ((r*e)+s)*wnums;
  }
	
  if(offsMode == 1) {
    if (offsCount + (wnums * s) > wnums) 
      offsCount = 0;
    if (e == 0.0f) 
      return (wnums * s) + offsCount;
    if (e > (1.0f - s)) 
      e = (1.0f - s);
    float r = (float)rand() / (float)RAND_MAX;
    rtn = (((e * r) + s) * wnums) + offsCount;
    //assert (rtn <= wnums);
    if (rtn > wnums) 
      return (wnums * s) + ((int)rtn % wnums);
		
    return rtn;
  }

  if(offsMode == 2){
    if ((wnums * s) - offsCount < 0) 
      offsCount = 0;
    if (e == 0.0f) 
      return (wnums * s) - offsCount;
    if (e > s) 
      e = s; // Check if range is greated than is available.
    float r = (float)rand() / (float)RAND_MAX;
    rtn = ((s - (e * r)) * wnums) - offsCount;
    //assert (rtn <= wnums);
    if (rtn < 0) 
      return (wnums * s);// - (rtn-(wnums*s));
    return rtn;
  }
  if (e == 0.0f) 
    return s * wnums;
  if (e > (1.0f - s)) 
    e = (1.0f - s);
  float r = (float)rand() / (float)RAND_MAX;
  return ((r * e) + s) * wnums;
}

inline int acloud::SelectWave(int mix) {
  // The win version, I think, assumed that RAND_MAX was always
  // 0xffff, and used the value of the paraWaveMix parameter directly
  // as a probability (because 0xffff is its maximum value). Here,
  // instead, we calculate: (mix / paraWaveMix->value_max) and and
  // (rand() / RAND_MAX). Each of these two quantities is in [0, 1] so
  // they can be directly compared. Similar story in SelectWave2,
  // below. -- jmmcd
  float r = (float)rand() / (float) RAND_MAX;
  float m = mix / (float)paraWaveMix->value_none;
  if (r > m) {
    waveslot = 1;
    return wavenum1;
  }
  waveslot = 2;
  return wavenum2;
}

void acloud::SelectWave2(int mix, WAVESEL * Wv){
  Wv->wnum = wavenum2;
  Wv->waveslot = 2;
  if (rand() / (float) RAND_MAX < mix / (float) paraWaveMix->value_max) {
    Wv->wnum = wavenum1;
    Wv->waveslot = 1;
  }
}

inline int acloud::SetGDRange() {
  if (gdrange <= gdur) 
    return gdur;
  float r = (float)rand() / RAND_MAX;
  return gdur + (int)((gdrange - gdur) * r);
}

inline float acloud::GetRandRate() {
  //float rtn = 0.0f;
  if (rrate == 0) 
    return 1.0f;
  // I think 0x4000 was intended as 
  float r = ((float)rand() * 2.0f / RAND_MAX) - 1.0f;
  float q = (r * rrate) / 120.0f;
  //float n = (q/128.0f);
  return powf(2.0f, q);
  //return x;
}

float acloud::GetRandPan() {
  float pwidth;
  float r = (float)rand() / RAND_MAX;
  if (rpan > lpan) {
    pwidth = (rpan - lpan) / 128.0f;
    return (pwidth * r) + (float)lpan / 128.0f;
  }
  pwidth = (lpan - rpan) / 128.0f;
  return (pwidth * r) + (float)rpan / 128.0f;
}
