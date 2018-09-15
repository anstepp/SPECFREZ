#include <Instrument.h>      // the base class for this instrument

class SPECFREZ : public Instrument {

public:
	SPECFREZ();
	virtual ~SPECFREZ();
	virtual int init(double *, int);
	virtual int configure();
	virtual int run();

private:

	void mangle_samps(const float* buf, const int len);
	static void Bucket_Wrapper(const float buf[], const int fft_size, void *obj);
	void FFT_increment();
	void Sample_increment();
	void doupdate();

	int _full_fft, _half_fft;

	float *_in, *_outbuf, wintab, *_lastfftbuf, *_ola, *_drybuf;
	int _nargs, _inchan, _branch, outframes, fft_index, out_index, inframes;
	float _amp, _pan, _inamp;
	float _decay;

	Offt *TheFFT;
	float *fftbuf;

	Obucket *TheBucket;
	Ooscili *window;
	Ooscili *decaytable;

};