#!/usr/bin/wish -f
#
# Virtual Tiny Keyboard
#
# Copyright (c) 1997-2000 by Takashi Iwai
#
# turn off auto-repeat on your X display by "xset -r"
#

#----------------------------------------------------------------
# default files
#----------------------------------------------------------------

# preset list file
set defpresetfile vkeybd.list
set defconfig vkeybdrc

#----------------------------------------------------------------
# keyboard size (width & height)

set keywid 18
set keyhgt 72

#----------------------------------------------------------------
# keyboard map {key-symbol midi-number}

set keymap {
    {a 8} {z 9} {s 10} {x 11}
    {c 12} {f 13} {v 14} {g 15} {b 16} {n 17} {j 18} {m 19} {k 20}
    {comma 21} {l 22} {period 23}
    {slash 24} {apostrophe 25} {backslash 26} {grave 27}
}

#----------------------------------------------------------------
# create virtual keyboard

proc KeybdCreate {w} {
    global keycolor keywid keyitem keyindex keyhgt keymap keywin

    set keywin $w

    canvas $w -width [expr $keywid * 21]  -height $keyhgt -bd 1 -bg black
    pack $w -side top
    for {set i 0} {$i < 36} {incr i} {
	set octave [expr ($i / 12) * 7]
	set j [expr $i % 12]
	if {$j >= 5} {incr j}
	if {$j % 2 == 0} {
	    set x1 [expr ($octave + $j / 2) * $keywid]
	    set x2 [expr $x1 + $keywid]
	    set y1 0
	    set y2 $keyhgt
	    set id [$w create rectangle $x1 $y1 $x2 $y2 -width 1\
		    -fill white -outline black -tags ebony]
	    set keycolor($i) white
	} else {
	    set x1 [expr ($octave + $j / 2) * $keywid + $keywid * 6/10]
	    set x2 [expr $x1 + $keywid * 8/10 - 1]
	    set y1 0
	    set y2 [expr $keyhgt * 6 / 10]
	    set id [$w create rectangle $x1 $y1 $x2 $y2 -width 1\
		    -fill black -outline white -tags ivory]
	    set keycolor($i) black
	}
	set keyitem($i) $id
	set keyindex($id) $i
	$w bind $id <Button-1> [list KeyStart $i 1]
	$w bind $id <ButtonRelease-1> [list KeyStop $i 1]
	$w bind $id <Motion> [list KeyMotion $i %x %y %s]
    }
    $w lower ebony

    foreach i $keymap {
	set key [lindex $i 0]
	set note [lindex $i 1]
	bind $w <KeyPress-$key> [list KeyStart $note 0]
	bind $w <KeyRelease-$key> [list KeyStop $note 0]
    }

    #
    # some special key sequences
    #
    bind $w <Key-Escape> {SeqOff; ResetControls}
    bind $w <Control-c> {exit 0}
    bind $w <Key-q> {exit 0}
    focus $w
}

#----------------------------------------------------------------
# note on/off

# base key note and default velocity
set keybase 48
set keyvel 127
set activekey ""

proc KeyStart {key button} {
    global keybase keywin keyitem keyvel activekey
    SeqOn
    if {$button == 1} {
	set activekey $keyitem($key)
    }
    $keywin itemconfigure $keyitem($key) -fill blue
    set key [expr $key + $keybase]
    SeqStartNote $key $keyvel
}

proc KeyStop {key button} {
    global keybase keywin keyitem keyindex keycolor activekey
    SeqOn
    if {$button == 1 && $activekey != ""} {
	set key $keyindex($activekey)
	set activekey ""
    }
    $keywin itemconfigure $keyitem($key) -fill $keycolor($key)
    set key [expr $key + $keybase]
    SeqStopNote $key 0
}

proc KeyMotion {key x y s} {
    global activekey keywin keyitem keyindex
    if {($s & 256) == 0} {
	if {$activekey != ""} {
	    KeyStop $keyindex($activekey) 1
	}
	return
    }
    set new [lindex [$keywin find overlapping $x $y $x $y] 0]
    if {$new != $activekey} {
	if {$activekey != ""} {
	    KeyStop $keyindex($activekey) 1
	}
	if {$new != ""} {
	    KeyStart $keyindex($new) 1
	}
    }
}

