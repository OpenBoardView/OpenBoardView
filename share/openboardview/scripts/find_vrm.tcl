proc find_vrm { cell } {
	if { [ llength $cell ] == 0 } return
	foreach c $cell {
		set pinc [pin_counts $c]

		#puts $pinc
		
		dict for { net pins } $pinc {
			#if { $pins > 15 } break
			if { $pins < 10 } break

			#puts "$net $pins"
			
			set cell [find_vrm_inductor $net [linsert $cell end $c]]
		}
	}
	return $cell
}

proc find_vrm_fet { net { cell } } {

	foreach c [get_cells -of $net -not $cell] {
		set npins [get_prop pins $c]
		set cstr [dup $c]
		set t [guess_type $c]
		
		if { $npins > 2 && [regexp {[^uU]?[uU][0-9]+} $cstr] } {
			puts "IC match $c"
			highlight -color \#ff0000aa $c
			set cell [find_bypass_caps \#ffff00 [get_nets -of $c -not $net -filter { ! $isgnd }] [linsert $cell end $c]]
		} elseif { $npins > 2 && [regexp {[^qQ]?[qQ][0-9]+} $cstr] } {
			puts "fet match $c"
			highlight -color \#ffee00ee $c
			set cell [find_bypass_caps \#ffffff00 [get_nets -of $c -not $net -filter { ! $isgnd }] [linsert $cell end $c]]
		} elseif { $t == "G" || $t == "J" } {
			set cell [find_vrm_fet [get_nets -of $c -not $net -filter { ! $isgnd }] [linsert $cell end $c]]
		}
	}
	return $cell
}
proc find_vrm_inductor { net { cell } } {
	# puts "inductor $net"
	foreach c [get_cells -of $net -not $cell] {
		set npins [get_prop pins $c]
		set cstr [list $c]
		set t [guess_type $c]

		# puts "$npins $t"
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
		} elseif { $t == "R" && $npins == 4 } {
			puts "sense resistor match $c"
			set cell [find_vrm_inductor [get_nets -of $c -not $net] [linsert $cell end $c]]
		} else {
			#puts "cap nomatch $c [get_prop pins $c]"
		}
	}
	return $cell
}
