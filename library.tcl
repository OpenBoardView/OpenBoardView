package require struct::set
package require Thread

set trace_frame 0

#source "share/openboardview/scripts/find_vrm.tcl"
#source "share/openboardview/scripts/find_pins.tcl"

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

proc draw_nodes { { top_bot -top } } {
	foreach cell [ get_cells $top_bot ] {
		foreach pin [ get_pins -of $cell ] {
			set n [ get_nets -of $pin ]
			set allnearpins [ lrange [ get_pins -of $n -ref $pin -not $pin -notof $cell -filter { [ distance $r $_ ] < 200 } -sort { [ distance $a $r ] < [ distance $b $r ] }] 0 0 ]
			if { $n == "+VCCIOP_LX" } {
				#puts "$allnearpins $pin $cell"
			}
			foreach pp $allnearpins {
				#puts "$pp $cell $pin [ objinfo $pp ] [ objinfo $cell ] [ objinfo $pin ] [ llength [ get_cells -top -of $pp -not $cell ] ]"
				set num_cells [ llength [ get_cells $top_bot -of $pp -not $cell ] ]
				if { $num_cells } {
					#puts "add"
					add_node $pin $pp $n
				}
				if { $num_cells == 1 } {
					#highlight -intensity 0.5 $pp
				}
			}
			continue
			set alldistpins [ lrange [ get_pins -of $n -ref $pin -filter { [ distance $r $_] >= 200 } ] 0 10 ]
			foreach pp $alldistpins {
				if { [ llength [ get_cells $top_bot -of $pp -not $cell ] ] > 0 } {
					add_node -stub $pin $pp $n
				}
			}
		}
	}
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


proc remove_extra_type_letter { cell } {
	set c [ dup $cell ]
	set f [ string index $c 0 ]
	set r [ string range $c 1 end ]
	if { [ string first $f $r ] != -1 } {
		return $r
	}
	return $c
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

proc schem_hover_tooltip { page word } {
	set otherpages 0
	set samepage 0
	for { set pi 1 } { $pi < [ get pages [ get_schematics ] ] } { incr pi } {
		set n [ llength [ get_schem_words -page $pi -- $word ] ]
		if { $pi == $page } {
			set samepage $n
		} else {
			set otherpages [ expr $otherpages + ($n ? 1 : 0) ]
		}
	}
	if { $samepage > 0 && $otherpages} {
		return "$samepage +$otherpages"
	} elseif { $samepage > 1 } {
		return "$samepage"
	}
	return ""
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

proc draw_something { draw coord } {
	. $draw AddText $coord 4294967295 "this is a test" 
}

proc preload_schem_pages { quality { threads 0 } } {
	if { $threads == 0 } { set threads [ schem_render_concurrency ] }
	create_thread -detach {
		set start [ clock milliseconds ]
		foreach sch [ get_schematics ] {
			set pages [ get pages $sch ]
			set thr [list]

			for { set ti 1 } { $ti <= $threads } { incr ti } {
				lappend thr [ create_thread {
					for { set p $ti } { $p <= $pages } { incr p $threads } {
						draw_page -cache -quality $quality -of $sch $p
					}
				} [ list ti threads pages sch quality ] ]
			}
			join_thread $thr
		}
		puts "\nschematic preload took [ expr [ clock millisecond ] - $start ] ms"
	} [ list threads quality ]
	return ""
}

proc on_schematic_open {} {
}

proc schem_find_pins { page } {
	if { [ info commands impl_schem_find_pins ] == "" } {
		source "share/openboardview/scripts/find_pins.tcl"
	}
	impl_schem_find_pins $page
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
	source "share/openboardview/scripts/find_vrm.tcl"
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