#----------------------------------------------------------------
# midi controls

set controls {
    {"ModWheel" 1 0}
    {"Volume" 7 127}
    {"Express" 11 127}
    {"Panning" 10 64}
    {"Reverb" 91 0}
    {"Chorus" 93 0}
    {"Sustain" 64 0}
    {"Sostenuto" 66 0}
}

proc ResetControls {{send_seq 0}} {
    global controls curctrl ctrlval
    foreach i $controls {
	set type [lindex $i 1]
	set ctrlval($type) [lindex $i 2]
	if {$send_seq} {SeqControl $type $ctrlval($type)}
    }
    # all sounds off
    SeqControl 120 0
}

proc NewControl {w ctrl} {
    global curctrl ctrlval
    set type [lindex $ctrl 1]
    set curctrl $type
    $w.ctrl configure -text [lindex $ctrl 0]
    $w.val configure -variable ctrlval($type)
    SeqControl $type $ctrlval($type)
}

#----------------------------------------------------------------
# program selection

# make a bank and preset list from preset file

proc MakeLists {w} {
    global sflist bankvar bankmode optvar

    set sflist {}
    if {[file readable $optvar(preset)]} {
	ReadSF $optvar(preset) sflist
    }

    if {$sflist == {}} {
	lappend sflist [list 0 0 "Piano"]
    }

    foreach i $sflist {
	set bank [lindex $i 0]
	set bankvar($bank) 1
    }

    $w.b.list delete 0 end
    $w.b.list insert end "all"
    foreach i [lsort -integer [array names bankvar]] {
	$w.b.list insert end [format "%03d" $i]
    }
    set bankmode all

    MakePresets $w
}

# read preset table

proc ReadSF {fname var} {
    upvar $var listp
    set file [open $fname r]
    if {$file == ""} {
	puts stderr "can't open file $fname."
	return
    }
    while {1} {
	set line [string trim [gets $file]]
	if {$line == ""} {break}
	if {[string match "#*" $line]} {continue}
	set bank [lindex $line 0]
	set preset [lindex $line 1]
	set name [lrange $line 2 [llength $line]]
	lappend listp [list $bank $preset $name]
    }

    close $file
}



# compare two preset list elements

proc listcmp {a b} {
   set v [expr [lindex $a 0] - [lindex $b 0]]
    if {$v == 0} {
	set v [expr [lindex $a 1] - [lindex $b 1]]
    }
    return $v
}

# make preset list

proc MakePresets {w} {
    global sflist bankmode
    $w.p.list delete 0 end
    foreach i [lsort -command listcmp $sflist] {
	set str [eval format "%03d:%03d:%s" $i]
	if {$bankmode == "all" || $bankmode == [lindex $i 0]} {
	    $w.p.list insert end $str
	}
    }
}

# change the current channel

proc InitPreset {} {
    global chanbank chanpreset
    for {set i 0} {$i < 16} {incr i} {
	if {$i == 9} {
	    set chanbank($i) 128
	} else {
	    set chanbank($i) 0
	}
	set chanpreset($i) 0
    }
}

proc DecrChannel {} {
    global optvar chanbank chanpreset
    if {$optvar(channel) > 0} {
	set optvar(channel) [expr $optvar(channel) - 1]
	# all sounds off
	SeqControl 120 0
	SeqProgram $chanbank($optvar(channel)) $chanpreset($optvar(channel))
    }
}

proc IncrChannel {} {
    global optvar chanbank chanpreset
    if {$optvar(channel) < 15} {
	incr optvar(channel)
	# all sounds off
	SeqControl 120 0
	SeqProgram $chanbank($optvar(channel)) $chanpreset($optvar(channel))
    }
}

# remake preset list from selected bank listbox

