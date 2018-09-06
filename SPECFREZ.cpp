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
#include <math.h>

float TWO_PI = M_PI * 2;

SPECFREZ::SPECFREZ()
{
	float *_in = NULL;
	float *_outbuf = NULL; 
	float *window = NULL;
	Obucket *TheBucket = NULL;
	Offt *TheFFT = NULL;
	float *_ola = NULL;
}

SPECFREZ::~SPECFREZ()
{
	delete [] _in;
	delete [] _outbuf;
	delete [] window;
	delete TheBucket;
	delete TheFFT;
	delete [] _ola;
}

int SPECFREZ::init(double p[], int n_args)
{
	_nargs = n_args;

	const float outskip = p[0];
	const float inskip = p[1];
	const float dur = p[2];
	_amp = p[3];
	const float inchans = p[4];
	_full_fft = p[5];
    _half_fft = _full_fft / 2;
	//_decay = p[7];
	_inchan = p[7];

	int winlen;
	double *wintab = (double *) getPFieldTable(8, &winlen);
	const float freq = 1.0/(float)_half_fft;
	window = new Ooscili(SR, freq, wintab, winlen);
    printf("%i %f\n", window->getlength(), freq);

	inframes = int(dur * SR);

    int feedbackwinlen;
    double *feedbacktab = (double *) getPFieldTable(6, &feedbackwinlen);
    const float feedbackfreq = 1.0/_half_fft;
    decaytable = new Ooscili(SR, freq, feedbacktab, feedbackwinlen);

    _pan = p[9];

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

	_ola = new float [_full_fft];
	_outbuf = new float [outframes];

	_lastfftbuf = new float [_full_fft];
	for (int i = 0; i < _full_fft; i++){
		_lastfftbuf[i] = 0.0f;
	}

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
	
    // window

    for(int i = 0; i < len; i++){
        float window_val = 0.5 * (1 - (cos((2*M_PI*i)/(len - 1))));
        fftbuf[i] = buf[i] * window_val;
    }

	// zero-padding

	for(int i = len; i < _full_fft; i++){
		fftbuf[i] = 0.0f;
	}

	TheFFT->r2c();

	fftbuf[0] = 0.0; //dc has no imaginary value
	fftbuf[1] = 0.0; //nyquist has no imaginary value


	//compute the polar coordinates for each bin
	//evaulate if the bin amplitude is greater than the previous fft
	//if so, keep as current bin, add decay
	//if not, change to last bin value, add decay
	//randomize phase (0-360)
	//convert back to Cartesian
	//set _lastfftbuf values to compare next fft to

	for(int i = 2, j = 3; i < _half_fft; i += 2, j += 2){
        _decay = decaytable->next(i/2);
        //printf("%f, %f\n", fftbuf[i], fftbuf[j]);
		float r_sq = fftbuf[i] * fftbuf[i];
		float i_sq = fftbuf[j] * fftbuf[j];
		float fft_mag = sqrt(r_sq + i_sq);
		float fft_phi = atan2(fftbuf[j], fftbuf[i]);
		float last_r_sq = _lastfftbuf[i] * _lastfftbuf[i];
		float last_i_sq = _lastfftbuf[j] * _lastfftbuf[j];
		float last_fft_mag = sqrt(last_r_sq + last_i_sq);
		float last_fft_phi = atan2(_lastfftbuf[j], _lastfftbuf[i]);
		if(fft_mag > last_fft_mag){
			fft_mag *= _decay;
			float phase = ((float)rand()/(float)RAND_MAX) * TWO_PI;
            //phase = fft_phi;
            //printf("%f\n", phase);
			fftbuf[i] = fft_mag * cos(phase);
			fftbuf[j] = fft_mag * sin(phase);
            //printf("decay %f, %f\n", fftbuf[i], fftbuf[j]);
			_lastfftbuf[i] = fftbuf[i];
			_lastfftbuf[j] = fftbuf[j];
		}
		else{
			fft_mag = last_fft_mag * _decay;
			float phase = ((float)rand()/(float)RAND_MAX) * TWO_PI;
            //phase = last_fft_phi;
            //printf("%f\n", phase);
			fftbuf[i] = fft_mag * cos(phase);
			fftbuf[j] = fft_mag * sin(phase);
            //printf("sustain %f, %f\n", fftbuf[i], fftbuf[j]);
			_lastfftbuf[i] = fftbuf[i];
			_lastfftbuf[j] = fftbuf[j];
		}
	}

	TheFFT->c2r();

	//overlap add

	for(int i = 0, j = len; i < len; i++, j++){
		_ola[i] += fftbuf[i];
		_outbuf[fft_index] = _ola[i];
		FFT_increment();
		_ola[i] = fftbuf[j];
	}
}

void SPECFREZ::doupdate()
{
}


int SPECFREZ::run()
{
	float out[2];

	const int nframes = framesToRun();
	const int inchans = inputChannels();
	const int outchans = outputChannels();

	const int samps = framesToRun() * inchans;
	const bool input_avail = (currentFrame() < nframes);

	if (currentFrame() < inframes){
		rtgetin(_in, this, samps);
		for (int i = 0; i < samps; i++){
		}
		for (int i = _inchan; i < samps; i += inchans){
			float the_current_sample = _in[i];
			TheBucket->drop(the_current_sample);
		}
	}

	for(int i = 0; i < nframes; i++)
	{
		out[0] = _outbuf[out_index] * _amp * _pan;
		out[1] = _outbuf[out_index] * _amp * (1.0 - _pan);
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