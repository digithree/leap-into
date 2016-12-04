<CsoundSynthesizer>
<CsOptions>
-o dac --opcode-lib=libLeapIntoCsound.dylib
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
0dbfs = 1
nchnls = 2 

; IMPORTANT!!! You must have both the offical Leap binary and the LeapIntoCsound binary in the path
; Most simply, you can have both these files in the same directory (i.e. folder) as this CSD file
; On Mac OS X, these files are libLeapIntoCsound.dylib and libLeap.dylib


;------------------------------------------
;
;  		Swipe gesture example
;
; Simple program to demonstrate swipe.
;
; The output is filtered pink noise. This
; uses a simple lowpass filter with the
; cutoff frequency dependent on the velocity
; of the swipe. Portamento is applied to the
; changing frequency with a long halflife.
; This produces a wind-like sound which dies
; down gradually.
;
; From the Leap Motion API reference:
; The Leap Motion software recognizes a
; linear movement of a finger as a Swipe
; gesture.
; You can make a swipe gesture with any
; finger and in any direction. Swipe
; gestures are continuous. Once the
; gesture starts, the Leap Motion software
; will update the progress until the
; gesture ends. A swipe gesture ends when
; the finger changes directions or moves
; too slow.
; -----------------------------------------



; just for settings
instr 1

; set leap settings
;leapsettings 	ipersistent		[,obgapp,	hminalive]
;    default   	1			0		0.2 (actually 127 but is defaulted to 0.2 if > 10)
leapsettings 	1,			1,		0.2

endin


instr 2

iscalefactor init 20000	;10k
ibandsize init 2000		;2k

leapsettings 0.2, 0		; turn off persistent mode

;klen leapgesture 3, 2		; swipe, length of swipe
kvel leapgesture 3, 3		; swipe, velocity
printk2 kvel

asig pinkish 0.5
kfilter port (kvel * iscalefactor), 0.5
asigfilter butterlp asig, kfilter

asigfilter clip asigfilter, 0, 0.5

outs asigfilter, asigfilter

endin 




</CsInstruments>
<CsScore>
; Run Csound indefinitely
f 0 3600

;sine wave
f1 0 16384 10 1

; settings
i 1 0 1

; instrument
i 2 0 -1

e
</CsScore>
</CsoundSynthesizer>
<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>72</x>
 <y>179</y>
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
  <uuid>{43ff132a-ecbd-49b8-bb63-3a38b1b9bee7}</uuid>
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
