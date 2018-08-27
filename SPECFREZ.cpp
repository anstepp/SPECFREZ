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
	: _in(NULL),
	_branch(0), 
	_outbuf(NULL), 
	window(NULL), 
	TheBucket(NULL)
{
}

SPECFREZ::~SPECFREZ()
{
	delete [] _in;
	delete [] _outbuf;
	delete window;
	delete TheBucket;
	delete [] _outbuf;
	delete fftbuf;
}

int SPECFREZ::init(double p[], int n_args)
{
	_nargs = n_args;

	const float outskip = p[0];
	const float inskip = p[1];
	const float dur = p[2];
	const float inchans = p[3];
	const int _full_fft = p[4];
	float decay = p[5];
	const float _inchan = p[6];

	int winlen;
	double *wintab = (double *) getPFieldTable(7, &winlen);
	const float freq = 1.0/(float)SR;
	window = new Ooscili(SR, freq, wintab, winlen);

	int _half_fft = _full_fft / 2;

	inframes = int(dur * SR + 0.5);

	rtsetoutput(outskip, dur, this);
	rtsetinput(inskip, this);

	return nSamps();
}

int SPECFREZ::configure()
{

	_full_fft = 8;
	_half_fft = _full_fft / 2;

	_in = new float [RTBUFSAMPS * inputChannels()];
	outframes = fmax(_full_fft, RTBUFSAMPS);

    if (_in == NULL)
        return -1;

	TheFFT = new Offt(_full_fft);
	fftbuf = TheFFT->getbuf();

	TheBucket = new Obucket(_full_fft, Bucket_Wrapper, (void *) this);
	if (TheBucket == NULL)
		return -1;

	fft_index = 0;
	out_index = 0;

	float _outbuf[outframes];

	return 0;

}

void SPECFREZ::FFT_increment()
{
	printf("%i\n", fft_index);
	if (++fft_index == outframes){
		fft_index = 0;
	}
}

void SPECFREZ::Sample_increment()
{
	printf("%i\n", out_index);
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
    for(int i = 0; i < _full_fft - 1; i++){
    	_lastfftbuf[i] = 0.0f;
    }

	TheFFT->r2c();

	for(int i = 0; i < _half_fft; i++){
		fftbuf[i] = buf[i]; //* window->next(i);
		printf("%f\n", fftbuf[i]);
	}
	for(int i = _half_fft; i < _full_fft; i++){
		_lastfftbuf[i] = fftbuf[i] = 0.0f;
	}

	for(int i = 0; i < _full_fft; i++){
		printf("%f\n", fftbuf[i]);
	}

	for(int i = 0; i < _full_fft; i++)
	{

		if (i % 2 == 0)
		{
			if(fftbuf[i] < _lastfftbuf[i]){
				_lastfftbuf[i] = _lastfftbuf[i] * decay;
				fftbuf[i] = _lastfftbuf[i];
				FFT_increment();
				printf("%s\n", "sustain");
				printf("%f\n", fftbuf[i]);
			}
			else{
				fftbuf[i] = fftbuf[i] * decay;
				_lastfftbuf[i] = fftbuf[i];
				FFT_increment();
				printf("%s\n", "decay");
				printf("%f\n", fftbuf[i]);
			}
		}
		else
		{
			//playfft[i] = (float) rand()/RAND_MAX;
			fftbuf[i] = 0.0f;
			printf("%s\n", "phase");
			printf("%f\n", fftbuf[i]);
		}
	}

	for (int i = 0; i < _half_fft; i++){
		printf("%f\n", fftbuf[i]);
	}

	TheFFT->c2r();

	for (int i = 0; i < len; i++){
		_outbuf[out_index] = fftbuf[i];
	}

}

void SPECFREZ::doupdate()
{
}


int SPECFREZ::run()
{
	float out[1];

	const int nframes = framesToRun();
	const int inchans = inputChannels();
	const int outchans = outputChannels();

	const int samps = framesToRun() * inchans;
	const bool input_avail = (currentFrame() < nframes);

	if (currentFrame() < inframes){
		rtgetin(_in, this, samps);
		for (int i = _inchan; i < nframes; i++){
			float the_current_sample = _in[fft_index];
			printf("%f\n", the_current_sample);
			TheBucket->drop(the_current_sample);
		}
	}

	for(int i = 0; i < nframes; i++)
	{
		out[0] = _outbuf[out_index];
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