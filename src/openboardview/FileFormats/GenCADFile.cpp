#include "GenCADFile.h"
#include "GenCADFileBnf.h"

#include "utils.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

bool GenCADFile::verifyFormat(const std::vector<char> &buf) {
	return find_str_in_buf("GENCAD", buf) && find_str_in_buf("$BOARD", buf);
}

GenCADFile::GenCADFile(const std::vector<char> &buf) {
	valid = parse_file(buf);
}

bool GenCADFile::parse_file(const std::vector<char> &buf) {
	bool ret = false;
#define X(CVAR, NAME) mpc_parser_t *CVAR = mpc_new((NAME));
	X_MACRO_PARSE_VARS
#undef X

#define X(CVAR, NAME) CVAR,
	mpc_err_t *language_error = mpca_lang(MPCA_LANG_WHITESPACE_SENSITIVE, kGenCadFileBnf, X_MACRO_PARSE_VARS NULL);
#undef X
	try {
		if (language_error != nullptr) {
			std::string parser_error("Failed to parse GenCAD file, parser reported:\n");
			parser_error.append(mpc_err_string(language_error));
			mpc_err_delete(language_error);
			throw parser_error;
		}

		mpc_result_t r;
		mpc_ast_t *ast = nullptr;
		ret            = true;
		if (mpc_nparse("", buf.data(), buf.size(), gencad_file, &r)) {
			ast = static_cast<mpc_ast_t *>(r.output);
			if (!ast) throw std::string("Failed to parse GenCAD file: the file does not match the GenCAD format specification");

			header_ast = mpc_ast_get_child(ast, "header|>");
			if (!header_ast) throw std::string("Failed to parse GenCAD file: the $HEADER section was not parsed properly");

			board_ast = mpc_ast_get_child(ast, "board|>");
			if (!board_ast) throw std::string("Failed to parse GenCAD file: the $BOARD section was not parsed properly");

			pads_ast = mpc_ast_get_child(ast, "pads|>");
			if (!pads_ast) throw std::string("Failed to parse GenCAD file: the $PADS section was not parsed properly");

			padstacks_ast = mpc_ast_get_child(ast, "padstacks|>");
			if (!padstacks_ast) throw std::string("Failed to parse GenCAD file: the $PADSTACKS section was not parsed properly");

			shapes_ast = mpc_ast_get_child(ast, "shapes|>");
			if (!shapes_ast) throw std::string("Failed to parse GenCAD file: the $SHAPES section was not parsed properly");

			components_ast = mpc_ast_get_child(ast, "components|>");
			if (!components_ast) throw std::string("Failed to parse GenCAD file: the $COMPONENTS section was not parsed properly");

			devices_ast = mpc_ast_get_child(ast, "devices|>");
			if (!devices_ast) throw std::string("Failed to parse GenCAD file: the $DEVICES section was not parsed properly");

			signals_ast = mpc_ast_get_child(ast, "signals|>");
			if (!signals_ast) throw std::string("Failed to parse GenCAD file: the $SIGNALS section was not parsed properly");

			tracks_ast = mpc_ast_get_child(ast, "tracks|>");
			if (!tracks_ast) throw std::string("Failed to parse GenCAD file: the $TRACKS section was not parsed properly");

			layers_ast = mpc_ast_get_child(ast, "layers|>");
			//if (!layers_ast) throw std::string("Failed to parse GenCAD file: the $LAYERS section was not parsed properly");
			// $LAYERS section is optional and doesn't have to exist in a GenCAD file

			routes_ast = mpc_ast_get_child(ast, "routes|>");
			if (!routes_ast) throw std::string("Failed to parse GenCAD file: the $ROUTES section was not parsed properly");

			parse_dimension_units(header_ast);
			parse_board_outline(board_ast);
			parse_components();
			parse_vias();

			for (auto i = 1; i <= 2; i++) { // Add dummy parts for probe points on both sides
				BRDPart part;
				part.name = "...";
				part.mounting_side =
				    (i == 1 ? BRDPartMountingSide::Bottom : BRDPartMountingSide::Top); // First part is bottom, last is top.
				part.end_of_pins = 0;                                                  // Unused
				parts.push_back(part);
			}

			AddNailsAsPins();

		} else {
			std::string parser_error("Failed to parse GenCAD file, parser reported:\n");
			parser_error.append(mpc_err_string(r.error));
			mpc_err_delete(r.error);
			throw parser_error;
		}
	} catch (std::string &error) {
		ret       = false;
		error_msg = error;
		error_msg.append(
		    "\nIf you think your GenCAD file is correct please report an issue here:\n"
		    "https://github.com/OpenBoardView/OpenBoardView/issues\n");
	}

#define X(CVAR, NAME) CVAR,
	mpc_cleanup(PARSE_VARS_COUNT, X_MACRO_PARSE_VARS NULL);
#undef X
	return ret;
}

