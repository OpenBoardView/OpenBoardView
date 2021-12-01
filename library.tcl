package require struct::set

set trace_frame 0

proc trace_execution { cmd code result op } {
	global trace_frame
	if { $code == 1 } { 
		set f [ info frame -4 ]
		#puts " $code [ dict get $f file ] [ dict get $f line ] [ dict get $f proc ] [ dict get $f level ]"
		if { $trace_frame == 0 } {
			set trace_frame [ info frame -4 ]
		}
	} else {
		set trace_frame 0
	}
}

proc traced { cmd } {
	global trace_frame
	trace add execution eval leavestep trace_execution
	try {
		eval $cmd
	} on error { msg opt } {
		if [ dict exists $trace_frame file ] {
			puts "ERROR: $msg [ dict get $trace_frame file ] [ dict get $trace_frame line ]"
		} else {
			puts "ERROR: $msg [ dict get $trace_frame line ]"
		}
	} finally {
		trace remove execution eval leavestep trace_execution
	}
}

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

proc guess_type { cell } {
	set cstr [dup $cell]
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
		set nstr [dup $n]
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

proc remove_extra_type_letter { cell } {
	set c [ dup $cell ]
	set f [ string index $c 0 ]
	set r [ string range $c 1 end ]
	if { [ string first $f $r ] != -1 } {
		return $r
	}
	return $c
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

proc connected { cell inten } {
	foreach n [ get_nets -of $cell ] {
		highlight -intensity $inten [get_cells -of $n -not $cell]
	}
}

set cell_hover_active [list]

proc has_prop_value { propname propvalue inten } {
	highlight -intensity $inten [ get_cells -filter "\$$propname = { $propvalue }" ]
}

proc cell_hover_1 { cell } {
	global cell_hover_active
	global hoverlamp_mode
	
	# if { [llength $cell_hover_active] } {
	if { $hoverlamp_mode } {
		has_prop_value schem_pages [get_prop schem_pages $cell_hover_active] 1
	} else {
		connected $cell_hover_active 1
	}
	set cell_hover_active [list]
	# }
	if { $cell ne "" } {
		set cell_hover_active $cell
		if { $hoverlamp_mode } {
			has_prop_value schem_pages [get_prop schem_pages $cell ] 10
		} else {
			connected $cell 10
		}
	}
}

set cell_hover { }

proc cell_hover_call { cell } {
	global cell_hover
	foreach cb $cell_hover { $cb $cell }
}

proc l {} {
	obv_open [lindex [file_history] 0]
	obv_open [lindex [file_history] 1]
	obv_open [lindex [file_history] 2]
}

set current_board 0

set hoverlamp_on 0
set hoverlamp_mode 0

proc kb_hoverlamp_mode { } {
	global hoverlamp_mode

	if { $hoverlamp_mode } {
		set hoverlamp_mode 0
	} else {
		set hoverlamp_mode 1
	}
}

proc kb_hoverlamp { } {
	global hoverlamp_on
	global cell_hover

	if { $hoverlamp_on } {
		default_intensity 1
		set cell_hover [list]
		set hoverlamp_on 0
	} else {
		default_intensity 0.7
		set cell_hover { cell_hover_1 }
		set hoverlamp_on 1
	}
}

proc kb_nextboard { } {
	global current_board
	incr current_board
	draw_board [ lindex [ get_boards ] [expr $current_board % [llength [get_boards]]] ]
}

proc kb_schematic { } {
	schem_sticky -toggle
}

proc enclosed_box1 { words } {
	set xmin 1000000
	set xmax 0
	set ymin $xmin
	set ymax 0
	foreach w $words {
		set minx [get_prop xmin $w]
		set miny [get_prop ymin $w]
		set maxx [get_prop xmax $w]
		set maxy [get_prop ymax $w]
		
		if { $minx < $xmin } { set xmin $minx }
		if { $miny < $ymin } { set ymin $miny }
		if { $maxx > $xmax } { set xmax $maxx }
		if { $maxy > $ymax } { set ymax $maxy }
	}
	return [list $xmin $ymin $xmax $ymax]
}

proc distance_to_box { box word } {
	
}

proc is_orthogonal { angle dev } {
	set aa [ expr { abs($angle) } ]

	for { set i 0 } { $i < 3 } { incr i } {
		if { $aa < $dev } { return 1 }
		set aa [expr { $aa - 90 } ]
		if { $aa < 0 } { return 0 }
	}
	return 0
}

proc highlight_marked {} {
	foreach o [ get_schem_words -filter { !$highlighted && $marked } ] {
		highlight -colorindex [get_prop mark $o] $o
	}
}

proc schem_find_pins_simple { page pin radius } {
	foreach cw [ get_schem_words -page $page -filter { $is_cell } ] {
		set_prop genmark [ generate_mark ] $cw
	}
	set last_dud 0
	while { 1 } {
		set dud [ impl_schem_find_pins_simple $page $pin $radius ]
		if { $dud == $last_dud } { break }
		set last_dud $dud
	}
}

proc find_orthogonal { words maxdist } {
	#prop_begin -mark
	#set_prop mark 0 $words
	foreach w $words {
		#puts "kkkkkkkkkkkkkkkkkkkk $w"
		if { ! [ get_prop marked $w] } { set_prop mark [generate_mark] $w }
		set n 1
		set ww2 [ get_schem_words -of $words -ref $w -filter { [ angle -ortho $r $_ ] < 5 } -sort { [ distance $a $r ] < [ distance $b $r ] } ]
		#puts "len [llength $ww2]"
		foreach w2 $ww2 {
			if { [ same_object $w $w2 ] } continue
			#puts "$w $w2 $words ---- $ww2 "
			if { ! [get_prop marked $w2] } {
				set ok 0
				if { [distance $w $w2] < [ expr 10 * $n ] && [ expr { [distance $w $w2] * [ angle -ortho $w $w2 ] } ] < 30 } {
					set_prop mark [ get_prop mark $w ] $w2
					set ok 1
				}
				#puts "[angle -ortho $w $w2] [ distance $w $w2 ] $w $w2 $n $ok"
				incr n
			}
		}
	}
}

proc impl_schem_find_pins_simple { page pin radius } {
	set dud 0
	set allcells [ get_schem_words -page $page -filter { $is_cell } ]
	foreach w [ get_schem_words -page $page -filter { !$marked } $pin ] {
		set cells [ get_schem_words -ref $w -radius $radius -of $allcells -filter { ! $marked } -sort { [ distance $a $r ] < [distance $b $r ] } ]
		if { [ llength $cells ] == 1 } {
			set_prop mark [ get_prop genmark $cells ] $cells
			set_prop mark [ get_prop genmark $cells ] $w
			#highlight -color "\#00ff00" $w
			#highlight -color "\#00ff00" $cells
			#puts "$cells $pin"
		} elseif { [ llength $cells ] } {
			set d0 [ distance [ lindex $cells 0 ] $w ]
			set d1 [ distance [ lindex $cells 1 ] $w ]
			if { [ expr $d1 / $d0 > 1.5 ] } {
				set gm [ get_prop genmark [lindex $cells 0] ]
				set_prop mark $gm [lindex $cells 0]
				set_prop mark $gm $w
				#highlight -color "\#00ff00" $w
				#highlight -color "\#00ff00" [lindex $cells 0]
				#puts "[lindex $cells 0] $pin"
			} else {
				incr dud
			}
		}
	}
	return $dud
}

proc schem_find_pins_simple_obsolete1 { cell } {
	set ref [ get_schem_words -of $cell -filter { !$marked } ]
	if { [ llength $ref ] != 1 } return
	set page [ get_prop page $ref ]

	set found [list]
	set numpins [ get_prop pins $cell ]
	foreach p [ lsort -integer -decreasing [ get_prop name [ get_pins -of $cell ]]] {
		set cand [ get_schem_words -filter { ! $marked } -page $page -ref $ref -radius [ expr $numpins * 10 ] $p ]
		if { [ llength $cand ] != 1 } return
		set found [ concat $found $cand ]
	}
	if { [ llength $found ] > 0 && [ llength $found ] == $numpins } {
		#puts "$cell $found"
		#report_prop -verbose $found
		#highlight -color "\#00ff00" $found
		set_prop mark $mark $found
	}
}

proc schem_find_pins_shaped { cell mark } {
	set ref [ get_schem_words -of $cell -filter { !$marked } ]
	if { [ llength $ref ] != 1 } return
	set page [ get page $ref ]
	set found [list]
	set numpins [ get pins $cell ]
	puts "$ref $numpins"
	#puts "[ info frame ]"
	#for { set fi 0 } { $fi < [ info frame ] } { incr fi } {
	#	puts [info frame $fi]
	#}
	foreach p [ get_pins -of $cell -sort { $a_name > $b_name } ] {
		set a_match_max -1
		set a_match_max_mindist 10000
		set a_match_max_word ""
		foreach w [ get_schem_words -ref $ref -radius [ expr $numpins * 10 ] -sort { [ distance $a $r ] < [ distance $b $r ] } -page $page -filter { !$marked } $p ] {
			set a_match 0
			set a_match_mindist 10000
			foreach ww $found {
				set ang [ angle -ortho $w $ww]
				#if { $w eq "6" } {
				#	puts "ortho $ang $ww"
				#}
				if { $ang < 5 } {
					incr a_match
					set d [ distance $ww $w ]
					if { $d < $a_match_mindist } {
						set a_match_mindist $d
					}
				}
			}
			#if { $w eq "6" } {
			#	puts "candid $w angle match $a_match mindist $a_match_mindist"
			#}
			# if { $a_match > $a_match_max || ($a_match == $a_match_max && $a_match_mindist < $a_match_max_mindist) } {}
			if { $a_match / $a_match_mindist > $a_match_max / $a_match_max_mindist } {
				set a_match_max $a_match
				set a_match_max_mindist $a_match_mindist
				set a_match_max_word $w
				#puts "candidate $w angle match $a_match mindist $a_match_mindist"
			}
		}
		set found [ linsert $found end $a_match_max_word ]
		#puts "[llength $found] [lindex $found 0]"
	}
	set_prop mark $mark $found
	#highlight -color "#00ff00" $found
	return $found
}

proc schem_interpage_nets { page } {
	set ret [ list ]
	foreach nw [ get_schem_words -page $page -filter { $is_net } ] {
		set pages [ lsort -unique [ get page [ get_schem_words -filter " \$page != $page " -- $nw ] ] ]
		if { [ llength $pages ] > 0 } {
			lappend ret $nw
		}
	}
	return $ret
}
		
proc schem_find_pins { page } {
	foreach cw [ get_schem_words -page $page -filter { $is_cell } ] {
		set_prop genmark [ generate_mark ] $cw
	}
	foreach c [ get_cells -filter { $pins > 4 } -sort { $a_pins > $b_pins } -of [ get_schem_words -page $page -filter { $is_cell } ] ] {
		schem_find_pins_shaped $c [ get genmark [ get_schem_words -of $c ] ]
		puts "$c [ get pins $c ]"
	}
	for { set pins 4 } { $pins > 0 } { incr pins -1 } {
		puts " find pins $pins "
		schem_find_pins_simple $page $pins 50

		set l1 [ llength [ get_schem_words -page $page -filter { $marked } $pins ] ]
		set_prop mark 0 [ get_schem_words -page $page -filter { $is_cell } ]
		set l2 [ llength [ get_schem_words -page $page -filter { $marked } $pins ] ]
		puts "$l1 $l2"
	}
}

proc schem_find_pin_labels { cell pins } {
	if { ! [llength $pins] } {
		set pins [ schem_find_pins $cell ]
	}
	set b [ enclosed_box $pins ]
	foreach w [ get_schem_words -bbox $b ] {
		if { ! ($w in $pins) } {
			puts $w
		}
	}
}

proc D_preload_schem_pages { quality } {
	foreach sch [ get_schematics ] {
		set pages [ get pages $sch ]

		for { set p 1 } { $p <= $pages } { incr p } {
			draw_page -cache -quality $quality -of $sch $p
		}
	}
}

proc on_schematic_open {} {
}

proc cell_select_event { } {
	if { [ llength [ selection ] ] == 0 } {
		undraw_page
	} else {
		set c [lindex [selection] 0]
		set page [lindex [lindex [get_prop schem_pages $c] 0 ] 0]
		draw_page -quality 0.5 $page [get_cells $c]
		schem_highlight [lrange [ lindex [get_prop schem_pages $c ] 0 ] 1 end]
	}
	#if { [ llength [ selection ] ] == 1 } {
	#	kb_schematic
	#}
}

proc r {} {
	find_vrm [get_cells -filter { $pins > 100 }]
	#adjacent_cells [lindex [get_cells] 8]
}

proc r2 {} {
	schem_find_pins [ get_cells PU7201 ]
	enclosed_box [ get_schem_words -color "#9900ff00" ]
}

proc r3 {} {
	GDFactory_construct GD
	set i [ GD create www 1000 1000 ]
	set col [ $i allocate_color 0 0 0 ]
	set col2 [ $i allocate_color 255 255 255 ]
	puts "$col $col2"
	$i line 0 0 100 100 $col2
	set fp [ open "test111.jpeg" w ]
	puts $fp [ $i jpeg_data 1 ]
}

proc RE {} {
	source library.tcl
}

#dual_draw 0
