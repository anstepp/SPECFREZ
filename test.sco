rtsetparams(44100, 2)
load("./libSPECFREZ.so")

set_option("clobber = on")

rtinput("../../../snd/nucular.wav")
rtoutput("specfrez.wav")

bus_config("in 0-1", "out 0-1")

start = 0
inskip = 0
duration = DUR()
amp = 1
inchans = 1
fft = 4096
decay = maketable("curve", 1000, 0,.8,2, 1000,.5)
inchan = 0

window = maketable("window", fft, 1)

SPECFREZ(start, inskip, duration, amp, inchans, fft, decay, inchan, window, 0.5)