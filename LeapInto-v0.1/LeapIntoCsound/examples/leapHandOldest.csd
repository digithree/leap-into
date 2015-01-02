<CsoundSynthesizer>
<CsOptions>
-o dac --opcode-lib=libLeapIntoCsound.dylib
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
0dbfs = 1
nchnls = 2 


; FM Synth driven by oldest hand
; x-axis = modulation frequency - between 0 and 3 Hz
; y-axis = note frequency - between 110 and 800 Hz
; z-axis = modulation index - between 0 and 10
instr 1

kposx, kposy, kposz leaphand 1, 0  	;oldest hand, hand position
kactive leaphand 1, 6				;oldest hand, activity

; Use portamento for smooth change in parameters
kfreq portk (110 + (kposy * 770)), 0.1
kmodfreq portk (3 * (kposx+1.0) * 0.5), 0.1
kmodidx portk (10 * (kposz+1.0) * 0.5), 0.1
;	note: z-axis range is -1 to 1, so add 1 to it (now between 0 and 2) and half

; Fade amplitude in if hand is active, out if not
kamp portk kactive, 0.2			
kamp = kamp * 0.5 ;max amp is 0.5

; Generate FM synth using f-table 1 (sine wave, see score)
asig foscili kamp, kfreq, 1, kmodfreq, kmodidx, 1



	outs asig, asig
endin


</CsInstruments>
<CsScore>
; Run Csound indefinitely
f 0 3600
; Sine wave f-table
f1 0 16384 10 1

; leap instrument
i 1 0 -1 ;run forever

e
</CsScore>
</CsoundSynthesizer>
<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>100</x>
 <y>100</y>
 <width>320</width>
 <height>240</height>
 <visible>true</visible>
 <uuid/>
 <bgcolor mode="nobackground">
  <r>255</r>
  <g>255</g>
  <b>255</b>
 </bgcolor>
</bsbPanel>
<bsbPresets>
</bsbPresets>
