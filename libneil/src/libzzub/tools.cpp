/*
Copyright (C) 2003-2007 Anders Ervik <calvin@countzero.no>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <cstdio>
#include <string>
#include <dlfcn.h>

#include "common.h"
#include "tools.h"

char backslashToSlash(char c) { if (c=='\\') return '/'; return c; }

void AddM2SPan(float* output, float* input, int numSamples, float inAmp, float inPan) {
	float panR=1.0f, panL=1.0f;
	if (inPan<1) {
		panR=inPan;	// when inPan<1, fade out right
	}
	if (inPan>1) {
		panL=2-inPan;	// when inPan>1, fade out left
	}
	for (int i=0; i<numSamples; i++) {
		float L=input[i]*panL * inAmp;
		float R=input[i]*panR * inAmp;

		output[i*2]+=L;
		output[i*2+1]+=R;

	}
}

void AddS2SPan(float* output, float* input, int numSamples, float inAmp, float inPan) {
	float panR=1.0f, panL=1.0f;
	if (inPan<1) {
		panR=inPan;	// when inPan<1, fade out right
	}
	if (inPan>1) {
		panL=2-inPan;	// when inPan>1, fade out left
	}
	for (int i=0; i<numSamples; i++) {
		float L=input[i*2]*panL * inAmp;
		float R=input[i*2+1]*panR * inAmp;

		output[i*2]+=L;
		output[i*2+1]+=R;

	}
}

void AddS2SPanMC(float** output, float** input, int numSamples, float inAmp, float inPan) {
	if (!numSamples)
		return;
	float panR=1.0f, panL=1.0f;
	if (inPan<1) {
		panR=inPan;	// when inPan<1, fade out right
	}
	if (inPan>1) {
		panL=2-inPan;	// when inPan>1, fade out left
	}
	float *pI0 = input[0];
	float *pI1 = input[1];
	float *pO0 = output[0];
	float *pO1 = output[1];
	do
	{
		*pO0++ += *pI0++ * panL * inAmp;
		*pO1++ += *pI1++ * panR * inAmp;
	} while (--numSamples);
}

void Amp(float *pout, int numsamples, float amp) {
	for (int i=0; i<numsamples; i++) {
		pout[i]*=amp;
	}
}

float linear_to_dB(float val) { 
	return(20.0f * log10(val)); 
}

float dB_to_linear(float val) {
	if (val == 0.0) return(1.0);
	return (float)(pow(10.0f, val / 20.0f));
}


#ifdef _USE_SEH
// the translator function
void __cdecl SEH_To_Cpp(unsigned int u, EXCEPTION_POINTERS *exp) {
  throw u;        // throw an exception of type int
}

#endif

void handleError(std::string errorTitle) {
  printf("%s: There was an error", errorTitle.c_str());
}



void CopyStereoToMono(float *pout, float *pin, int numsamples, float amp)
{
	do
	{
		*pout++ = (pin[0] + pin[1]) * amp;
		pin += 2;
	} while(--numsamples);
}


void AddStereoToMono(float *pout, float *pin, int numsamples, float amp)
{
	do
	{
		*pout++ += (pin[0] + pin[1]) * amp;
		pin += 2;
	} while(--numsamples);
}

void AddStereoToMono(float *pout, float *pin, int numsamples, float amp, int ch)
{
	do
	{
		*pout++ += pin[ch] * amp;
		pin += 2;
	} while(--numsamples);
}

void CopyM2S(float *pout, float *pin, int numsamples, float amp)
{
	do
	{
		double s = *pin++ * amp;
		pout[0] = (float)s;
		pout[1] = (float)s;
		pout += 2;
	} while(--numsamples);

}

void Add(float *pout, float *pin, int numsamples, float amp)
{
	do
	{
		*pout++ += *pin++ * amp;
	} while(--numsamples);
}

size_t sizeFromWaveFormat(int waveFormat) {
	switch (waveFormat) {
		case zzub::wave_buffer_type_si16:
			return 2;
		case zzub::wave_buffer_type_si24:
			return 3;
		case zzub::wave_buffer_type_f32:
		case zzub::wave_buffer_type_si32:
			return 4;
		default:
			return -1;
	}
}

// disse trenger vi for lavniv� redigering p� flere typer bitformater, waveFormat er buzz-style
// det er kanskje mulig � oppgradere copy-metodene med en interleave p� hver buffer for � gj�re konvertering mellom stereo/mono integrert
void CopyMonoToStereoEx(void* srcbuf, void* targetbuf, size_t numSamples, int waveFormat) {

	int sampleSize=sizeFromWaveFormat(waveFormat);
	char* tbl=(char*)targetbuf;
	char* tbr=(char*)targetbuf;
	tbr+=sampleSize;
	char* sb=(char*)srcbuf;

	int temp;

	for (size_t i=0; i<numSamples; i++) {
		switch (waveFormat) {
			case zzub::wave_buffer_type_si16:
				*((short*)tbr)=*((short*)tbl)=*(short*)sb;
				break;
			case zzub::wave_buffer_type_si24:
				temp=(*(int*)sb) >> 8;
				*((int*)tbr)=*((int*)tbl)=temp;
				break;
			case zzub::wave_buffer_type_f32:
			case zzub::wave_buffer_type_si32:
				temp=*(int*)sb;
				*((int*)tbr)=*((int*)tbl)=temp;
				break;
		}
		tbl+=sampleSize*2;
		tbr+=sampleSize*2;
		sb+=sampleSize;
	}
}

void CopyStereoToMonoEx(void* srcbuf, void* targetbuf, size_t numSamples, int waveFormat) {
}

// from 16 bit conversion
void Copy16To24(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep, size_t dststep, size_t srcoffset, size_t dstoffset) {
	CopySamplesT((short*)srcbuf, (S24*)targetbuf, numSamples, srcstep, dststep, srcoffset, dstoffset);
}

void Copy16ToS32(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep, size_t dststep, size_t srcoffset, size_t dstoffset) {
	CopySamplesT((short*)srcbuf, (int*)targetbuf, numSamples, srcstep, dststep, srcoffset, dstoffset);
}

void Copy16ToF32(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep, size_t dststep, size_t srcoffset, size_t dstoffset) {
	CopySamplesT((short*)srcbuf, (float*)targetbuf, numSamples, srcstep, dststep, srcoffset, dstoffset);
}


// from 32 bit floating point conversion
void CopyF32To16(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep, size_t dststep, size_t srcoffset, size_t dstoffset) {
	CopySamplesT((float*)srcbuf, (short*)targetbuf, numSamples, srcstep, dststep, srcoffset, dstoffset);
}

void CopyF32To24(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep, size_t dststep, size_t srcoffset, size_t dstoffset) {
	CopySamplesT((float*)srcbuf, (S24*)targetbuf, numSamples, srcstep, dststep, srcoffset, dstoffset);
}

void CopyF32ToS32(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep, size_t dststep, size_t srcoffset, size_t dstoffset) {
	CopySamplesT((float*)srcbuf, (int*)targetbuf, numSamples, srcstep, dststep, srcoffset, dstoffset);
}


// from 32 bit integer conversion
void CopyS32To16(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep, size_t dststep, size_t srcoffset, size_t dstoffset) {
	CopySamplesT((int*)srcbuf, (short*)targetbuf, numSamples, srcstep, dststep, srcoffset, dstoffset);
}

void CopyS32To24(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep, size_t dststep, size_t srcoffset, size_t dstoffset) {
	CopySamplesT((int*)srcbuf, (S24*)targetbuf, numSamples, srcstep, dststep, srcoffset, dstoffset);
}

void CopyS32ToF32(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep, size_t dststep, size_t srcoffset, size_t dstoffset) {
	CopySamplesT((int*)srcbuf, (float*)targetbuf, numSamples, srcstep, dststep, srcoffset, dstoffset);
}


// from 24 bit integer conversion
void Copy24To16(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep, size_t dststep, size_t srcoffset, size_t dstoffset) {
	CopySamplesT((S24*)srcbuf, (short*)targetbuf, numSamples, srcstep, dststep, srcoffset, dstoffset);
}

void Copy24ToF32(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep, size_t dststep, size_t srcoffset, size_t dstoffset) {
	CopySamplesT((S24*)srcbuf, (float*)targetbuf, numSamples, srcstep, dststep, srcoffset, dstoffset);
}

void Copy24ToS32(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep, size_t dststep, size_t srcoffset, size_t dstoffset) {
	CopySamplesT((S24*)srcbuf, (int*)targetbuf, numSamples, srcstep, dststep, srcoffset, dstoffset);
}

void Copy16(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep, size_t dststep, size_t srcoffset, size_t dstoffset) {
	CopySamplesT((short*)srcbuf, (short*)targetbuf, numSamples, srcstep, dststep, srcoffset, dstoffset);
}

void Copy24(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep, size_t dststep, size_t srcoffset, size_t dstoffset) {
	CopySamplesT((S24*)srcbuf, (S24*)targetbuf, numSamples, srcstep, dststep, srcoffset, dstoffset);
}

void CopyS32(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep, size_t dststep, size_t srcoffset, size_t dstoffset) {
	CopySamplesT((int*)srcbuf, (int*)targetbuf, numSamples, srcstep, dststep, srcoffset, dstoffset);
}

void CopyF32(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep, size_t dststep, size_t srcoffset, size_t dstoffset) {
	CopySamplesT((float*)srcbuf, (float*)targetbuf, numSamples, srcstep, dststep, srcoffset, dstoffset);
}

    //~ zzub_wave_buffer_type_si16	= 0,    // signed int 16bit
    //~ zzub_wave_buffer_type_f32	= 1,    // float 32bit
    //~ zzub_wave_buffer_type_si32	= 2,    // signed int 32bit
    //~ zzub_wave_buffer_type_si24	= 3,    // signed int 24bit

typedef void (*CopySamplesPtr)(void *, void *, size_t, size_t, size_t, size_t, size_t);

CopySamplesPtr CopySamplesMatrix[4][4] = {
  // si16 -> si16, f32, si32, si24
  {Copy16, Copy16ToF32, Copy16ToS32, Copy16To24},
  // f32 -> si16, f32, si32, si24
  {CopyF32To16, CopyF32, CopyF32ToS32, CopyF32To24},
  // si32 -> si16, f32, si32, si24
  {CopyS32To16, CopyS32ToF32, CopyS32, CopyS32To24},
  // si24 -> si16, f32, si32, si24
  {Copy24To16, Copy24ToF32, Copy24ToS32, Copy24},
};

// auto select based on waveformat
void CopySamples(void *srcbuf, void *targetbuf, size_t numSamples, int srcWaveFormat, int dstWaveFormat, size_t srcstep, size_t dststep, size_t srcoffset, size_t dstoffset) {
	CopySamplesMatrix[srcWaveFormat][dstWaveFormat](srcbuf, targetbuf, numSamples, srcstep, dststep, srcoffset, dstoffset);
}

// found the trims in one of the comments at http://www.codeproject.com/vcpp/stl/stdstringtrim.asp

std::string& trimleft( std::string& s )
{
   std::string::iterator it;

   for( it = s.begin(); it != s.end(); it++ )
      if( !isspace((unsigned char) *it ) )
         break;

   s.erase( s.begin(), it );
   return s;
}

std::string& trimright( std::string& s )
{
   std::string::difference_type dt;
   std::string::reverse_iterator it;

   for( it = s.rbegin(); it != s.rend(); it++ )
      if( !isspace((unsigned char) *it ) )
         break;

   dt = s.rend() - it;

   s.erase( s.begin() + dt, s.end() );
   return s;
}

std::string& trim( std::string& s )
{
   trimleft( s );
   trimright( s );
   return s;
}

std::string trim( const std::string& s )
{
   std::string t = s;
   return trim( t );
}


int transposeNote(int v, int delta) {
	// 1) convert to "12-base"
	// 2) transpose
	// 3) convert back to "16-base"
	int note=(v&0xF)-1;
	int oct=(v&0xF0) >> 4;

	int twelve=note+12*oct;

	twelve+=delta;

	note=(twelve%12)+1;
	oct=twelve/12;
	return (note) + (oct<<4);
}


int getNoValue(const zzub::parameter* para) {
	switch (para->type) {
		case zzub::parameter_type_switch:
			return zzub::switch_value_none;
		case zzub::parameter_type_note:
			return zzub::note_value_none;
		default:
			return para->value_none;
	}
}


bool validateParameter(int value, const zzub::parameter* p) {
	if (p->type==zzub::parameter_type_switch) return true;
	// TODO: validate note
	if (p->type==zzub::parameter_type_note) return true;

	return (value==getNoValue(p) || (value>=p->value_min && value<=p->value_max) );

}

// cross platform library loading

xp_modulehandle xp_dlopen(const char* path)
{
  return dlopen(path, RTLD_LAZY | RTLD_LOCAL);
}

void* xp_dlsym(xp_modulehandle handle, const char* symbol)
{
  return dlsym(handle, symbol);
}
	
void xp_dlclose(xp_modulehandle handle)
{
  dlclose(handle);
}

const char *xp_dlerror() {
  return dlerror();
}