bool GenCADFile::parse_vias() {
	for (int i = 0; i >= 0;) {
		i = mpc_ast_get_index_lb(routes_ast, "route|>", i);
		if (i >= 0) {
			auto route_ast = mpc_ast_get_child_lb(routes_ast, "route|>", i);
			if (!parse_route_vias(route_ast)) return false;
			i++;
		}
	}
	return true;
}

bool GenCADFile::parse_route_vias(mpc_ast_t *route_ast) {
	char *route_name = get_nonquoted_or_quoted_string_child(route_ast, "sig_name");
	if (!route_name) return false;

	for (int i = 0; i >= 0;) {
		i = mpc_ast_get_index_lb(route_ast, "via|>", i);
		if (i >= 0) {
			mpc_ast_t *via_ast = mpc_ast_get_child_lb(route_ast, "via|>", i);
			if (via_ast) {
				mpc_ast_t *pos_ast = mpc_ast_get_child(via_ast, "x_y_ref|>");
				if (pos_ast) {
					BRDNail nail{};
					nail.side  = BRDPartMountingSide::Both;
					nail.net   = route_name;
					nail.probe = 1; // WTF
					x_y_ref_to_brd_point(pos_ast, &nail.pos);
					nails.push_back(nail);
					num_nails++;
				}
			}
			i++;
		}
	}
	return true;
}

bool GenCADFile::parse_components() {
	fill_signals_cache();
	for (int i = 0; i >= 0;) {
		i = mpc_ast_get_index_lb(components_ast, "component|>", i);
		if (i >= 0) {
			mpc_ast_t *component_ast = mpc_ast_get_child_lb(components_ast, "component|>", i);

			BRDPart brd_part;
			char *component_name = get_nonquoted_or_quoted_string_child(component_ast, "component_name");
			if (component_name) {
				brd_part.name = component_name;
			}

			mpc_ast_t *place_ast = mpc_ast_get_child(component_ast, "place|>");
			if (place_ast) {
				mpc_ast_t *pos_ast = mpc_ast_get_child(place_ast, "x_y_ref|>");
				if (pos_ast) {
					x_y_ref_to_brd_point(pos_ast, &brd_part.p1);
					x_y_ref_to_brd_point(pos_ast, &brd_part.p2);
				}
			}

			double component_rotation_angle = 0.0;
			mpc_ast_t *rotation_ast         = mpc_ast_get_child(component_ast, "rotation|>");
			mpc_ast_t *rotation_value_ast   = mpc_ast_get_child(rotation_ast, "rot|number|regex");
			if (rotation_ast && rotation_value_ast) {
				component_rotation_angle = atof(rotation_value_ast->contents);
			}

			mpc_ast_t *layer_ast = mpc_ast_get_child(component_ast, "named_layer|>");
			if (layer_ast) {
				mpc_ast_t *layer_index_ast = mpc_ast_get_child(layer_ast, "layer_name|nonquoted_string|regex");
				if (has_text_content(layer_index_ast, "TOP")) {
					brd_part.mounting_side = BRDPartMountingSide::Top;
				} else if (has_text_content(layer_index_ast, "BOTTOM")) {
					brd_part.mounting_side = BRDPartMountingSide::Bottom;
				}
			}

			mpc_ast_t *component_device_ast = mpc_ast_get_child(component_ast, "device_|>");
			if (component_device_ast) {
				char *dev_name = get_stringtoend_child(component_device_ast, "device_name");
				// workaround for RSI-TRANSLATOR CAMCAD bad COMPONENT -> DEVICE references
				std::replace(dev_name, dev_name + strlen(dev_name), ' ', '_');
				if (dev_name) brd_part.mfgcode = dev_name;
			}

			mpc_ast_t *shape_ref_ast = mpc_ast_get_child(component_ast, "shape_|>");
			if (shape_ref_ast) {
				char *shape_name_str = get_nonquoted_or_quoted_string_child(shape_ref_ast, "shape_name");
				if (shape_name_str && *shape_name_str) {
					if (!brd_part.mfgcode.empty()) {
						brd_part.mfgcode += " SHAPE ";
					}
					brd_part.mfgcode += shape_name_str;
					mpc_ast_t *shape_ast = get_shape_by_name(shape_name_str);
					if (shape_ast) {
						mpc_ast_t *mirror_ast = mpc_ast_get_child(shape_ref_ast, "mirror|string");
						bool mirror_x         = has_text_content(mirror_ast, "MIRRORX");
						bool mirror_y         = has_text_content(mirror_ast, "MIRRORY");
						brd_part.part_type    = is_shape_smd(shape_ast) ? BRDPartType::SMD : BRDPartType::ThroughHole;
						parse_shape_pins_to_component(&brd_part, component_rotation_angle, mirror_x, mirror_y, shape_ast);
						if ( brd_part.part_type == BRDPartType::ThroughHole ) {
							brd_part.mounting_side = BRDPartMountingSide::Both;
						}
						brd_part.end_of_pins = num_pins - 1;
					}
				}
			}
			parts.push_back(brd_part);
			num_parts++;
			i++;
		}
	}
	return true;
}

