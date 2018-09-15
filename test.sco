rtsetparams(44100, 2)
load("./libSPECFREZ.so")

set_option("clobber = on")

rtinput("lowsine.wav")
rtoutput("specfrez.wav")

bus_config("in 0-1", "out 0-1")

start = 0
inskip = 0
duration = 4
amp = 1
inchans = 1
fft = 4096
decay = maketable("random", "nonorm", 1000, 'high', .8, .99)

//decay = maketable("line", "nonorm", 1000, 0,.9, 1000,0) 
inchan = 0

window = maketable("window", fft, 1)

SPECFREZ(start, inskip, duration, amp, inchans, fft, decay, inchan, window, 0.5)