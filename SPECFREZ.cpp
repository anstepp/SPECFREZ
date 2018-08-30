/* SPECFREZ

aaron stepp

*/
#include <stdio.h>
#include <stdlib.h>
#include <ugens.h>
#include <Ougens.h>
#include "SPECFREZ.h"
#include <rt.h>
#include <rtdefs.h>

SPECFREZ::SPECFREZ()
{
	float *_in = NULL;
	float *_outbuf = NULL; 
	float *window = NULL;
	Obucket *TheBucket = NULL;
	int _full_fft = 0;
	int _half_fft = 0;
	Offt *TheFFT = NULL;
	float _amp = 0;
	float _decay = 0;
	int _inchan = 0;
	int _inamp = 0;
}

SPECFREZ::~SPECFREZ()
{
	delete [] _in;
	delete [] _outbuf;
	delete window;
	delete TheBucket;
	delete TheFFT;
}

int SPECFREZ::init(double p[], int n_args)
{
	_nargs = n_args;

	const float outskip = p[0];
	const float inskip = p[1];
	const float dur = p[2];
	_amp = p[3];
	_inamp = p[4];
	const float inchans = p[5];
	_full_fft = p[6];
	_decay = p[7];
	_inchan = p[8];

	int winlen;
	double *wintab = (double *) getPFieldTable(9, &winlen);
	const float freq = 1.0/(float)SR;
	window = new Ooscili(SR, freq, wintab, winlen);

	_half_fft = _full_fft / 2;

	inframes = int(dur * SR + 0.5);

	rtsetoutput(outskip, dur, this);
	rtsetinput(inskip, this);

	return nSamps();
}

int SPECFREZ::configure()
{

	_in = new float [RTBUFSAMPS * inputChannels()];
	outframes = fmax(_full_fft, RTBUFSAMPS);
    if (_in == NULL)
        return -1;

	TheFFT = new Offt(_full_fft);
	fftbuf = TheFFT->getbuf();
	if (TheFFT == NULL)
		return -1;

	TheBucket = new Obucket(_half_fft, Bucket_Wrapper, (void *) this);
	if (TheBucket == NULL)
		return -1;

	fft_index = 0;
	out_index = 0;

	_outbuf = new float [outframes];

	return 0;

}

void SPECFREZ::FFT_increment()
{

	if (++fft_index == outframes){
		fft_index = 0;
	}
}

void SPECFREZ::Sample_increment()
{

	if (++out_index == outframes){
		out_index = 0;
	}
}

void SPECFREZ::Bucket_Wrapper(const float buf[], const int len, void *obj)
{
	SPECFREZ *myself = (SPECFREZ *) obj;
	myself->mangle_samps(buf, len);
}

void SPECFREZ::mangle_samps(const float *buf, const int len)
{

	float _lastfftbuf[_full_fft];

	for(int i = 0; i < _half_fft; i++){
		_lastfftbuf[i] = fftbuf[i] = buf[i] * window->next(i) * _inamp;
		printf("%f windowed\n", fftbuf[i]);
	}
	for(int i = _half_fft; i < _full_fft; i++){
		_lastfftbuf[i] = fftbuf[i] = 0.0f;
	}

	TheFFT->r2c();

	for(int i = 0; i < _full_fft; i++)
	{

		if (i % 2 == 0)
		{
			if(fftbuf[i] < _lastfftbuf[i]){
				_lastfftbuf[i] = _lastfftbuf[i] * _decay;
				fftbuf[i] = _lastfftbuf[i];
				FFT_increment();
				//printf("%f sustain\n", fftbuf[i]);
			}
			else{
				fftbuf[i] = fftbuf[i] * _decay;
				_lastfftbuf[i] = fftbuf[i];
				FFT_increment();
				//printf("%f decay\n", fftbuf[i]);
			}
		}
		else
		{
			fftbuf[i] = (float) rand()/RAND_MAX;
			//printf("%f phase\n", fftbuf[i]);
		}
	}


	TheFFT->c2r();

	for(int i = 0; i < _half_fft; i++){
		_outbuf[i] = _outbuf[i] + fftbuf[i];
	}
}

void SPECFREZ::doupdate()
{
}


int SPECFREZ::run()
{
	float out[0];

	const int nframes = framesToRun();
	const int inchans = inputChannels();
	const int outchans = outputChannels();

	const int samps = framesToRun() * inchans;
	const bool input_avail = (currentFrame() < nframes);

	if (currentFrame() < inframes){
		rtgetin(_in, this, samps);
		for (int i = 0; i < samps; i++){
			//printf("%f in\n", _in[i]);
		}
		for (int i = _inchan; i < samps; i += inchans){
			float the_current_sample = _in[i];
			//printf("%f Current Sample\n", the_current_sample);
			TheBucket->drop(the_current_sample);
		}
	}

	for(int i = 0; i < nframes; i++)
	{
		out[0] = fftbuf[out_index] * _amp;
		rtaddout(out);
		Sample_increment();
		increment();
	}

	return framesToRun();
}

Instrument *makeSPECFREZ()
{
	SPECFREZ *inst = new SPECFREZ();
	inst->set_bus_config("SPECFREZ");

	return inst;
}

void rtprofile()
{
	RT_INTRO("SPECFREZ", makeSPECFREZ);
}