proc BankSelect {w coord} {
    global bankmode
    set idx [$w.b.list nearest $coord]
    if {$idx == ""} {return}
    set sel [$w.b.list get $idx]
    if {$sel == "all"} {
	set mode all
    } else {
	scan $sel "%d" mode
    }
    if {$mode != $bankmode} {
	set bankmode $mode
	MakePresets $w
    }
    $w.b.label configure -text "Bank:$bankmode"
}

# set the selected preset to sequencer

proc ProgSelect {w coord} {
    global sflist chanpreset chanbank optvar
    set idx [$w.p.list nearest $coord]
    if {$idx == ""} {return}
    set sel [$w.p.list get $idx]
    set lp [split $sel :]
    scan [lindex $lp 0] "%d" bank
    scan [lindex $lp 1] "%d" preset
    set name [join [string range $lp 8 end]]
    set chanbank($optvar(channel)) $bank
    set chanpreset($optvar(channel)) $preset
    SeqProgram $bank $preset
    $w.p.label configure -text "Preset:$name"
}

#----------------------------------------------------------------
# make a listbox

proc my-listbox {w title width height {dohoriz 0}} {
    frame $w
    label $w.label -text $title -relief flat
    pack $w.label -side top -fill x -anchor w
    scrollbar $w.yscr -command "$w.list yview"
    pack $w.yscr -side right -fill y
    set lopt [list -width $width -height $height]
    lappend lopt -selectmode single -exportselect 0
    if {$dohoriz} {
	scrollbar $w.xscr -command "$w.list xview" -orient horizontal
	pack $w.xscr -side bottom -fill x
	eval listbox $w.list -relief sunken -setgrid yes $lopt\
		[list -yscroll "$w.yscr set"]\
		[list -xscroll "$w.xscr set"]
    } else {
	eval listbox $w.list -relief sunken -setgrid yes $lopt\
		[list -yscroll "$w.yscr set"]
    }
    pack $w.list -side left -fill both -expand yes
    return $w.list
}

#----------------------------------------------------------------
# create windows

# toggle display of preset selection windows

set disp(keyvel) 0
set disp(ctrl) 0
set disp(pitch) 0
set disp(prog) 0

proc ToggleView {w cvar} {
    global disp
    if {$disp($cvar)} {
	pack $w.$cvar -side top -fill x
    } else {
	pack forget $w.$cvar
    }
}

# toggle sequencer on/off

set seqswitch 0
proc ToggleSeqOn {w} {
    global seqswitch
    if {$seqswitch} {
	SeqOn
    } else {
	SeqOff
	ResetControls
    }
}


