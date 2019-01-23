rtsetparams(44100, 2)
load("./libSPECFREZ.so")
load("GVERB")

set_option("clobber = on", "play = off")
//set_option("clobber = on")
//rtinput("sine440.wav")
rtinput("ambikaFatherNR.wav")
//rtinput("../../../snd/nucular.wav")

rtoutput("specfrez.wav")

bus_config("SPECFREZ", "in 0-1", "aux 0-1 out")
bus_config("GVERB", "aux 0-1 in", "out 0-1")

start = 0
inskip = 0
duration = DUR() + 100
amp = 4
inamp = 3
inchans = 1
fft = 2048
overlap = 2

//decay = maketable("curve", "nonorm", 1000, 0,.95,2, 100,.99,2, 200,0.9,2, 400,.99,2, 1000,0.8) 

inchan = 0

decay_shift = maketable("curve", "nonorm", 1000, 0,1,2, 1000,fft)

window = maketable("window", fft, 1)

pan = 0.5

max_dur = 260

frezamp = 0

increment = 1/(max_dur * 0.999 / DUR())

low = 0.0
threshold = 1

verbamp = 1
room = 300
verb = 2
damp = 1
bw = 0.5
dry = -90
er = -40
tail = -5
ringdown = 10

reset(1000)

while(start < max_dur){
	bus_config("SPECFREZ", "in 0-1", "aux 0-1 out")
	decay = maketable("random", "nonorm", 1000, 'high', low, .9999)
	SPECFREZ(start, inskip, duration, amp, inamp, inchans, fft, overlap, decay, decay_shift, threshold, inchan, window, pan)
	GVERB(start, 0, duration, verbamp, room, verb, damp, bw, dry, er, tail, ringdown)
	bus_config("SPECFREZ", "in 0-1", "out 0-1")
	frezamp += increment * 1.5
	decay_shift = maketable("curve", "nonorm", 1000, 0,0,2, 1000, trand(0,fft))
	SPECFREZ(start, inskip, duration, frezamp, inamp, inchans, fft, overlap, decay, decay_shift, threshold, inchan, window, pan)
	low += increment
	threshold -= increment
	room -= increment * 30
	dry += increment * 8
	tail -= increment * 10
	damp -= increment
	start += irand(DUR(), DUR() + 1)
}