bool GenCADFile::parse_shape_pins_to_component(
    BRDPart *part, double rotation_in_degrees, bool mirror_x, bool mirror_y, mpc_ast_t *shape_ast) {
	double rotation_in_rads = (rotation_in_degrees * (M_PI / 180.0));
	int mirror_x_sign       = mirror_x ? (-1) : 1;
	int mirror_y_sign       = mirror_y ? (-1) : 1;
	if (mirror_x_sign * mirror_y_sign == -1) // exatly one mirror
	{
		rotation_in_rads = M_PI - rotation_in_rads;
	}
	double cos_ = cos(rotation_in_rads);
	double sin_ = sin(rotation_in_rads);

	for (int i = 0; i >= 0;) {
		// go through all pins of the current shape
		i = mpc_ast_get_index_lb(shape_ast, "shapes_pin|>", i);
		if (i >= 0) {
			mpc_ast_t *pin_ast = mpc_ast_get_child_lb(shape_ast, "shapes_pin|>", i);
			if (pin_ast) {
				mpc_ast_t *pos_ast      = mpc_ast_get_child(pin_ast, "x_y_ref|>");
				char *pin_name = get_nonquoted_or_quoted_string_child(pin_ast, "shape_pin_name");
				if (pos_ast && pin_name) {
					BRDPin pin;
					pin.radius = 0.5;
					mpc_ast_t *padstack_name_ast = mpc_ast_get_child(pin_ast, "pad_name|nonquoted_string|regex");
					mpc_ast_t *padstack_ast = 0;
					if (padstack_name_ast) {
						padstack_ast = get_padstack_by_name(padstack_name_ast->contents);
						// enable the code below once the pin.radius will be processed
						//if (padstack_ast)
						//	pin.radius = get_padstack_radius(padstack_ast);
					}

					// part is not yet added to the list at this point
					pin.part  = static_cast<unsigned int>(parts.size() + 1);
					pin.pos.x = part->p1.x;
					pin.pos.y = part->p1.y;
					pin.snum  = pin_name;

					BRDPoint tmpPos{};
					x_y_ref_to_brd_point(pos_ast, &tmpPos);
					pin.pos.x += mirror_x_sign * (tmpPos.x * cos_ - tmpPos.y * sin_);
					pin.pos.y += mirror_y_sign * (tmpPos.x * sin_ + tmpPos.y * cos_);

					pin.net = get_signal_name_for_component_pin(part->name, pin_ast);
					if (!pin.net) {
						char *tmp = static_cast<char *>(malloc(32));
						sprintf(tmp, "NC@%d", nc_counter);
						pin.net = tmp;
						nc_counter++;
					}
					if (padstack_ast && !is_padstack_smd(padstack_ast)) {
						pin.side = BRDPinSide::Both;
					} else if (part->mounting_side == BRDPartMountingSide::Top) {
						pin.side = BRDPinSide::Top;
					} else {
						pin.side = BRDPinSide::Bottom;
					}
					pins.push_back(pin);
					num_pins++;
				}
			}
			i++;
		}
	}
	return true;
}

