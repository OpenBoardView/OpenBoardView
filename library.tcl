set dir /usr/share/tcltk/tcllib1.19/struct
source "${dir}/pkgIndex.tcl"
package require struct::set

proc adjacent_cells { c } {
	set ret [list]

	foreach n [get_nets -of $c ] {
		foreach cc [get_cells -of $n ] {
			if { $c == $cc || [lsearch $ret $cc] != -1 } continue
			lappend ret $cc
		}
	}
	return $ret
}

proc net_intersect { clist } {
	set nets [list]
	set allnets [list]
	foreach c $clist {
		#puts $c
		foreach n [get_nets -of $c -filter { ! $isgnd }] {
			#puts $n
			if { [ lsearch $allnets $n ] != -1 } {
				lappend nets $n
			}
			lappend allnets $n
		}
	}
	return $nets
}

proc pin_count { cell net } {
	llength [::struct::set intersect [get_pins -of $net] [get_pins -of $cell]]
}

proc pin_counts { cell } {
	set ret { }
	foreach n [get_nets -all -of $cell] {
		dict incr ret $n
	}
	return [lsort -index 1 -stride 2 -integer -decreasing $ret]
}

proc is_high_current { pins } {
	set ret [list]
	foreach p $pins {
		set p2 [get_pins -parallel -of $p]
		if { [llength $p2] > 1 || [get_prop ispad $p2] } {
			lappend ret 1
		} else {
			lappend ret 0
		}
	}
	return $ret
}

proc find_vrm { cell } {
	if { [ llength $cell ] == 0 } return
	foreach c $cell {
		set pinc [pin_counts $c]
	
		dict for { net pins } $pinc {
			#if { $pins > 15 } break
			if { $pins < 10 } break
			
			set cell [find_vrm_inductor $net [linsert $cell end $c]]
		}
	}
	return $cell
}

proc guess_type { cell } {
	set cstr [list $cell]
	set npins [get_prop pins $cell]

	if { $npins > 2 && [regexp {[^uU]?[uU][0-9]+} $cstr] } {
		return "U"
	} elseif { ($npins == 2 || $npins == 4) && [regexp {[^rR]?[rR][0-9]+} $cstr] } {
		return "R"
	} elseif { $npins == 2 && [regexp {([^cC]?[cC]|[cC][eE])[0-9]+} $cstr] } {
		return "C"
	} elseif { $npins > 2 && [regexp {[^qQ]?[qQ][0-9]+} $cstr] } {
		return "Q"
	} elseif { ($npins == 2 || $npins == 4 || $npins % 2 == 0) && [regexp {[^lL]?[lL][0-9]+} $cstr] } {
		return "L";
	} elseif { $npins == 2 && ([regexp {[^jJ]?[jJ][0-9]+} $cstr] || [regexp {[jJ][pP][0-9]+} $cstr]) } {
		return "J";
	} elseif { $npins == 2 && [regexp {[^gG]?[gG][0-9]+} $cstr] } {
		return "G";
	}
	return "?"
}

proc guess_switch_topology { c_inductor n_phase } {
	foreach c [get_cells -of $n_phase -not $c_inductor] {
		if [ [guess_type $c] == "C" ] {
		}
	}
}

proc guess_fet_type { cell } {
	foreach n [get_nets $cell] {
		set nstr [list $n]
		if { [regexp {BOOT|BST|BOOST} $nstr] } {
			incr has_bst
		} elseif { [regexp {UG|HG} $nstr] } {
			incr has_ug
		} elseif { [regexp {LG} $nstr] } {
			incr has_lg
		} elseif { [regexp {SW|PHASE|LX} $nstr] } {
			incr has_phase
		} elseif { [regexp {PWM} $nstr] } {
			incr has_pwm
		}
		if { $has_pwm == 1 && $has_ug == 1 && $has_lg == 1 && $has_bst == 1 && $has_phase == 1 } {
			return "fet_driver"
		} elseif { $has_ug == 1 && $has_lg == 1 } {
			return "dual_fet"
		} elseif { $has_ug == 1 } {
			return "high_side_fet";
		} elseif { $has_lg == 1 } {
			return "low_side_fet";
		} elseif { $has_pwm > 1 && $has_ug == 0 && $has_lg == 0 && $has_bst == 0 && has_phase == 0 } {
			return "pwm_only";
		}
	}
}

proc find_bypass_caps { color net { cell } } {
	foreach c [get_cells -of $net -not $cell] {
		if { [ guess_type $c ] == "C" && [get_prop has_gnd $c] } {
			highlight -color $color $c
		}
	}
	return $cell
}

proc find_vrm_fet { net { cell } } {

	foreach c [get_cells -of $net -not $cell] {
		set npins [get_prop pins $c]
		set cstr [list $c]
		set t [guess_type $c]
		
		if { $npins > 2 && [regexp {[^uU]?[uU][0-9]+} $cstr] } {
			puts "IC match $c"
			highlight -color \#ff0000aa $c
			set cell [find_bypass_caps \#00ff00 [get_nets -of $c -not $net -filter { ! $isgnd }] [linsert $cell end $c]]
		} elseif { $npins > 2 && [regexp {[^qQ]?[qQ][0-9]+} $cstr] } {
			puts "fet match $c"
			highlight -color \#ffee00ee $c
			set cell [find_bypass_caps \#00ff00 [get_nets -of $c -not $net -filter { ! $isgnd }] [linsert $cell end $c]]
		} elseif { $t == "G" || $t == "J" } {
			set cell [find_vrm_fet [get_nets -of $c -not $net -filter { ! $isgnd }] [linsert $cell end $c]]
		}
	}
	return $cell
}

proc find_vrm_inductor { net { cell } } {
	foreach c [get_cells -of $net -not $cell] {
		set npins [get_prop pins $c]
		set cstr [list $c]
		set t [guess_type $c]
		
		if { $t == "C" } {
			if { [get_prop has_gnd $c] } {
				#puts "bypass cap match $c"
				highlight -color \#ff00aa00 $c
			} else {
				#puts "cap match $c"
			}
		} elseif { $t == "L" } {
			puts "inductor match $c"
			highlight $c
			set cell [find_vrm_fet [get_nets -of $c -not $net] [linsert $cell end $c]]
		} elseif { $t == "J" } {
			puts "jumper match $c"
			set cell [find_vrm_inductor [get_nets -of $c -not $net] [linsert $cell end $c]]
		} elseif { $t == "G" } {
			puts "gap match $c"
			set cell [find_vrm_inductor [get_nets -of $c -not $net] [linsert $cell end $c]]
		} else {
			#puts "cap nomatch $c [get_prop pins $c]"
		}
	}
	return $cell
}


proc r {} {
	find_vrm [get_cells UC1]
	#adjacent_cells [lindex [get_cells] 8]
}

proc RE {} {
	source library.tcl
}