<CsoundSynthesizer>
<CsOptions>
-o dac
--opcode-lib=libLeapIntoCsound.dylib
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 2048
0dbfs = 1
nchnls = 2 

; IMPORTANT!!! You must have both the offical Leap binary and the LeapIntoCsound binary in the path
; Most simply, you can have both these files in the same directory (i.e. folder) as this CSD file
; On Mac OS X, these files are libLeapIntoCsound.dylib and libLeap.dylib


;------------------------------------------
;
;	Two FM synths on Hand A and B
;
; This program demonstrates the use of hand
; A and B together, i.e. bi-manual synth
; input.
;
; Per hand:
;   x-axis : panning and modulation frequency
;   y-axis : input frequency
;   z-axis : modulation index
;
; Also the distance between each hand, if
; and only if two hands are visible, modulates
; a tremello effect on the master channel.
; The closer the hands, the lower the trem
; frequency, and vice versa.
;
; Instruments:
;   1 - Settings
;   2 - Hand A controlled synth. Note, uses discrete
;   			MIDI note values for input freq.
;   3 - Hand B controlled synth. Note, uses continuous
;			frequency values.
;   4 - Mixer. Mixes gloabl a-rate signal buffers and
;			applies tremello if hands > 1
;------------------------------------------


; midi note value to frequency consts
gilowestmidinote init 32
gihighestmidinote init 100

;for fm synth
gimaxmodfreq init 3
gimaxmodidx init 10
gksynth1on init 0
gksynth2on init 0

; for tremolo on master mix
gimaxtremfreq init 10
gitremamt init 0.7

; buses from synth to master mix
gasynth1l init 0
gasynth1r init 0
gasynth2l init 0
gasynth2r init 0




; just for settings
instr 1

; set leap settings
;leapsettings 	ipersistent		[,obgapp,	hminalive]
;    default   	1			0		0.2 (actually 127 but is defaulted to 0.2 if > 10)
leapsettings 	1,			1,		0.2

endin




; leap hand synth
;
; simple synth which takes it's pitch from the y position
;
; note that the pitch is discrete between gilowestmidinote and gihighestmidinote midi notes
;
; hand A
instr 2

kampmax = 0.5
kposx, kposy, kposz leaphand 2, 0  	;hand A, hand position

gksynth1on leaphand 2, 6				;hand A, activity

knote = ((gihighestmidinote - gilowestmidinote) * kposy) + gilowestmidinote
kfreq = cpsmidinn( floor(knote) )

kamp portk gksynth1on, 0.2			
kamp = kamp * kampmax

kmodfreq init 0
kmodidx init 0
;kmodfreq portk (gimaxmodfreq * (kposx+1.0) * 0.5), 0.2
;kmodidx portk (gimaxmodidx * (kposz+1.0) * 0.5), 0.2

asig foscili kamp, kfreq, 1, kmodfreq, kmodidx, 1

; clip signal at 0.5 (shouldn't be more than this anyway)	
aclipped  clip asig, 0, 0.5
; pan signal using kposx
gasynth1l, gasynth1r pan2 aclipped, ((kposx+1)*0.5)

endin




; leap hand synth
;
; simple synth which takes it's pitch from the y position
;
; note that the pitch is continuous between gilowestmidinote and gihighestmidinote frequencies
;	and is scaled to an exponential curve
;
; hand B
instr 3

kampmax = 0.5
kposx, kposy, kposz leaphand 3, 0  	;hand B, hand position

gksynth2on leaphand 3, 6				;hand B, activity

knote = ((cpsmidinn(gihighestmidinote) - cpsmidinn(gilowestmidinote)) * (kposy*kposy)) + cpsmidinn(gilowestmidinote)
kfreq portk knote, 0.5

;printk2 kactive
kamp portk gksynth2on, 0.2			
kamp = kamp * kampmax

;asig oscil kamp, kfreq, 1
kmodfreq init 0
kmodidx init 0
;kmodfreq portk (gimaxmodfreq * (kposx+1.0) * 0.5), 0.2
;kmodidx portk (gimaxmodidx * (kposz+1.0) * 0.5), 0.2

asig foscili kamp, kfreq, 1, kmodfreq, kmodidx, 1

; clip signal at 0.5 (shouldn't be more than this anyway)	
aclipped  clip asig, 0, 0.5
; pan signal using kposx
gasynth2l, gasynth2r pan2 aclipped, ((kposx+1)*0.5)

endin




; master channel mixer
instr 4

kdist leaphand 4, 0				; hand A/B, distance
printk2 kdist
atrem poscil gitremamt, (kdist * gimaxtremfreq), 1

if gksynth1on == 1 && gksynth2on == 1 then
	ktremmixtarget = 1
else
	ktremmixtarget = 0
endif

ktremmix portk ktremmixtarget, 0.3

printk2 ktremmix

; mix wet signal at ktremmix, and dry signal with its inverse
outs (((gasynth1l * atrem) + (gasynth2l * atrem)) * ktremmix) + ((gasynth1l + gasynth2l) * (1 - ktremmix)), (((gasynth1r * atrem) + (gasynth2r * atrem)) * ktremmix) + ((gasynth1r + gasynth2r) * (1 - ktremmix))

endin


</CsInstruments>
<CsScore>
; Run Csound indefinitely
f 0 3600
f1 0 16384 10 1

; set settings
i 1 0 1

; leap instruments
i 2 0 -1
i 3 0 -1

; master mixer
i 4 0 -1


e
</CsScore>
</CsoundSynthesizer>
<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>0</x>
 <y>22</y>
 <width>400</width>
 <height>200</height>
 <visible>true</visible>
 <uuid/>
 <bgcolor mode="nobackground">
  <r>231</r>
  <g>46</g>
  <b>255</b>
 </bgcolor>
 <bsbObject type="BSBVSlider" version="2">
  <objectName>slider1</objectName>
  <x>5</x>
  <y>5</y>
  <width>20</width>
  <height>100</height>
  <uuid>{c7303246-d42e-4f2d-a474-44fb146559b1}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <minimum>0.00000000</minimum>
  <maximum>1.00000000</maximum>
  <value>0.00000000</value>
  <mode>lin</mode>
  <mouseControl act="jump">continuous</mouseControl>
  <resolution>-1.00000000</resolution>
  <randomizable group="0">false</randomizable>
 </bsbObject>
</bsbPanel>
<bsbPresets>
</bsbPresets>
