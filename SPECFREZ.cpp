/* SPECFREZ

A spectral freezing instrument.

p0 = start
p1 = inskip
p2 = duration
p3 = amplitude
p4 = pre-fft amplitude
p5 = inchans
p6 = fft size
p7 = delay table
p8 = delay bin shift
p9 = minimum bin magnitude
p10 = window
p11 = input channel
p12 = pan 

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
    float *_drybuf = NULL;
}

SPECFREZ::~SPECFREZ()
{
	delete [] _in;
	delete [] _outbuf;
	delete [] window;
	delete TheBucket;
	delete TheFFT;
	delete [] _ola;
    delete [] _drybuf;
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
    _half_fft = _full_fft / 2;
    _overlap = p[7];
	_decay_shift = p[9];
	_threshold = p[10];
	_inchan = p[12];

	int winlen;
	double *wintab = (double *) getPFieldTable(10, &winlen);
	const float freq = 1.0/(float)_half_fft;
	window = new Ooscili(SR, freq, wintab, winlen);
    //printf("%i %f\n", window->getlength(), freq);

	inframes = int(dur * SR + 0.5);

    int feedbackwinlen;
    double *feedbacktab = (double *) getPFieldTable(8, &feedbackwinlen);
    const float feedbackfreq = 1;
    decaytable = new Ooscili(_half_fft, feedbackfreq, feedbacktab, feedbackwinlen);

    _pan = p[13];

    _decimation = int(_full_fft / _overlap);
    _window_len_minus_decimation = _full_fft - _decimation;
    int _latency = _window_len_minus_decimation;
    const float latency_dur = _latency / SR;

	rtsetoutput(outskip, dur, this);
	rtsetinput(inskip, this);

	return nSamps();
}

int SPECFREZ::configure()
{

	_in = new float [RTBUFSAMPS * inputChannels()];
	outframes = imax(_full_fft, RTBUFSAMPS) * 2;

    fft_index = outframes - _half_fft;
    out_index = 0;

    if (_in == NULL)
        return -1;

    _outbuf = new float [outframes];
    for(int i = 0; i < outframes; i++){
        _outbuf[i] = 0.0f;
    }

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

     _drybuf = new float [outframes];
    for(int i = 0; i < outframes; i++){
        _drybuf[i] = 0.0f;
    }

	_lastfftbuf = new float [_full_fft];
	for (int i = 0; i < _full_fft; i++){
		_lastfftbuf[i] = 0.0f;
	}

	inner_out = new float [_full_fft];
	inner_in = new float [_full_fft];

	// window using hann window
	_window = new float [_full_fft];
    for(int i = 0; i < _full_fft; i++){
       	_window[i] = 0.5 * (1 - (cos((TWO_PI*i)/(_full_fft-1))));
    }

	return 0;

}

inline void SPECFREZ::FFT_increment()
{
	if (++fft_index == outframes){
		fft_index = 0;
	}
    //printf("fft_index:%i, out_index:%i, outframes:%i\n", fft_index, out_index, outframes);
    assert(fft_index != out_index);
}

inline void SPECFREZ::Sample_increment()
{
	if (++out_index == outframes){
		out_index = 0;
	}
    //printf("fft_index:%i, out_index:%i, outframes:%i\n", fft_index, out_index, outframes);
    assert(out_index != fft_index);
}

void SPECFREZ::Bucket_Wrapper(const float buf[], const int len, void *obj)
{
	SPECFREZ *myself = (SPECFREZ *) obj;
	myself->mangle_samps(buf, len);
}

void SPECFREZ::window_input(const float buf[])
{
	//rotate previous TheBucket samples to beginning of fft
	for (int i = 0; i < _window_len_minus_decimation; i++){
		inner_in[i] = inner_in[i + _decimation];
	}

	//fill second half of buffer with current TheBucket samps
	for (int i = _window_len_minus_decimation, j = 0; i < _full_fft; i++, j++){
		inner_in[i] = buf[j] * _inamp;
	}

	//window before taking fft
	for (int i = 0; i < _full_fft; i++)
		fftbuf[i] = 0.0f;
	int j = currentFrame() % _full_fft;
	for (int i = 0; i < _full_fft; i++){
		fftbuf[j] += _window[i] * inner_in[i];
		if (++j == _full_fft){
			j = 0;
		}
	}

}

void SPECFREZ::window_output()
{

	//get current location in fft with mod
	//iterate through and add from location on in fftbuf to
	//our internal buffer (inner_out)
	int j = currentFrame() % _full_fft;
	for(int i = 0; i < _full_fft; i++){
		inner_out[i] += fftbuf[j] * _window[i];
		if(++j == _full_fft){
			j = 0;
		}
	}

	//write to output buffer at current location in out_frames
	//increment counter for location in out_frames
	for(int i = 0; i < _half_fft; i++){
		_outbuf[fft_index] = inner_out[i];
		FFT_increment();
	}

	//rotate and zero out inner out buffer for overlap add
	for(int i = 0; i < _window_len_minus_decimation; i++){
		inner_out[i] = inner_out[i + _decimation];
	}
	for(int i = _window_len_minus_decimation; i < _full_fft; i++){
		inner_out[i] = 0.0f;
	}

}

void SPECFREZ::mangle_samps(const float *buf, const int len)
{

    // window

    window_input(buf);

	TheFFT->r2c();

	//compute the polar coordinates for each bin
	//evaulate if the bin amplitude is greater than the previous fft
	//if so, keep as current bin, add decay
	//if not, change to last bin value, add decay
	//randomize phase (0-2pi)
	//convert back to Cartesian
	//set _lastfftbuf values to compare next fft to

	for(int i = 2, j = 3, k = 0; i < _full_fft; i += 2, j += 2, k++){
		int _decay_shift_bin = static_cast<int>(_decay_shift);
        _decay = decaytable->next((k + _decay_shift_bin) % _full_fft);
        //printf("%i %f\n", i/2, _decay);
		float r_sq = fftbuf[i] * fftbuf[i];
		float i_sq = fftbuf[j] * fftbuf[j];
		float fft_mag = sqrt(r_sq + i_sq);
		float fft_phi = atan2(fftbuf[j], fftbuf[i]);
		float last_r_sq = _lastfftbuf[i] * _lastfftbuf[i];
		float last_i_sq = _lastfftbuf[j] * _lastfftbuf[j];
		float last_fft_mag = sqrt(last_r_sq + last_i_sq);
        float last_fft_phi = atan2(_lastfftbuf[j], _lastfftbuf[i]);
		if(fft_mag > last_fft_mag && fft_mag > _threshold){
			fft_mag *= _decay;
			float phase = ((float)rand()/(float)RAND_MAX) * TWO_PI;
			//float phase = fft_phi;
            fftbuf[i] = fft_mag * cos(phase);
			fftbuf[j] = fft_mag * sin(phase);
			_lastfftbuf[i] = fftbuf[i];
			_lastfftbuf[j] = fftbuf[j];
		}
		else if (fft_mag < last_fft_mag && fft_mag > _threshold){
			fft_mag = last_fft_mag * _decay;
			float phase = ((float)rand()/(float)RAND_MAX) * TWO_PI;
			//float phase = fft_phi;
            fftbuf[i] = fft_mag * cos(phase);
			fftbuf[j] = fft_mag * sin(phase);
			_lastfftbuf[i] = fftbuf[i];
			_lastfftbuf[j] = fftbuf[j];
		}
		else if (fft_mag < last_fft_mag && last_fft_mag > _threshold){
			fft_mag = last_fft_mag * _decay;
			float phase = ((float)rand()/(float)RAND_MAX) * TWO_PI;
			//float phase = fft_phi;
            fftbuf[i] = fft_mag * cos(phase);
			fftbuf[j] = fft_mag * sin(phase);
			_lastfftbuf[i] = fftbuf[i];
			_lastfftbuf[j] = fftbuf[j];
		}
		else {
			float phase = ((float)rand()/(float)RAND_MAX) * TWO_PI;
			fftbuf[i] = 0;
			fftbuf[j] = fft_mag * sin(phase);
		}
	}

	TheFFT->c2r();

    window_output();
}

void SPECFREZ::doupdate()
{

	double p[14];
	update(p, 14, 1 << 3 | 1 << 4 | 1 << 9 | 1 << 10 | 1 << 13);
	_amp = p[3];
	_inamp = p[4];
	_decay_shift = p[9];
	_threshold = p[10];
	_pan = p[13];

}


int SPECFREZ::run()
{

	const int nframes = framesToRun();
	const int inchans = inputChannels();
	const int outchans = outputChannels();

    const int insamps = nframes * inchans;

    rtgetin(_in, this, insamps);

	for(int i = 0; i < nframes; i++)
	{

		if (--_branch <= 0) {
			doupdate();
			_branch = getSkip();
		}

        float insig;
        insig = _in[(i * inchans) + _inchan];

        TheBucket->drop(insig);
 
        float outsig = _outbuf[out_index];
        outsig *= _amp;

        float out[2];
		out[0] = outsig * (1.0f - _pan);
		out[1] = outsig * _pan;
        Sample_increment();
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