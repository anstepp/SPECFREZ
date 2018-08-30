rtsetparams(44100, 2)
load("./libSPECFREZ.so")

rtinput("../../../snd/nucular.wav")

bus_config("in 0-1", "out 0-1")

window = maketable("window", 2000, 1)

start = 0
inskip = 0
duration = DUR()
amp = .25
inamp = 5
inchans = 1
fft = 1024
decay = .9
inchan = 0

SPECFREZ(start, inskip, duration, amp, inamp, inchans, fft, decay, inchan, window)