void GenCADFile::fill_signals_cache() {
	for (int i = 0;;) {
		i = mpc_ast_get_index_lb(signals_ast, "signal|>", i);
		if (i < 0) break;
		mpc_ast_t *signal_ast = mpc_ast_get_child_lb(signals_ast, "signal|>", i);
		for (int j = 0;;) {
			j = mpc_ast_get_index_lb(signal_ast, "node|>", j);
			if (j < 0) break;
			mpc_ast_t *node_ast           = mpc_ast_get_child_lb(signal_ast, "node|>", j);
			char *node_comp_name = get_nonquoted_or_quoted_string_child(node_ast, "component_name");
			char *node_pin_name = get_nonquoted_or_quoted_string_child(node_ast, "pin_name");
			char *signal_name = get_nonquoted_or_quoted_string_child(signal_ast, "sig_name");
			if (node_comp_name && node_pin_name && signal_name) {
				ComponentPin key     = {node_comp_name, node_pin_name};
				m_signals_cache[key] = signal_name;
			}
			j++;
		}
		i++;
	}
}

const char *GenCADFile::get_signal_name_for_component_pin(const char *component_name, mpc_ast_t *pin_ast) {
	char *pin_name = get_nonquoted_or_quoted_string_child(pin_ast, "shape_pin_name");
	if (!pin_name) return nullptr;

	ComponentPin key = {component_name, pin_name};

	auto found_pin = m_signals_cache.find(key);
	if (found_pin != m_signals_cache.end()) {
		return found_pin->second.c_str();
	}
	return nullptr;
}

double GenCADFile::distance(BRDPoint &p1, BRDPoint &p2) {
	return sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2));
}

bool GenCADFile::parse_dimension_units(mpc_ast_t *header_ast) {
	mpc_ast_t *units = mpc_ast_get_child(header_ast, "units|>");
	m_dimension      = INVALID;
	if (!units) return false;

	if (units->children_num == 4) {
		// Try UNITS <unit> format
		mpc_ast_t *unit = mpc_ast_get_child(units, "unit|dimension|string");
		if (unit) {
			if (has_text_content(unit, "INCH")) {
				m_dimension = INCH;
			} else if (has_text_content(unit, "THOU")) {
				m_dimension = THOU;
			} else if (has_text_content(unit, "MM")) {
				m_dimension = MM;
			} else if (has_text_content(unit, "MM100")) {
				m_dimension = MM100;
			}
			return true;
		}

		// UNITS <unit> does not match -> try UNITS <unit> <dimension_unit>
		unit = mpc_ast_get_child(units, "unit|dimension|>");
		if (unit->children_num == 3) {
			mpc_ast_t *start_child = unit->children[0];
			if (has_text_content(start_child, "USER")) {
				m_dimension = USER;
			} else if (has_text_content(start_child, "USERM")) {
				m_dimension = USERCM;
			} else if (has_text_content(start_child, "USERMM")) {
				m_dimension = USERMM;
			}

			if (m_dimension != INVALID) {
				m_dimension_unit = atoi(unit->children[2]->contents);
				return true;
			}
		}
	}

	return false;
}

