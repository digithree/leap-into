<CsoundSynthesizer>
<CsOptions>
-o dac --opcode-lib=libLeapIntoCsound.dylib
-d
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
;		Circle gesture example
;
; This program demonstrates how to use the
; circle gesture.
;
; This program emulates a traditional music
; box, the kind one winds up with a key
; and lets play.
;
; The circle gesture is perfect for this
; application. From the Leap Motion API
; docs:
;
; 	The Leap Motion software recognizes the
; 	motion of a finger tracing a circle in
; 	space as a Circle gesture. [...] A circle
;	gesture ends when the circling finger or
;	tool departs from the circle locus or
;	moves too slow.
;
; So we use the circle gesture to "wind up"
; the music box. When the Leap system
; recognises your circle gesture, you will
; hear the familiar winding-up sound, played
; at a rate (i.e. speed and pitch) relative
; to the speed at which you are tracing the
; circle. If you continue this gesture for
; more than one second, when you stop you
; will hear the tinkling music of a music box.
; This music will slow down gradually at a
; rate relative to how long you "wound up"
; the music box.
;
; The mechanism that triggers the music box
; sound is this. Instr1 keeps track of the
; circle gestures movement velocity (i.e
; progress change) and the time it has been
; life. If the movement stops and the
; gesture had been live for more than one
; second, then the music box instrument (2)
; is triggered using a score statement (using
; the event opcode). The duration of instr2
; is twice the length of the winding up.
; Instr2 then plays and decreases the rate
; of playback (and thus the pitch) according
; to a logrithmic curve, i.e. slow to begin
; with and faster at the end.
;------------------------------------------


instr 1

iwindupsamplelength = 1 / 3


; change leap internal settings
leapsettings 0			; turn persistent data off

; get circle data, progress change
kcircle leapgesture 2, 2		; circle, progress _change_
;printk2 kcircle

; --- wind up ---

; play wind up sound at rate relative to circle progress change
aratewindup phasor (iwindupsamplelength * kcircle * 3)
aratewindup = aratewindup * ftlen(1)
asigwindup tablei aratewindup, 1

; --- music box ---

kcirclealivepersistent init 0
; get circle alive time
kcirclealive leapgesture 2, 6	; circle, alive time
if kcirclealive > 0 then
	kcirclealivepersistent = kcirclealive
endif
printk2 kcirclealivepersistent

; check to see if winding has stopped and user has wound for more than one second
kcirclechanged changed kcircle
if kcirclechanged == 1 && kcircle == 0 && kcirclealivepersistent >= 1 then
	event "i", 2, 0, kcirclealivepersistent
endif

outs asigwindup, asigwindup

endin 



; music box
instr 2

imusicboxlength = 1 / 9

kmusicboxrate line 1, p3, 0

; map kmusicboxrate to log curve, i.e. slows down slowly at first, quickly toward the end
klograte = log10( 1 + (kmusicboxrate*9) )
;printk2 klograte

; play sound
aratemusicbox phasor (imusicboxlength * klograte)
aratemusicbox = aratemusicbox * ftlen(2)
asigmusicbox tablei aratemusicbox, 2

outs asigmusicbox, asigmusicbox

endin




</CsInstruments>
<CsScore>
; Run Csound indefinitely
f 0 3600

; load samples
f1 0 0 1 "windup1.wav" 0 0 0
f2 0 0 1 "musicbox.wav" 0 0 0

;sine wave
;f1 0 16384 10 1

i 1 0 -1

e
</CsScore>
</CsoundSynthesizer>
<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>0</x>
 <y>44</y>
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
  <uuid>{1b554a38-602e-4ade-a3ac-7e5579577e2a}</uuid>
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
