#include <Instrument.h>      // the base class for this instrument
#include <assert.h>

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
	void increment_out_read_index();
	void increment_out_write_index();
	void window_input(const float buf[]);
	void window_output();
	void doupdate();
	int make_windows();

	int _full_fft, _half_fft;

	float *_in, *_outbuf, wintab, *_lastfftbuf, *_ola, *_drybuf;
	float *inner_in, *inner_out, *_window, *_anal_window, *_synth_window;
	int _nargs, _inchan, _branch, outframes, inframes;
	int _decimation, _overlap, _window_len, _window_len_minus_decimation;
	float _amp, _pan, _inamp, _decay_shift, _threshold;
	float _decay;
	int _input_end_frame, out_read_index, out_write_index;

	Offt *TheFFT;
	float *fftbuf;

	Obucket *TheBucket;
	Ooscili *window;
	Ooscili *decaytable;

};

inline void SPECFREZ::increment_out_write_index()
{
	if (++out_write_index == outframes){
		out_write_index = 0;
	}
	//printf("out write index: %i out read index: %i\n", out_write_index, out_read_index);
    assert(out_write_index != out_read_index);
}

inline void SPECFREZ::increment_out_read_index()
{
	if (++out_read_index == outframes){

		out_read_index = 0;
	}
	//printf("out read index: %i out write index: %i\n", out_read_index, out_write_index);
    assert(out_read_index != out_write_index);
}