bool GenCADFile::parse_board_outline(mpc_ast_t *board_ast) {
	for (int i = 0; i < board_ast->children_num; i++) {
		if (strcmp(board_ast->children[i]->tag, "line|>") == 0) {
			mpc_ast_t *line = board_ast->children[i];
			if (line) {
				mpc_ast_t *lref = mpc_ast_get_child(line, "line_ref|>");
				if (!lref) continue;
				mpc_ast_t *p1_ast = mpc_ast_get_child(lref, "line_start|x_y_ref|>");
				mpc_ast_t *p2_ast = mpc_ast_get_child(lref, "line_end|x_y_ref|>");
				if (!p1_ast || !p2_ast) continue;

				BRDPoint p1{}, p2{};
				if (x_y_ref_to_brd_point(p1_ast, &p1)) {
					format.push_back(p1);
					num_format++;
				}

				if (x_y_ref_to_brd_point(p2_ast, &p2)) {
					format.push_back(p2);
					num_format++;
				}
			}
		} else if (strcmp(board_ast->children[i]->tag, "arc|>") == 0) {
			mpc_ast_t *arc = board_ast->children[i];
			if (arc) {
				mpc_ast_t *aref = mpc_ast_get_child(arc, "arc_ref|>");
				if (!aref) continue;
				mpc_ast_t *start  = mpc_ast_get_child(aref, "arc_start|x_y_ref|>");
				mpc_ast_t *stop   = mpc_ast_get_child(aref, "arc_end|x_y_ref|>");
				mpc_ast_t *center = mpc_ast_get_child(aref, "arc_center|x_y_ref|>");
				// TODO add support for ellipse arcs (which have arc_p1 and arc_p2)
				if (!start || !stop || !center) continue;
				add_arc_to_outline(start, stop, center);
			}
		}
	}

	for (int i = 0; i >= 0;) {
		i = mpc_ast_get_index_lb(board_ast, "rectangle|>", i);
		if (i >= 0) {
			mpc_ast_t *rectangle = mpc_ast_get_child_lb(board_ast, "rectangle|>", i);
			if (!rectangle) continue;
			mpc_ast_t *x_ast = mpc_ast_get_child(rectangle, "x|number|regex");
			mpc_ast_t *y_ast = mpc_ast_get_child(rectangle, "y|number|regex");
			mpc_ast_t *w_ast = mpc_ast_get_child(rectangle, "width|number|regex");
			mpc_ast_t *h_ast = mpc_ast_get_child(rectangle, "height|number|regex");
			if (!x_ast || !y_ast || !w_ast || !h_ast) continue;

			BRDPoint p;

			p.x = board_unit_to_brd_coordinate(atof(x_ast->contents));
			p.y = board_unit_to_brd_coordinate(atof(y_ast->contents));
			format.push_back(p);

			p.y += board_unit_to_brd_coordinate(atof(h_ast->contents));
			format.push_back(p);

			p.x += board_unit_to_brd_coordinate(atof(w_ast->contents));
			format.push_back(p);

			p.y -= board_unit_to_brd_coordinate(atof(h_ast->contents));
			format.push_back(p);

			num_format += 4;
			i++;
		}
	}
	return true;
}

void GenCADFile::add_arc_to_outline(mpc_ast_t *start, mpc_ast_t *stop, mpc_ast_t *center) {
	BRDPoint p1{}, p2{}, pc{};
	if (!x_y_ref_to_brd_point(start, &p1)) return;

	if (!x_y_ref_to_brd_point(stop, &p2)) return;

	if (!x_y_ref_to_brd_point(center, &pc)) return;

	double r     = distance(p1, pc);
	double delta = distance(p1, p2);

	double startAngle = asin((p1.y - pc.y) / r);
	if (p1.y == pc.y && p1.x < pc.x) startAngle = M_PI;

	double endAngle = startAngle + (asin((delta / 2.0) / r) * 2);

	bool inverseOrder = false;
	if (format.size()) {
		double p1toLast = distance(p1, format.back());
		double p2toLast = distance(p2, format.back());
		if (p2toLast < p1toLast) inverseOrder = true;
	}

	if (!inverseOrder) {
		for (double i = startAngle; i < endAngle; i += arc_slice_angle_rad) {
			BRDPoint p{};
			p.x = pc.x + r * cos(i);
			p.y = pc.y + r * sin(i);
			format.push_back(p);
			num_format++;
		}
		format.push_back(p2);
		num_format++;
	} else {
		for (double i = endAngle; i > startAngle; i -= arc_slice_angle_rad) {
			BRDPoint p{};
			p.x = pc.x + r * cos(i);
			p.y = pc.y + r * sin(i);
			format.push_back(p);
			num_format++;
		}
		format.push_back(p1);
		num_format++;
	}
}

bool GenCADFile::x_y_ref_to_brd_point(mpc_ast_t *x_y_ref, BRDPoint *point) {
	if (x_y_ref->children_num == 3) {
		point->x = board_unit_to_brd_coordinate(strtod(x_y_ref->children[0]->contents, nullptr));
		point->y = board_unit_to_brd_coordinate(strtod(x_y_ref->children[2]->contents, nullptr));
		return true;
	}
	return false;
}

