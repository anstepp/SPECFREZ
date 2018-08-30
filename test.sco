rtsetparams(44100, 2)
load("./libSPECFREZ.so")

set_option("clobber = on")

rtinput("../../../snd/nucular.wav")
rtoutput("specfrez.wav")

bus_config("in 0-1", "out 0-1")

window = maketable("window", 2000, 1)

start = 0
inskip = 0
duration = DUR()
amp = 5000
inamp = 1
inchans = 1
fft = 1024
decay = .9
inchan = 0

SPECFREZ(start, inskip, duration, amp, inamp, inchans, fft, decay, inchan, window)