rtsetparams(44100, 2)
load("./libSPECFREZ.so")

set_option("clobber = on")

//rtinput("sine.wav")
rtinput("../../../snd/nucular.wav")
rtoutput("specfrez.wav")

bus_config("in 0-1", "out 0-1")

start = 0
inskip = 0
duration = DUR() * 10
amp = 0.5
inchans = 1
fft = 2048
decay = maketable("random", "nonorm", 1000, 'high', .9, .99)

//decay = maketable("curve", "nonorm", 1000, 0,.95,2, 100,.99,2, 200,0,2, 400,.99,2, 1000,0) 
inchan = 0

decay_mult = maketable("curve", "nonorm", 1000, 0,1,2, 100,0.5,10, 1000,1)
//decay_mult = 1
window = maketable("window", fft, 1)

SPECFREZ(start, inskip, duration, amp, inchans, fft, decay, decay_mult, inchan, window, 0.5)