mpc_ast_t *GenCADFile::get_shape_by_name(const char *name) {
	size_t name_length = strlen(name);
	for (int i = 0; i >= 0;) {
		i = mpc_ast_get_index_lb(shapes_ast, "shape|>", i);
		if (i >= 0) {
			mpc_ast_t *shape_ast = mpc_ast_get_child_lb(shapes_ast, "shape|>", i);
			if (!shape_ast) continue;

			char *shape_name = get_nonquoted_or_quoted_string_child(shape_ast, "shape_name");
			if (shape_name && (strcmp(shape_name, name) == 0) && strlen(shape_name) == name_length) {
				return shape_ast;
			}
			i++;
		}
	}
	return nullptr;
}

int GenCADFile::board_unit_to_brd_coordinate(double brdUnit) {
	// board coordinates in mil?
	switch (m_dimension) {
		case GenCADFile::INCH: return static_cast<int>(brdUnit * 1000.0);
		case GenCADFile::INVALID: // assume mil by default
		case GenCADFile::THOU: return static_cast<int>(brdUnit);
		case GenCADFile::MM: return static_cast<int>(brdUnit * (100.0 / 2.54));
		case GenCADFile::MM100: return static_cast<int>(brdUnit * (1.0 / 2.54));
		case GenCADFile::USER: return static_cast<int>((m_dimension_unit * brdUnit) / 1000);
		case GenCADFile::USERCM: return static_cast<int>(m_dimension_unit * brdUnit * (10.0 / 2.54));
		case GenCADFile::USERMM: return static_cast<int>(m_dimension_unit * brdUnit * (1.0 / 2.54));
	}
	return static_cast<int>(brdUnit);
}

bool GenCADFile::is_shape_smd(mpc_ast_t *shape_ast) {
	for (int i = 0; i >= 0;) {
		i = mpc_ast_get_index_lb(shape_ast, "shapes_pin|>", i);
		if (i >= 0) {
			mpc_ast_t *pin_ast = mpc_ast_get_child_lb(shape_ast, "shapes_pin|>", i);
			if (!pin_ast) continue;

			char *pad_name = get_nonquoted_or_quoted_string_child(pin_ast, "pad_name");
			if (!pad_name) continue;

			mpc_ast_t *padstack_ast = get_padstack_by_name(pad_name);
			if (padstack_ast && !is_padstack_smd(padstack_ast)) {
				return false;
			}
			i++;
		}
	}
	return true;
}

char *GenCADFile::get_nonquoted_or_quoted_string_child(mpc_ast_t *parent, const char *name) {
	char *key = static_cast<char *>(malloc(strlen(name) + 25));
	sprintf(key, "%s|nonquoted_string|regex", name);
	mpc_ast_t *ret_ast = mpc_ast_get_child(parent, key);
	if (ret_ast) {
		free(key);
		return ret_ast->contents;
	}

	sprintf(key, "%s|string|>", name);
	ret_ast = mpc_ast_get_child(parent, key);
	free(key);
	if (ret_ast) {
		auto value_ast = mpc_ast_get_child(ret_ast, "regex");
		if (value_ast) return value_ast->contents;
	}
	return nullptr;
}

bool GenCADFile::has_text_content(mpc_ast_t *content_holder, const char *text) {
	return content_holder != nullptr && (strcmp(content_holder->contents, text) == 0);
}

char *GenCADFile::get_stringtoend_child(mpc_ast_t *parent, const char *name) {
	char *key = static_cast<char *>(malloc(strlen(name) + 50));
	sprintf(key, "%s|wrapper_to_end|string_to_end|regex", name);
	mpc_ast_t *ret_ast = mpc_ast_get_child(parent, key);
	if (ret_ast) {
		free(key);
		return ret_ast->contents;
	}

	sprintf(key, "%s|wrapper_to_end|string|>", name);
	ret_ast = mpc_ast_get_child(parent, key);
	if (ret_ast) {
		auto value_ast = mpc_ast_get_child(ret_ast, "regex");
		if (value_ast) {
			free(key);
			return value_ast->contents;
		}
	}

	sprintf(key, "%s|string|>", name);
	ret_ast = mpc_ast_get_child(parent, key);
	if (ret_ast) {
		auto value_ast = mpc_ast_get_child(ret_ast, "regex");
		if (value_ast) {
			free(key);
			return value_ast->contents;
		}
	}
	free(key);
	return nullptr;
}

