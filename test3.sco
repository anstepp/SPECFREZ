rtsetparams(44100, 2)
load("./libSPECFREZ.so")

set_option("clobber=on")

rtinput("steppMusings.wav")
//rtinput("sine440.wav")

rtoutput("SmearsForTears.wav")

inskip = 0
duration = DUR()
ringdur = 10
amp = 1
inamp = 1
inchans = 1
fft = 8192
overlap = 2

// decay = maketable("curve", "nonorm", fft, 0,0,-2, 100,0.99999999,-2, 
// 	110,0,-2, 
// 	120,0.999,-2, 
// 	140,0,-2, 
// 	160,0.999,-2, 
// 	180,0,-2, 
// 	200,0.999,-2, 
// 	1000,0.5)

//decay = maketable("curve", "nonorm", 1000, 0,.95,2, 100,.99,2, 200,0.9,2, 400,.99,2, 1000,0.8) 

decay = maketable("line", "nonorm", 1000, 0,0, 1,0)

inchan = 0

decay_shift = 0

threshold = 0.99

window = maketable("window", fft, 1)

pan = 0.5


SPECFREZ(0, inskip, duration, ringdur, amp, inamp, inchans, fft, overlap, decay, decay_shift, threshold, inchan, window, pan)