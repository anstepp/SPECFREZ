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
#include <assert.h>

float TWO_PI = M_PI * 2;

inline int imax(int x, int y) { return x > y ? x : y; }

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
    //printf("%i %f\n", window->getlength(), freq);

	inframes = int(dur * SR + 0.5);

    int feedbackwinlen;
    double *feedbacktab = (double *) getPFieldTable(6, &feedbackwinlen);
    const float feedbackfreq = 1;
    decaytable = new Ooscili(_half_fft, feedbackfreq, feedbacktab, feedbackwinlen);

    _pan = p[9];

    fft_index = 0;
    out_index = 0;

	rtsetoutput(outskip, dur, this);
	rtsetinput(inskip, this);

	return nSamps();
}

int SPECFREZ::configure()
{

	_in = new float [RTBUFSAMPS * inputChannels()];
	outframes = imax(_half_fft, RTBUFSAMPS) * 2;
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
    for(int i = 0; i < _full_fft; i++){
        _ola[i] = 0.0f;
    }

	_outbuf = new float [outframes];
    for(int i = 0; i < outframes; i++){
        _outbuf[i] = 0.0f;
    }

     _drybuf = new float [outframes];
    for(int i = 0; i < outframes; i++){
        _outbuf[i] = 0.0f;
    }

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
        //assert(fft_index != out_index);
	}
}

void SPECFREZ::Sample_increment()
{

	if (++out_index == outframes){
		out_index = 0;
        //assert(out_index != fft_index);
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
        float window_val = 0.5 * (1 - (cos((TWO_PI*i)/(len - 1))));
        printf("%i in buf %f\n", i, buf[i]);
        fftbuf[i] = buf[i] * window_val;
    }

    // for (int i = 0; i < len; i++){
    //     fftbuf[i] = buf[i];
    // }

    for (int i = len; i < _full_fft; i++){
        fftbuf[i] = 0.0f;
    }

	TheFFT->r2c();

	//compute the polar coordinates for each bin
	//evaulate if the bin amplitude is greater than the previous fft
	//if so, keep as current bin, add decay
	//if not, change to last bin value, add decay
	//randomize phase (0-2pi)
	//convert back to Cartesian
	//set _lastfftbuf values to compare next fft to

	for(int i = 2, j = 3; i < _full_fft; i += 2, j += 2){
        _decay = decaytable->next(i/2);
        //printf("%i %f\n", i/2, _decay);
		float r_sq = fftbuf[i] * fftbuf[i];
		float i_sq = fftbuf[j] * fftbuf[j];
		float fft_mag = sqrt(r_sq + i_sq);
		float fft_phi = atan2(fftbuf[j], fftbuf[i]);
		float last_r_sq = _lastfftbuf[i] * _lastfftbuf[i];
		float last_i_sq = _lastfftbuf[j] * _lastfftbuf[j];
		float last_fft_mag = sqrt(last_r_sq + last_i_sq);
        float last_fft_phi = atan2(_lastfftbuf[j], _lastfftbuf[i]);
		if(fft_mag > last_fft_mag){
			//fft_mag;// *= _decay;
			//float phase = ((float)rand()/(float)RAND_MAX) * TWO_PI;
			float phase = fft_phi;
            fftbuf[i] = fft_mag * cos(phase);
			fftbuf[j] = fft_mag * sin(phase);
			_lastfftbuf[i] = fftbuf[i];
			_lastfftbuf[j] = fftbuf[j];
		}
		else{
			//fft_mag = last_fft_mag;// * _decay;
			//float phase = ((float)rand()/(float)RAND_MAX) * TWO_PI;
			float phase = fft_phi;
            fftbuf[i] = fft_mag * cos(phase);
			fftbuf[j] = fft_mag * sin(phase);
			_lastfftbuf[i] = fftbuf[i];
			_lastfftbuf[j] = fftbuf[j];
		}
        fftbuf[i] = fft_mag * cos(fft_phi);
        fftbuf[j] = fft_mag * sin(fft_phi);
	}

	TheFFT->c2r();

    for(int i = 0, j = len; i < len; i++, j++){
        _ola[i] += fftbuf[i];
        _outbuf[fft_index] = _ola[i];
        _drybuf[fft_index] = buf[i];
        printf("fft index %i _drybuf %f\n", fft_index, buf[i]);
        FFT_increment();
        _ola[i] = fftbuf[j];
    }
}

void SPECFREZ::doupdate()
{
}


int SPECFREZ::run()
{

	const int nframes = framesToRun();
	const int inchans = inputChannels();
	const int outchans = outputChannels();

    const int insamps = nframes * inchans;

    rtgetin(_in, this, insamps);

    printf("%i %i %i\n", nframes, inchans, outchans);

	for(int i = 0; i < nframes; i++)
	{

        float insig;
        insig = _in[(i * inchans) + _inchan];

        TheBucket->drop(insig);
 
        float out[2];
        //printf("%i putbuf %f\n", i, _outbuf[out_index]);
		out[0] = _outbuf[out_index] * _amp;
		out[1] = _drybuf[out_index] * _amp;
        Sample_increment();
        printf("out index %i dry signal %f\n", out_index, out[0]);
		rtaddout(out);
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