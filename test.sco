rtsetparams(44100, 2)
load("./libSPECFREZ.so")

rtinput("../../../snd/nucular.wav")

bus_config("in 0-1", "out 0-1")

window = maketable("window", 2000, 1)

start = 0
inskip = 0
duration = 100
inchans = 1
fft = 256
decay = .9
inchan = 0

SPECFREZ(start, inskip, duration, inchans, fft, decay, inchan, window)