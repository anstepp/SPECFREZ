rtsetparams(44100, 2)
load("./libSPECFREZ.so")

set_option("clobber = on")

rtinput("../../../snd/loocher.aiff")

rtoutput("specfrez.wav")

bus_config("in 0-1", "out 0-1")

start = 0
inskip = 0
duration = DUR() + 10
amp = 0.25
inamp = 1
inchans = 1
fft = 1024
overlap = 2

decay = maketable("curve", "nonorm", fft, 0,0,10, fft/8,0.999,10, fft/7,0,10, fft/6,0.999,10, fft/5,0,10, fft/4,0.999,10, fft/3,0,10, fft/2,0.999,10, fft,0)

//decay = maketable("curve", "nonorm", 1000, 0,.95,2, 100,.99,2, 200,0.9,2, 400,.99,2, 1000,0.8) 
inchan = 0

decay_shift = maketable("curve", "nonorm", 1000, 0,1,2, 1000,fft)
//decay_shift = 0

threshold = 0.9

window = maketable("window", fft, 1)

pan = maketable("line", "nonorm", 1000, 0,0, 1000,1)

SPECFREZ(start, inskip, duration, amp, inamp, inchans, fft, overlap, decay, decay_shift, threshold, inchan, window, pan)