# create the pulldown menues
proc MenuCreate {{pw ""}} {
    global env optvar defconfig

    set w $pw.menubar
    menu $w -tearoff 0

    $w add cascade -menu $w.file -label "File" -underline 0
    menu $w.file -tearoff 0
    $w.file add checkbutton -label "Connection"\
	    -command "ToggleSeqOn $w.file.off"\
	    -variable seqswitch -underline 0
    $w.file add command -label "Save Config" -command "SaveConfig $env(HOME)/.$defconfig" -underline 0
    $w.file add command -label "Quit" -command {exit 0} -underline 0

    $w add cascade -menu $w.view -label "View" -underline 0
    menu $w.view -tearoff 0
    $w.view add check -label "Key/Velocity" -variable disp(keyvel)\
	    -command [list ToggleView $pw keyvel]
    $w.view add check -label "Controls" -variable disp(ctrl)\
	    -command [list ToggleView $pw ctrl]
    $w.view add check -label "Pitchwheel" -variable disp(pitch)\
	    -command [list ToggleView $pw pitch]
    $w.view add check -label "Program List" -variable disp(prog)\
	    -command [list ToggleView $pw prog]

    $w add cascade -menu $w.reverb -label "Reverb" -underline 0
    menu $w.reverb -tearoff 0
    $w.reverb add radio -label "Room 1"\
	    -variable reverbmode -value 0 -command "SeqReverbMode 0"
    $w.reverb add radio -label "Room 2"\
	    -variable reverbmode -value 1 -command "SeqReverbMode 1"
    $w.reverb add radio -label "Room 3"\
	    -variable reverbmode -value 2 -command "SeqReverbMode 2"
    $w.reverb add radio -label "Hall 1"\
	    -variable reverbmode -value 3 -command "SeqReverbMode 3"
    $w.reverb add radio -label "Hall 2"\
	    -variable reverbmode -value 4 -command "SeqReverbMode 4"
    $w.reverb add radio -label "Plate"\
	    -variable reverbmode -value 5 -command "SeqReverbMode 5"
    $w.reverb add radio -label "Delay"\
	    -variable reverbmode -value 6 -command "SeqReverbMode 6"
    $w.reverb add radio -label "Panning Delay"\
	    -variable reverbmode -value 7 -command "SeqReverbMode 7"

    $w add cascade -menu $w.chorus -label "Chorus" -underline 0
    menu $w.chorus -tearoff 0
    $w.chorus add radio -label "Chorus 1"\
	    -variable chorusmode -value 0 -command "SeqChorusMode 0"
    $w.chorus add radio -label "Chorus 2"\
	    -variable chorusmode -value 1 -command "SeqChorusMode 1"
    $w.chorus add radio -label "Chorus 3"\
	    -variable chorusmode -value 2 -command "SeqChorusMode 2"
    $w.chorus add radio -label "Chorus 4"\
	    -variable chorusmode -value 3 -command "SeqChorusMode 3"
    $w.chorus add radio -label "Feedback"\
	    -variable chorusmode -value 4 -command "SeqChorusMode 4"
    $w.chorus add radio -label "Flanger"\
	    -variable chorusmode -value 5 -command "SeqChorusMode 5"
    $w.chorus add radio -label "Short Delay"\
	    -variable chorusmode -value 6 -command "SeqChorusMode 6"
    $w.chorus add radio -label "Short Delay 2"\
	    -variable chorusmode -value 7 -command "SeqChorusMode 7"

    global reverbmode chorusmode
    set reverbmode 4
    set chorusmode 2

}    

# create the virtual keyboard panel

proc PanelCreate {{pw ""}} {
    global controls curctrl ctrlval

    set w $pw.ctrl
    frame $w

    frame $w.chan
    label $w.chan.label -text "Channel"
    button $w.chan.left -text "<" -command { DecrChannel }
    label $w.chan.digit -textvariable optvar(channel)
    button $w.chan.right -text ">" -command { IncrChannel }
    pack $w.chan.label $w.chan.left $w.chan.digit $w.chan.right -side left -expand 1

    frame $w.c
    label $w.c.label -text "Control"
    menubutton $w.c.ctrl -relief raised -width 9 -menu $w.c.ctrl.m
    menu $w.c.ctrl.m
    foreach i $controls {
	set label [lindex $i 0]
	set type [lindex $i 1]
	$w.c.ctrl.m add radio -label $label\
		-variable curctrl -value $type\
		-command [list NewControl $w.c $i]
	set ctrlval($type) [lindex $i 2]
    }
    $w.c.ctrl.m add separator
    $w.c.ctrl.m add command -label "Reset" -command {ResetControls 1}
    scale $w.c.val -orient horizontal -from 0 -to 127 -showvalue true\
	    -command {SeqControl $curctrl}
    NewControl $w.c [lindex $controls 0]
    pack $w.c.label $w.c.ctrl $w.c.val -side left -expand 1

    pack $w.chan $w.c -side left -expand 1

    #----------------------------------------------------------------

    set w $pw.keyvel
    frame $w
    frame $w.k
    label $w.k.label -text "Key"
    scale $w.k.val -orient horizontal\
	    -from 0 -to 84 -resolution 12\
	    -showvalue true -variable keybase
    pack $w.k.label $w.k.val -side left -expand 1

    frame $w.v
    label $w.v.label -text "Velocity"
    scale $w.v.val -orient horizontal\
	    -from 0 -to 127\
	    -showvalue true -variable keyvel
    pack $w.v.label $w.v.val -side left -expand 1

    pack $w.k $w.v -side left -expand 1

    #----------------------------------------------------------------

    set w $pw.pitch
    frame $w

    global pitchbend
    button $w.label -text "Pitch Clear" -command "set pitchbend 0; SeqBender 0"
    scale $w.pitch -orient horizontal -from -8192 -to 8192 -showvalue 0\
	    -length 256 -variable pitchbend -command SeqBender
    set pitchbend 0

    pack $w.label $w.pitch -side left -expand 1
    
    #----------------------------------------------------------------

    set w $pw.prog
    frame $w

    my-listbox $w.p "Preset:" 28 10
    my-listbox $w.b "Bank:all" 5 10
    bind $w.b.list <Button-1> [list BankSelect $w %y]
    bind $w.p.list <Button-1> [list ProgSelect $w %y]
    pack $w.b $w.p -side left -expand 1

    MakeLists $w

    $pw. configure -menu $pw.menubar

    ToggleView $pw keyvel
    ToggleView $pw ctrl
    ToggleView $pw pitch
    ToggleView $pw prog

    #----------------------------------------------------------------

    KeybdCreate $pw.kbd
    pack $pw.kbd -fill x -side bottom -pady 1
}


