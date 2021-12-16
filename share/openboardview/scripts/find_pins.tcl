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

proc impl_schem_find_pins { page } {
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
