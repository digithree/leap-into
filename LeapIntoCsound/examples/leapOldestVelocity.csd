<CsoundSynthesizer>
<CsOptions>
-o dac --opcode-lib=libLeapIntoCsound.dylib -v
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
;		Hand velocity example
;
; Here is a simple demo to show the hand
; velocity in action. We are using just one
; hand, the oldest, as it is the most
; consistent for single handed applications.
;
; All we are doing is scaling concert A
; (440 Hz) up if there is any change in
; velocity. We then render a single sine
; wave at this frequency.
;
; Even though this is a very simple use of
; hand velocity, we can see it results in a
; very dynamic and enjoyable demo.
;------------------------------------------

instr 1

kamp = 0.5
kvel leaphand 1, 2		; oldest hand, velocity (magnitude)

printk2 kvel

kvel = kvel * kvel	; square kvel to supress small values

kfreqtarget = 440 + (kvel * 440)
kfreq init 440
kfreq port kfreqtarget, 0.05

asig oscil kamp, kfreq, 1

aclipped clip asig, 0, 0.5

	outs aclipped, aclipped
endin

</CsInstruments>
<CsScore>
; Run Csound indefinitely
f 0 3600
f1 0 16384 10 1

i 1 0 -1

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