bool GenCADFile::is_padstack_smd(mpc_ast_t *padstack_ast) {
	mpc_ast_t *drill_size_ast = mpc_ast_get_child(padstack_ast, "drill_size|number|regex");
	if (drill_size_ast) return atof(drill_size_ast->contents) == 0.0;
	return false;
}

mpc_ast_t *GenCADFile::get_padstack_by_name(const char *padstack_name_wanted) {
	for (int i = 0; i >= 0;) {
		i = mpc_ast_get_index_lb(padstacks_ast, "padstack|>", i);
		if (i >= 0) {
			mpc_ast_t *padstack_ast = mpc_ast_get_child_lb(padstacks_ast, "padstack|>", i);
			if (!padstack_ast) continue;

			char *padstack_name = get_nonquoted_or_quoted_string_child(padstack_ast, "pad_name");
			if (padstack_name && (strcmp(padstack_name, padstack_name_wanted) == 0)) {
				return padstack_ast;
			}
			i++;
		}
	}
	return nullptr;
}

mpc_ast_t *GenCADFile::get_pad_by_name(const char *pad_name_wanted) {
	for (int i = 0; i >= 0;) {
		i = mpc_ast_get_index_lb(pads_ast, "pad|>", i);
		if (i >= 0) {
			mpc_ast_t *pad_ast = mpc_ast_get_child_lb(pads_ast, "pad|>", i);
			if (!pad_ast) continue;

			char *pad_name = get_nonquoted_or_quoted_string_child(pad_ast, "pad_name");
			if (pad_name && (strcmp(pad_name, pad_name_wanted) == 0)) {
				return pad_ast;
			}
			i++;
		}
	}
	return nullptr;
}

double GenCADFile::get_padstack_radius(mpc_ast_t *padstack_ast) {
	double radius = 0.5;
	// loop through all pads in a padstack and return with the greatest size
	for (int i = 0; i >= 0;) {
		i = mpc_ast_get_index_lb(padstack_ast, "padstacks_pad|>", i);
		if (i >= 0) {
			mpc_ast_t *pad_ref_ast = mpc_ast_get_child_lb(padstack_ast, "padstacks_pad|>", i);
			if (!pad_ref_ast) continue;

			char *pad_name = get_nonquoted_or_quoted_string_child(pad_ref_ast, "pad_name");
			if (pad_name) {
				mpc_ast_t *pad_ast = get_pad_by_name(pad_name);
				double pad_radius  = get_pad_radius(pad_ast);
				if (pad_radius > radius) radius = pad_radius;
			}
			i++;
		}
	}
	return radius;
}

double GenCADFile::get_pad_radius(mpc_ast_t *pad_ast) {
	double radius    = 0.5;
	double min_x     = std::numeric_limits<double>::max();
	double max_x     = std::numeric_limits<double>::min();
	double min_y     = std::numeric_limits<double>::max();
	double max_y     = std::numeric_limits<double>::min();
	bool limit_found = false;

	// look for circles
	for (int i = 0; i >= 0;) {
		i = mpc_ast_get_index_lb(pad_ast, "circle|>", i);
		if (i >= 0) {
			mpc_ast_t *circle_ast = mpc_ast_get_child_lb(pad_ast, "circle|>", i);
			if (!circle_ast) continue;
			i++;
			mpc_ast_t *circle_ref_ast = mpc_ast_get_child(circle_ast, "circle_ref|>");
			if (circle_ref_ast) {
				auto radius_ast = mpc_ast_get_child(circle_ref_ast, "radius|number|regex");
				if (radius_ast) {
					auto radius = atoi(radius_ast->contents);
					if (-radius < min_x) min_x = -radius;
					if (radius > max_x) max_x = radius;
					if (-radius < min_y) min_y = -radius;
					if (radius > max_y) max_y = radius;

					limit_found = true;
				}
			}
		}
	}

	if (limit_found) {
		double width  = (max_x - min_x);
		double height = (max_y - min_y);
		radius        = board_unit_to_brd_coordinate(width > height ? width : height);
	}

	return radius;
}