#----------------------------------------------------------------
# search default path
#
proc SearchDefault {fname} {
    global env optvar
    set path "$env(HOME)/$fname"
    if {[file readable $path]} {
	return $path
    }
    set path "$env(HOME)/.$fname"
    if {[file readable $path]} {
	return $path
    }
    if {[info exists env(VKEYBD)]} {
	set path "$env(VKEYBD)/$fname"
	if {[file readable $path]} {
	    return $path
	}
    }
    set path "$optvar(libpath)/$fname"
    return $path
}

#
# parse command line options
#
proc ParseOptions {argc argv} {
    global optvar

    for {set i 0} {$i <= $argc} {incr i} {
	set arg [lindex $argv $i]
	if {! [string match -* $arg]} {break}
	set arg [string range $arg 2 end]
	if {[info exists optvar($arg)]} {
	    incr i
	    set optvar($arg) [lindex $argv $i]
	} else {
	    usage
	    exit 1
	}
    }
}

#
# load config file
#
proc LoadConfig {fname} {
    if {[file readable $fname]} {
	source $fname
    }
}

#
# save config file
#
proc SaveConfig {fname} {
    global disp keymap
    set file [open $fname w]
    if {$file == ""} {
	tk_messageBox -icon error -message "can't open file $fname." -type ok
	return
    }
    puts $file "global disp keymap"
    puts $file "set disp(keyvel) $disp(keyvel)"
    puts $file "set disp(ctrl) $disp(ctrl)"
    puts $file "set disp(pitch) $disp(pitch)"
    puts $file "set disp(prog) $disp(prog)"
    puts $file "set keymap {"
    foreach i $keymap {
	set key [lindex $i 0]
	set note [lindex $i 1]
	puts $file "  {$key $note}"
    }
    puts $file "}"
    close $file
}

#
# read keymap file
#
proc ReadKeymap {fname} {
    global keymap
    if {[file readable $fname]} {
	set file [open $fname r]
	if {$file == ""} {
	    puts stderr "can't open file $fname."
	    return
	}
	set keymap {}
	while {1} {
	    set line [string trim [gets $file]]
	    if {$line == ""} {break}
	    lappend keymap [list [lindex $line 0] [lindex $line 1]]
	}
	close $file
    }
}

#----------------------------------------------------------------
# main ...
#

set optvar(devfile) ""
if {! [info exists optvar(libpath)]} {
    set optvar(libpath) "/etc"
}

set optvar(preset) [SearchDefault $defpresetfile]
set optvar(config) [SearchDefault $defconfig]
set optvar(channel) 0

ParseOptions $argc $argv

LoadConfig $optvar(config)

InitPreset
MenuCreate
PanelCreate

wm title . "Virtual Keyboard ver.0.1.8"
wm iconname . "Virtual Keyboard"

SeqOn preinit

tkwait window  .
exit 0
