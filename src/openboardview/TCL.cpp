#include "TCL.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wparentheses"

//Tcl_ObjType TCL::net_objtype_, TCL::cell_objtype_, TCL::pin_objtype_, TCL::board_objtype_;
#if 0
decltype(TCL::cell_props_) TCL::cell_props_;
decltype(TCL::pin_props_) TCL::pin_props_;
decltype(TCL::net_props_) TCL::net_props_;
decltype(TCL::pdf_word_props_) TCL::pdf_word_props_;
#endif
//decltype(TCL::board_props_) TCL::board_props_;
TCL * TCL::this_s_;

char const ** TCL::pdf_word_props_ = nullptr,
	** TCL::cell_props_ = nullptr,
	** TCL::net_props_  = nullptr,
	** TCL::pin_props_  = nullptr;

bool TCL::static_initialized_ = false;

namespace OBV_Tcl {
	std::optional<std::map<std::string, object> > TCL::get_variables_from_expr(std::string const & expr, bool command, bool throw_on_error) {
		std::map<std::string, object> ret;

		Tcl_Parse tokens;

		auto typestr = [](int t) {
			if (t == TCL_TOKEN_WORD) return "word";
			else if (t == TCL_TOKEN_SIMPLE_WORD) return "simpleword";
			else if (t == TCL_TOKEN_EXPAND_WORD) return "expandword";
			else if (t == TCL_TOKEN_TEXT) return "text";
			else if (t == TCL_TOKEN_BS) return "bs";
			else if (t == TCL_TOKEN_COMMAND) return "command";
			else if (t == TCL_TOKEN_VARIABLE) return "variable";
			else if (t == TCL_TOKEN_SUB_EXPR) return "subexpr";
			else if (t == TCL_TOKEN_OPERATOR) return "operator";
			else return "(unknown)";
		};
		typestr(0);

		const bool dump = false;

		int tret;
		if (command) {
			tret = Tcl_ParseCommand(tcl, expr.c_str(), -1, 1, &tokens);
		} else {
			tret = Tcl_ParseExpr(tcl, expr.c_str(), -1, &tokens);
		}
				
		if (tret != TCL_OK) {
			//std::cerr << "cannot parse: " << expr << "\n";
			if (throw_on_error) {
				throw;
			} else {
				return { };
			}
		} else {
			if (dump) {
				std::cerr << "num tokens: " << tokens.numTokens << "\n";
			}
			for (int ti = 0; ti < tokens.numTokens; ++ti) {
				if (dump) {
					std::cerr << tokens.tokenPtr[ti].type << " " << typestr(tokens.tokenPtr[ti].type) << " "  << tokens.tokenPtr[ti].size << " " << tokens.tokenPtr[ti].numComponents << " ";
					std::cerr << std::string(tokens.tokenPtr[ti].start, tokens.tokenPtr[ti].size) << "\n";
				}
				auto & r = tokens.tokenPtr[ti];
				if (r.type == TCL_TOKEN_VARIABLE) {
					if (r.numComponents != 1) {
						std::cerr << "variable with >1 components\n";
						return { };
					}

					// !!!!!
					// object cannot be the default object(), because at this point the refcount of the
					// created object must be =1
					// !!!!!

					ret.emplace(std::string(tokens.tokenPtr[ti + 1].start, tokens.tokenPtr[ti + 1].size), object().duplicate());
				} else if (r.type == TCL_TOKEN_COMMAND) {
					if (r.numComponents != 0) {
						std::cerr << "command with >1 components\n";
						return { };
					}
					try {
						auto rv = get_variables_from_expr(std::string(r.start + 1, r.size - 1), true, true);
						if (rv) {
							for (auto & i : *rv) {
								ret.insert(i);
							}
						}
					} catch (...) {
						if (throw_on_error) throw;
						else {
							return { };
						}
					}
				}
			}
			Tcl_FreeParse(&tokens);
			return ret;
		}
	}

#ifdef OBV_USE_POPPLER
	object TCL::get_schem_words(getopt<int> const & page, getopt<std::string> const & filter_arg, getopt<bool> const & use_regex, getopt<pdf_txt_bbox *> const & ref, getopt<std::string> const & sort, getopt<list<pdf_txt_bbox> > const & not_, getopt<any<list<Component> , list<Pin>, list<Net>, list<pdf_txt_bbox> > > const & of, getopt<std::string> const & exact, getopt<std::string> const & color, getopt<list<pdf_txt_bbox> > const & bbox, getopt<float> const & radius, variadic<std::string> const & match) {
		object r;
		object filter_o; int filter = 0; std::string filter_ns;
		object sort_o; int sort_ix; std::string sort_ns;
		std::regex re;
		if (use_regex) re = match[0];

		if (radius && ! ref) {
			throw tcl_error("-radius requires -ref");
		}

		if (filter_arg) {
			filter = filter_ns_ix++ % 1000;
			filter_ns = namespace_str(filter);
			//namespace_open(*tcli, filter_ns);
			filter_o = object(std::string("namespace eval ") + filter_ns + " { expr { " + *filter_arg + " } }");

			//std::cerr << "filter vars: " << join(get_variables_from_expr(*filter_arg)) << "\n";
		}
		auto filt_nshold = make_namespace_holder(filter_ns);
		
		if (sort) {
			sort_ix = sort_ns_ix++ % 1000;
			sort_ns = sort_namespace_str(sort_ix);
			//namespace_open(*tcli, sort_ns);
			sort_o = object(std::string("namespace eval ") + sort_ns + " { expr { " + *sort + " } }");
			//std::cerr << "sort vars: " << join(get_variables_from_expr(*sort)) << "\n";
		}
		auto sort_nshold = make_namespace_holder(sort_ns);
		
		std::set<pdf_txt_bbox *> not_set;
		if (not_ && *not_) {
			for (auto * p : *not_) {
				not_set.insert(p);
			}
		}

		uint32_t col;
		if (color) {
			col = color_parse(*color);
			//std::cerr << std::hex << "col match " << col << std::dec << "\n";
		}

		bool of_schem_words = false;
		std::set<std::string> exact_match;
		if (of) {
			if (of->as<list<pdf_txt_bbox> >().size()) {
				//std::cerr << "of schem_words " << fff << "\n";
				of_schem_words = true;
			} else {
				of->visit([&](Component * c) {
							  exact_match.insert(c->name);
						  },
						  [&](Pin * p) {
							  exact_match.insert(p->name);
						  },
						  [&](Net * n) {
							  exact_match.insert(n->name);
						  });
			}
		}

		std::vector<pdf_txt_bbox *> ret;

		auto filter_vars1 = get_variables_from_expr(*filter_arg);
		auto * filter_vars = filter_vars1 ? &*filter_vars1 : nullptr;

		auto filter_vars_holder = make_variables_installer(filter_ns, filter_vars ? &*filter_vars : nullptr);
		
		auto try_append = [&](pdf_txt_bbox * p) {
			//std::cerr << "try " << p->word << " " << p << "\n";
			if (matches(match, re, p->word, use_regex)
				&& (!color || p->color == col)
				&& (!exact || *exact == p->word)
				&& (exact_match.empty() || exact_match.find(p->word) != exact_match.end())
				&& filter_match(*tcli, filter_ns, filter_vars, filter_o, p, nullptr, ref ? *ref : nullptr)
				&& not_set.find(p) == not_set.end()
				&& (!radius || ((*ref)->page == p->page && distance((*ref)->box, p->box) < *radius))
				) {
				if (sort) {
					//std::cerr << "push " << p->word << "\n";
					ret.push_back(p);
				} else {
					r.append(makeobj(p));
				}
			}
		};

		if (of_schem_words) {
			of->visit([&](pdf_txt_bbox * p) {
						  //std::cerr << "of " << p->word << " " << p << "\n";
						  try_append(p);
					  });
		} else if (bbox) {
			for (pdf_txt_bbox * p : *bbox) {
				auto it = pdf_words_.find(p->page);
				if (it != pdf_words_.end()) {
					for (auto & w : it->second) {
						if (intersect(p->box, w.box)) {
							try_append(&w);
						}
					}
				}
			}
		} else if (page || ref) {
			auto it = pdf_words_.find(page ? *page : (*ref)->page);
			if (it != pdf_words_.end()) {
				for (auto & w : it->second) {
					try_append(&w);
				}
			}
		} else {
			for (auto & mi : pdf_words_) {
				for (auto & w : mi.second) {
					try_append(&w);
				}
			}
		}
		if (sort) {
			auto vars = get_variables_from_expr(*sort);
			auto vars_holder = make_variables_installer(sort_ns, vars ? &*vars : nullptr);

			std::sort(ret.begin(), ret.end(), [&](pdf_txt_bbox * a, pdf_txt_bbox * b) -> bool {
						  //std::cerr << "less " << a->word << " " << b->word << "\n";
						  return sort_less(*tcli, sort_ns, vars ? &*vars : nullptr, sort_o, a, b, ref ? *ref : nullptr);
					  });
			for (auto * p : ret) {
				//std::cerr << "ret " << p->word << "\n";
				r.append(makeobj(p));
			}
		}
		return r;
	}
#endif // OBV_USE_POPPLER
	
	object TCL::get_nets(getopt<bool> const & use_regex, getopt<bool> const & all, getopt<list<Net> > const & not_, getopt<bool> const & gnd, getopt<any<list<Component>, list<Pin>, list<Net>, list<BRDBoard> > > const & of, getopt<std::string> const & filter_arg, variadic<std::string> const & match) {
		object r;

		object filter_o; int filter = 0; std::string filter_ns;

		if (filter_arg) {
			filter = filter_ns_ix++ % 1000;
			filter_ns = namespace_str(filter);
			//namespace_open(*tcli, filter_ns);
			filter_o = object(std::string("namespace eval ") + filter_ns + " { expr { " + *filter_arg + " } }");
		}
		auto filt_nshold = make_namespace_holder(filter_ns);

		std::regex re;
		if (use_regex && match.size() == 1) re = match[0];

		std::set<Net *> dup;
		std::set<Net *> not_set;

		if (not_ && *not_) {
			for (std::size_t i = 0; i < not_->size(); ++i) {
				if (auto n = not_->at(i)) {
					not_set.insert(n);
				}
			}
		}
		auto filter_vars1 = get_variables_from_expr(*filter_arg);
		auto * filter_vars = filter_vars1 ? &*filter_vars1 : nullptr;

		auto filter_vars_holder = make_variables_installer(filter_ns, filter_vars ? &*filter_vars : nullptr);
		
		auto try_append = [&](Net * n, Component * of = nullptr) {
			if ((gnd || !n->is_ground)
				&& (all || dup.find(n) == dup.end())
				&& not_set.find(n) == not_set.end()
				&& matches(match, re, n->name, use_regex)
				&& filter_match(*tcli, filter_ns, filter_vars, filter_o, n, of)) {
				r.append(makeobj(n));
				if (!all) dup.insert(n);
			}
			return true;
		};

		if (of) {
			of->visit([&](Pin * p) {
						  if (p->net) try_append(p->net);
					  },
					  [&](Component * c) {
						  for (auto && pi : c->pins) {
							  if (pi->net) try_append(pi->net, c);
						  }
					  },
					  [&](BRDBoard * b) {
						  for (auto && ni : b->Nets()) {
							  try_append(ni.get());
						  }
					  },
					  [&](Net * n) {
						  try_append(n);
					  });

		} else {
			bool found = !default_boards_.empty();
			for (std::size_t bi = 0; bi < (default_boards_.empty() ? boards_.size() : default_boards_.size()); ++bi) {
				BRDBoard * b = default_boards_.empty() ? boards_[bi] : default_boards_[bi];
				if (static_cast<Board *>(b) == boardview()->m_board.get())
					found = true;

				for (std::size_t i = 0; i < b->Nets().size(); ++i) {
					Net * n = &(*b->Nets()[i]);
					
					try_append(n);
				}
			}
			if (!found) {
				Board * b = boardview()->m_board.get();
				if (b) {
					for (std::size_t i = 0; i < b->Nets().size(); ++i) {
						Net * n = &(*b->Nets()[i]);
							
						try_append(n);
					}
				}
			}				
		}
		return r;
	}


	object TCL::get_pins(getopt<bool> const & use_regex, getopt<bool> const & all, getopt<bool> const & parallel, getopt<std::string> const & filter_arg, getopt<any<list<Component>, list<Net>, list<Pin>, list<BRDBoard> > > const & of, getopt<bool> const & brief, variadic<std::string> const & match) {
		if (brief) { }

		object r;
		object filter_o;
		int filter = 0;
		std::string filter_ns;

		if (filter_arg) {
			filter = filter_ns_ix++ % 1000;
			filter_ns = namespace_str(filter);
			filter_o = object(std::string("namespace eval ") + filter_ns + " { expr { " + *filter_arg + " } }");
		}

		auto filt_nshold = make_namespace_holder(filter_ns);
		
		std::regex re;
		if (use_regex && match.size() == 1) re = match[0];

		std::set<Pin *> dup;
		bool match_has_slash = true; //std::find(match.begin(), match.end(), '/') != match.end();
		
		auto filter_vars1 = get_variables_from_expr(*filter_arg);
		auto * filter_vars = filter_vars1 ? &*filter_vars1 : nullptr;

		auto filter_vars_holder = make_variables_installer(filter_ns, filter_vars ? &*filter_vars : nullptr);
		
		auto try_append = [&](Pin * c) {
			std::string m(c->name);
			if (match_has_slash) {
				m = c->component->name + "/" + c->name;
			}
			if ((all || dup.find(c) == dup.end())
				&& matches(match ? match : variadic<std::string>{}, re, m, use_regex)
				&& filter_match(*tcli, filter_ns, filter_vars, filter_o, c)) {
				r.append(makeobj(c));
				if (!all) dup.insert(c);
			}
			return true;
		};

		if (of) {
			of->visit([&](Net * n) {
						  for (auto && pi : n->pins) {
							  try_append(pi.get());
						  }
					  },
					  [&](Component * c) {
						  for (auto && pi : c->pins) {
							  try_append(pi.get());
						  }
					  },
					  [&](Pin * p) {
						  try_append(p);
						  if (parallel) {
							  for (auto && pi : p->component->pins) {
								  if (p->net == pi->net) {
									  try_append(pi.get());
								  }
							  }
						  }
					  },
					  [&](BRDBoard * b) {
						  for (auto && pi : b->Pins()) {
							  try_append(&*pi);
						  }

					  });

		} else {
			bool found = ! default_boards_.empty();
			for (std::size_t bi = 0; bi < (default_boards_.empty() ? boards_.size() : default_boards_.size()); ++bi) {
				BRDBoard * b = default_boards_.empty() ? boards_[bi] : default_boards_[bi];
				if (static_cast<Board *>(b) == boardview()->m_board.get()) found = true;

				for (std::size_t i = 0; i < b->Pins().size(); ++i) {
					try_append(&(*b->Pins()[i]));
				}
			}
			if (!found) {
				Board * b = boardview()->m_board.get();
				if (b) {
					for (std::size_t i = 0; i < b->Pins().size(); ++i) {
						try_append(&(*b->Pins()[i]));
					}
				}					
			}
		}
		return r;
	}

	object TCL::get_cells(getopt<std::string> const & filter_arg, getopt<std::string> const & sort, getopt<bool> const & use_regex, getopt<any<list<Component>, list<Net>, list<Pin>, list<pdf_txt_bbox> > > const & of, getopt<bool> const & all,
						  getopt<bool> const & top,
						  getopt<bool> const & bottom,
						  getopt<list<Component> > const & not_, getopt<Component *> const & ref, variadic<std::string> const & match) {
		object r;
		int filter = 0; std::string filter_ns; object filter_o;
		int sort_ix; std::string sort_ns; object sort_o;
		std::regex re;

		if (use_regex && match) re = match[0];

		if (filter_arg) {
			filter = filter_ns_ix++ % 1000;
			filter_ns = namespace_str(filter);
			//namespace_open(*tcli, filter_ns);
			filter_o = object(std::string("namespace eval ") + namespace_str(filter) + " { expr { " + *filter_arg + " } }");
		}
		auto filt_nshold = make_namespace_holder(filter_ns);

		if (sort) {
			sort_ix = sort_ns_ix++ % 1000;
			sort_ns = sort_namespace_str(sort_ix);
			//namespace_open(*tcli, sort_ns);
			sort_o = object(std::string("namespace eval ") + sort_ns + " { expr { " + *sort + " } }");
		}
		auto sort_nshold = make_namespace_holder(sort_ns);

		std::set<Component *> dup;
		std::set<Component *> not_set;

		if (not_ && *not_) {
			for (Component * c : *not_) {
				not_set.insert(c);
			}
		}

		std::vector<Component *> ret;

		auto filter_vars1 = get_variables_from_expr(*filter_arg);
		auto * filter_vars = filter_vars1 ? &*filter_vars1 : nullptr;

		auto filter_vars_holder = make_variables_installer(filter_ns, filter_vars ? &*filter_vars : nullptr);
		
		auto try_append = [&](Component * c) {
			if (!is_dummy(c)
				&& ((! top && ! bottom) || (top && c->board_side == kBoardSideTop) || (bottom && c->board_side == kBoardSideBottom))
				&& (all || dup.find(c) == dup.end())
				&& not_set.find(c) == not_set.end()
				&& matches(match, re, c->name, use_regex)
				&& filter_match(*tcli, filter_ns, filter_vars, filter_o, c)) {

				if (sort) {
					ret.push_back(c);
				} else {
					r.append(makeobj(c));
				}
				if (!all) dup.insert(c);
			}
			return true;
		};

		if (of) {
			of->visit([&](Component * c) { try_append(c); },
					  [&](Net * n) {
						  for (auto && pi : n->pins) {
							  try_append(pi->component.get());
						  }
					  },
					  [&](Pin * p) { try_append(p->component.get()); },
					  [&](BRDBoard * b) {
						  for (auto && c : b->Components()) {
							  try_append(&*c);
						  }
					  },
					  [&](pdf_txt_bbox * p) {
						  for (auto && c : boardview()->m_board->Components()) {
							  if (c->name == p->word) {
								  try_append(&*c);
							  }
						  }
					  });
		} else {
			bool found = !default_boards_.empty();
			for (std::size_t bi = 0; bi < (default_boards_.empty() ? boards_.size() : default_boards_.size()); ++bi) {
				BRDBoard * b = default_boards_.empty() ? boards_[bi] : default_boards_[bi];
				if (static_cast<Board *>(b) == boardview()->m_board.get()) found = true;
				for (std::size_t i = 0; i < b->Components().size(); ++i) {
					try_append(&(*b->Components()[i]));
				}
			}
			if (!found) {
				Board * b = boardview()->m_board.get();
				if (b) {
					for (std::size_t i = 0; i < b->Components().size(); ++i) {
						try_append(&(*b->Components()[i]));
					}
				}
			}				
		}

		if (sort) {

			auto vars = get_variables_from_expr(*sort);
			auto vars_holder = make_variables_installer(sort_ns, vars ? &*vars : nullptr);
			std::sort(ret.begin(), ret.end(), [&](Component * a, Component * b) -> bool {
						  return sort_less(*tcli, sort_ns, vars ? &*vars : nullptr, sort_o, a, b, ref ? *ref : nullptr);
					  });
			for (auto * p : ret) {
				r.append(makeobj(p));
			}
		}

		return r;
	}

	object TCL::selection() {
		object r;

		for (auto && pi : boardview()->m_pinHighlighted) {
			r.append(makeobj(pi.get()));
		}
		for (auto && ci : boardview()->m_partHighlighted) {
			r.append(makeobj(ci.get()));
		}
		return r;
	}

	void TCL::highlight(getopt<bool> const & save, getopt<bool> const & un, getopt<bool> const & all, getopt<std::string> const & color, getopt<int> const & colorindex, getopt<float> const & inten, opt<list<any<Component, Pin, pdf_txt_bbox> > > const & obj) {
		uint32_t color_v = 0xffaaaaaa;

		const uint32_t distinct[] = {
			//0x2f4f4f,
			0xf0e68c,
			0xa0522d,
			//0x006400,
			0x00008b,
			0x48d1cc,
			0xff0000,
			0xffa500,
			0xffff00,
			0x00ff00,
			0x00fa9a,
			0x0000ff,
			0xda70d6,
			0xd8bfd8,
			0xff00ff,
			0x1e90ff
		};
		const int ndistinct = sizeof(distinct) / sizeof(distinct[0]);

		if (save) {
			this->eval("prop_begin -highlight");
		}
		
		if (colorindex) {
			color_v = uint32_t(0xff000000) | distinct[*colorindex % ndistinct];
		} else if (color) {
			const std::string & col = *color;
			if (col[0] == '#') {
				std::istringstream iss(col);
				char dummy;
						
				iss >> dummy >> std::hex >> color_v;
				color_v |= 0xff000000;
			} else {
				color_v = 0xffaaaaaa;
			}
			color_v &= 0x00ffffff;
			color_v |= 0x99000000;
		}
		if (un && all) {
			for (auto && ci : brd()->Components()) {
				ci->shade_color_ = 0;
				ci->intensity_delta_ = 1;
			}
		} else if (obj) {
			for (auto le : *obj) {
				le.visit([&](Component * c) {
							 if (un) {
								 c->shade_color_ = 0;
							 } else {
								 if (highlight_delay_enable_) {
									 highlight_delay_.push_back(highlight_delay{c, color});
								 } else {
									 if (color || !inten) {
										 c->shade_color_ = color_v; //0xffff0000;
									 }
									 if (inten) {
										 c->intensity_delta_ = *inten;
									 }
								 }
							 }
						 },
						 [&](Pin * p) {
							 for (auto && pi : brd()->Pins()) {
								 if (pi.get() == p) {
									 boardview()->m_pinHighlighted.push_back(pi);
									 break;
								 }
							 }
						 },
						 [&](pdf_txt_bbox * p) {
							 prop_stack_try_save_highlight(p);
							 if (un) p->color = 0;
							 else p->color = color_v;
						 });
			}
		}
	}

	void TCL::set_prop(std::string const & prop, std::string const & value, list<any<Component, Net, Pin, pdf_txt_bbox> > const & obj) {
		for (auto a : obj) {
			be_priv * pr = nullptr;
			a.visit([&](Component * c) {
						if (! c->tcl_priv) { c->tcl_priv = (void *) new cell_priv(); }
						pr = (be_priv *) c->tcl_priv;
					},
					[&](Net * n) {
						if (! n->tcl_priv) { n->tcl_priv = (void *) new net_priv(); }
						pr = (be_priv *) n->tcl_priv;
					},
					[&](Pin * p) {
						if (! p->tcl_priv) { p->tcl_priv = (void *) new pin_priv(); }
						pr = (be_priv *) p->tcl_priv;
					},
					[&](pdf_txt_bbox * p) {
						if (prop == "mark") {
							prop_stack_try_save_mark(p);
							p->mark = stoi(value);
						} else if (prop == "genmark") {
							p->mark = stoi(value);
							p->mark |= 1ULL << sizeof(p->mark) * 8 - 1;
						} else {
							throw tcl_error(std::string("illegal property ") + prop + " for type schem_word");
						}
					});
			if (pr) {
				pr->properties[prop] = value;
			}
		}
	}

	object TCL::get_prop(std::string const & prop, list<any_obv_type> const & obj) {
		object r;
		std::size_t sz = obj.size();
		for (std::size_t i = 0; i < sz; ++i) {
			auto le = obj.at(i);

			le.visit([&] (Component    * c) { r.append(cell_get_prop      (prop.c_str(), c)); },
					 [&] (Net          * n) { r.append(net_get_prop       (prop.c_str(), n)); },
					 [&] (Pin          * p) { r.append(pin_get_prop       (prop.c_str(), p)); },
					 [&] (pdf_txt_bbox * p) { r.append(schem_word_get_prop(prop.c_str(), p)); });

		}
		return r;
	}

	void TCL::select(list<any_obv_type> obj) {
		boardview()->m_partHighlighted.clear();

		for (std::size_t ai = 0; ai < obj.size(); ++ai) {
			auto le = obj.at(ai);

			Component * c; Pin * p; Net * n;
			if (p && n) { }
			if ((c = le.as<Component>())) {
				for (auto && ci : brd()->Components()) {
					if (ci.get() == c) {
						boardview()->m_partHighlighted.push_back(ci);
						//found = true;
						break;
					}
				}
			}
		}
	}

	void TCL::report_prop(getopt<bool> const & verbose, list<any_obv_type> const & obj) {
		std::ostringstream oss;

		if (verbose) {
			oss << obj.size() << " objects\n";
		}
		
		for (std::size_t ai = 0; ai < obj.size(); ++ai) {
			if (verbose && ai) {
				oss << "-----------\n";
			}

			auto le = obj.at(ai);

			be_priv * pr = nullptr;
			//oss << "object=" << to << " type=" << (to->typePtr && to->typePtr->name ? to->typePtr->name : "(none)") << "[" << to->typePtr << "]\n";
			if (Net * n = le.as<Net>()) {
				for (int i = 0; net_props_[i]; ++i) {
					oss << std::left << std::setw(20) << net_props_[i] << net_get_prop(net_props_[i], n).get<std::string>() << "\n";
				}
				pr = (be_priv *) n->tcl_priv;
			} else if (Component * c = le.as<Component>()) {
				for (int i = 0; cell_props_[i]; ++i) {
					oss << std::left << std::setw(20) << cell_props_[i] << cell_get_prop(cell_props_[i], c) << "\n";
				}
				pr = (be_priv *) c->tcl_priv;
			} else if (Pin * p = le.as<Pin>()) {
				for (int i = 0; pin_props_[i]; ++i) {
					oss << std::left << std::setw(20) << pin_props_[i] << pin_get_prop(pin_props_[i], p) << "\n";
				}
				pr = (be_priv *) p->tcl_priv;
			} else if (auto * p = le.as<pdf_txt_bbox>()) {
				if (verbose) {
					oss << "objtype: schem_word\n";
				}
				for (int i = 0; pdf_word_props_[i]; ++i) {
					oss << std::left << std::setw(20) << pdf_word_props_[i] << schem_word_get_prop(pdf_word_props_[i], p) << "\n";
				}
			}
			if (pr) {
				for (auto it = pr->properties.begin(); it != pr->properties.end(); ++it) {
					oss << std::left << std::setw(20) << it->first << it->second << "\n";
				}
			}
		}
		std::cerr << oss.str();
	}

#ifdef HAVE_READLINE
	static void	int_handler(int status) {
		if (status) { }
		printf("\n"); // Move to a new line
		rl_on_new_line(); // Regenerate the prompt on a newline
		rl_replace_line("", 0); // Clear the previous text
		rl_redisplay();
	}
#endif
	
	struct TClass {
		int var_;
		TClass(int v) : var_(v) { }
		int get() {
			return var_;
		}
		void set(int v) {
			var_ = v;
		}
		int get2() {
			return var_;
		}
		~TClass() {
			std::cerr << "delete TClass " << this << "\n";
		}
	};

	TCL::TCL(BoardView * bvv, bool * d, std::string const & script) : b(bvv), done_(d) {
		tcl = Tcl_CreateInterp();
		
		tcli = new interpreter(tcl, true);

		this_s_ = this;

		static_initialize();
		
		typedef TCL this_t;

		struct cell_ops : public interpreter::type_ops<Component> {
			static void str(Tcl_Obj * o) {
				Component * c = (Component *) o->internalRep.otherValuePtr;
				//std::cerr << "str_cell ";
				//std::cerr << c << " ";
				//std::cerr << "str_cell " << c << " " << c->name << " " << c->name.size() << " ";
				str_impl(o, c->name);
			}
		};
		struct net_ops : public interpreter::type_ops<Net> {
			static void str(Tcl_Obj * o) {
				Net * n = (Net *) o->internalRep.otherValuePtr;
				str_impl(o, n->name);
			}
		};
		struct pin_ops : public interpreter::type_ops<Pin> {
			static void str(Tcl_Obj * o) {
				Pin * p = (Pin *) o->internalRep.otherValuePtr;
				str_impl(o, p->component->name + "/" + p->name);
			}
		};
		struct board_ops : public interpreter::type_ops<BRDBoard> {
			static void str(Tcl_Obj * o) {
				BRDBoard * b = (BRDBoard *) o->internalRep.otherValuePtr;
				std::ostringstream oss;
				oss << "brd" << (void *) b;
				str_impl(o, oss.str());
			}
		};
		struct schem_word_ops : public interpreter::type_ops<pdf_txt_bbox> {
			static void str(Tcl_Obj * o) {
				pdf_txt_bbox * p = (pdf_txt_bbox *) o->internalRep.otherValuePtr;
				if (p->word.empty()) {
					std::ostringstream oss;
					oss << "pdf_txt_bbox@" << p;
					str_impl(o, oss.str());
				} else {
					str_impl(o, p->word);
				}
					
			}
		};
		tcli->type<cell_ops> ("cell",  (Component *) nullptr);
		tcli->type<net_ops>  ("net",   (Net       *) nullptr);
		tcli->type<pin_ops>  ("pin",   (Pin       *) nullptr);
		tcli->type<board_ops>("board", (BRDBoard  *) nullptr);
		tcli->type<schem_word_ops>("schem_word", (pdf_txt_bbox *) nullptr);

#if 0
		tcli->def(this)
			.def("distance", &this_t::distance_fun, options(distance_opt));

		std::cerr << "get_cells\n";
		tcli->def(this)
			.def("angle",             &this_t::angle,             options(angle_opt))
			.def("get_schem_words",   &this_t::get_schem_words,   options(get_schem_words_opt))
			.def("schem_open",        &this_t::schematic_open,    options(schematic_open_opt))
			.def("get_cells",         &this_t::get_cells,         options(get_cells_opt));
#endif
#if 1
		
		tcli->def(this)
			.def("get_nets",          &this_t::get_nets,          options(get_nets_opt))
			.def("get_cells",         &this_t::get_cells,         options(get_cells_opt))
			.def("get_pins",          &this_t::get_pins,          options(get_pins_opt))
			.def("get_boards",        &this_t::get_boards)
			.def("default_boards",    &this_t::default_boards)
			.def("draw_board",        &this_t::draw_board)
			.def("report_prop",       &this_t::report_prop,       options(report_prop_opt))
			.def("highlight",         &this_t::highlight,         options(highlight_opt))
			.def("selection",         &this_t::selection)
			.def("same_object",       &this_t::same_object)
#ifdef OBV_USE_POPPLER
			.def("get_schem_words",   &this_t::get_schem_words,   options(get_schem_words_opt))
			.def("schem_open",        &this_t::schematic_open,    options(schematic_open_opt))
			.def("schem_sticky",      &this_t::pdf_sticky,        options("toggle"))
			.def("schem_invert",      &this_t::pdf_invert)
			.def("draw_page",         &this_t::draw_page)
			.def("undraw_page",       &this_t::undraw_page)
			.def("schem_highlight",   &this_t::schem_highlight)
			.def("distance",          &this_t::distance_fun,      options(distance_opt))
			.def("angle",             &this_t::angle,             options(angle_opt))
			.def("enclosed_box",      &this_t::enclosed_box)
			.def("prop_begin",        &this_t::prop_begin,        options(prop_begin_opt))
			.def("prop_abort",        &this_t::prop_abort)
			.def("prop_commit",       &this_t::prop_commit)
			.def("prop_stack_enable", &this_t::prop_stack_enable)
			.def("prop_stack_info",   &this_t::prop_stack_info)
#endif
			.def("generate_mark",     &this_t::generate_mark,     options(generate_mark_opt))
			.def("get_prop",          &this_t::get_prop)
			.def("set_prop",          &this_t::set_prop)
			.def("get",               &this_t::get_prop)
			.def("trace",             &this_t::trace,             options(trace_opt))
			.def("select",            &this_t::select)
			.def("file_history",      &this_t::file_history)
			.def("obv_open",          &this_t::obv_open)
			.def("exit",              &this_t::quit)
			.def("objinfo",           &this_t::objinfo)
			.def("last_result",       &this_t::last_result);

		tcli->defvar("default_intensity", boardview()->m_default_intensity);
		tcli->defvar("dual_draw", boardview()->m_draw_both_sides);
#endif

		tcli->managed_class_<TClass>("tclass", init<int>())
			.def("set", &TClass::set)
			.def("get", &TClass::get);
#if 0

		tcli->class_<ImVec2>("ImVec2", init<float, float>())
			.defvar("x", &ImVec2::x)
			.defvar("y", &ImVec2::y);
#endif

		Tcl_Init(tcl);

#ifdef HAVE_READLINE
		rl_prep_terminal(0);
		rl_attempted_completion_function = rl_complete;
		rl_callback_handler_install("OBV> ", rl_cb);
		read_history((get_user_dir(UserDir::Data) + "/tcl_history").c_str());
		signal(SIGINT, int_handler);
#endif
		Tcl_SetExitProc(exit_proc);
		tcli->eval("source library.tcl");
		startup_script = script;
	}
	
	template <typename CMP>
	bool TCL::schem_word_get_prop_impl(const char * prop, pdf_txt_bbox * p, object & ret, CMP icmp) {
		if (icmp(prop, "xmin")) {
			ret = p->box.min.x;
		} else if (icmp(prop, "ymin")) {
			ret = p->box.min.y;
		} else if (icmp(prop, "xmax")) {
			ret = p->box.max.x;
		} else if (icmp(prop, "ymax")) {
			ret = p->box.max.y;
		} else if (icmp(prop, "width")) {
			ret = p->box.max.x - p->box.min.x;
		} else if (icmp(prop, "height")) {
			ret = p->box.max.y - p->box.min.y;
		} else if (icmp(prop, "clickable")) {
			ret = p->clickable;
		} else if (icmp(prop, "hoverable")) {
			ret = p->hoverable;
		} else if (icmp(prop, "page")) {
			ret = p->page;
		} else if (icmp(prop, "color")) {
			std::ostringstream oss;
			oss << '#' << std::hex << p->color;
			ret = oss.str();
		} else if (icmp(prop, "mark")) {
			ret = (p->mark & 1 << sizeof(p->mark) * 8 - 1 ? 0 : p->mark);
		} else if (icmp(prop, "genmark")) {
			ret = (p->mark & ~(1 << sizeof(p->mark) * 8 - 1));
		} else if (icmp(prop, "is_cell")) {
			ret = bool(p->cell);
		} else if (icmp(prop, "is_net")) {
			ret = bool(p->net);
		} else if (icmp(prop, "highlighted")) {
			ret = bool(p->color);
		} else if (icmp(prop, "marked")) {
			ret = bool(p->mark) && !bool(p->mark & 1ULL << sizeof(p->mark) * 8 - 1);
		} else if (icmp(prop, "genmarked")) {
			ret = bool(p->mark & 1ULL << sizeof(p->mark) * 8 - 1) && bool(p->mark & ~(1ULL << sizeof(p->mark) * 8 - 1));
		} else {
			//ret = object();
			return false;
		}
		return true;
	}

	template <typename CMP>
	bool TCL::cell_get_prop_impl(const char * prop, Component * c, object & ret, CMP icmp) {
		if (icmp(prop, "type")) {
			std::ostringstream oss;
			oss << c->component_type;
			ret = oss.str();
		} else if (icmp(prop, "pins")) {
			//std::cerr << "P" << ret.get_object()->refCount << "\n";
			ret = (int) c->pins.size();
			//std::cerr << "PP" << ret.get_object()->refCount << "\n";
		} else if (icmp(prop, "has_gnd")) {
			for (auto && pi : c->pins) {
				Net * n = pi->net;
				if (n && n->is_ground) {
					ret = true;
					return true;
				}
			}
			ret = false;
		} else {
			if constexpr (std::is_same<CMP, cmpeq_t>::value) {
				return extra_properties_get_impl(prop, ret, (be_priv *) c->tcl_priv);
			} else {
				//ret = object();
				return false;
			}
		}
		return true;
	}
	template <typename CMP>
	bool TCL::pin_get_prop_impl(const char * prop, Pin * p, object & ret, CMP icmp) {
		if (icmp(prop, "type")) {
			ret = "the_type";
		} else if (icmp(prop, "isgnd")) {
			ret = bool(p->net && p->net->is_ground);
		} else if (icmp(prop, "ispad")) {
			Component * c = p->component.get();
			if (c) {
				bool top = false, bottom = false, left = false, right = false;
				for (auto && pi : c->pins) {
					if (pi->position.x < p->position.x - p->diameter / 2) left = true;
					if (pi->position.x > p->position.x + p->diameter / 2) right = true;
					if (pi->position.y < p->position.y - p->diameter / 2) bottom = true;
					if (pi->position.y > p->position.y + p->diameter / 2) top = true;
					if (top && bottom && left && right) {
						ret = true;
						return true;
					}
				}
			}
			ret = false;
		} else if (icmp(prop, "name")) {
			ret = p->name;
		} else {
			if constexpr (std::is_same<CMP, cmpeq_t>::value) {
				return extra_properties_get_impl(prop, ret, (be_priv *) p->tcl_priv);
			} else {
				//ret = object();
				return false;
			}
		}
		return true;
	}
	template <typename CMP>
	bool TCL::net_get_prop_impl(const char * prop, Net * n, object & ret, CMP icmp) {
		if (icmp(prop, "isgnd")) {
			ret = n->is_ground;
		} else {
			if constexpr (std::is_same<CMP, cmpeq_t>::value) {
				return extra_properties_get_impl(prop, ret, (be_priv *) n->tcl_priv);
			} else {
				return false;
			}
		}
		return true;
	}

	void TCL::static_initialize() {
		if (! static_initialized_) {
			auto inst = [](auto & p, bool doit) {
				//using type = typename std::remove_pointer<decltype(p)>::type;
				using type1 = decltype(p);
				using type = typename std::conditional<std::is_same<type1, cmpeq_t &>::value, cmpeq_t, type1>::type;
				if (doit) {
					pdf_word_props_ = enumerate_props_impl(schem_word_get_prop<type>, p);
					cell_props_     = enumerate_props_impl(cell_get_prop<type>, p);
					net_props_      = enumerate_props_impl(net_get_prop<type>, p);
					pin_props_      = enumerate_props_impl(pin_get_prop<type>, p);
				}
			};
			record_strings_t rec;
			inst(rec, true);
			cmpeq_t c;
			inst(c, false); // instantiate methods only
		}
		static_initialized_ = true;
	}

	bool TCL::matches(const variadic<std::string> & glob, const std::regex & re, const std::string & s, bool use_re) {
		if (glob.size()) {
			if (use_re) {
				return std::regex_search(s, re);
			} else {
				for (auto const & ss : glob) {
					if (ss[0] == '!') {
						if (fnmatch(ss.c_str() + 1, s.c_str(), FNM_CASEFOLD) != 0) return true;
					} else {
						if (fnmatch(ss.c_str(), s.c_str(), FNM_CASEFOLD) == 0) return true;
					}
				}
				return false;
			}
		}
		return true;
	}

#ifdef OBV_USE_POPPLER
	float TCL::distance_fun(getopt<bool> const & norm, getopt<bool> border, any<Component, Pin, pdf_txt_bbox> const & a, any<Component, Pin, pdf_txt_bbox> const & b) {
		if (norm) { }

		if (a.as<pdf_txt_bbox>() && b.as<pdf_txt_bbox>()) {
			if (border) {
				
			} else { // center
				ImVec2 ca = center(a.as<pdf_txt_bbox>()->box), cb = center(b.as<pdf_txt_bbox>()->box);
				ImVec2 d = ca - cb;
				return sqrtf(d.x * d.x + d.y * d.y);
			}
		} else if (a.as<Component>()) {
			//std::cerr << "is component\n";
		} else if (a.as<Pin>()) {
			//std::cerr << "is pin\n";
		} else {
			//std::cerr << "cannot any cast\n";
		}

		return 0;
	}

	float TCL::angle(getopt<bool> const & ortho, getopt<float> const & decay, pdf_txt_bbox * a, pdf_txt_bbox * b) {
		if (decay) {
			throw tcl_error("angle -decay not implemented");
		}

		ImVec2 v = center(a->box) - center(b->box);
		if (v.y == 0) {
			return 0.0f;
		} else {
			float deg = atanf(v.x / v.y) * 180 / M_PI;
			if (ortho) {
				int m1 = int(abs(deg)) % 90;
				int m2 = 90 - int(abs(deg)) % 90;
				return float(m1 < m2 ? m1 : m2);
			} else {
				return deg;
			}
		}
	}
	object TCL::enclosed_box(list<pdf_txt_bbox> const & words) {
		pdf_txt_bbox ret_o;
		pdf_txt_bbox * ret = &ret_o; //new pdf_txt_bbox;
		typedef std::numeric_limits<float> lim;
		ret->box.min = { lim::max(), lim::max() };
		ret->box.max = { lim::min(), lim::min() };
		for (pdf_txt_bbox * w : words) {
			if (w->box.min.x < ret->box.min.x) {
				ret->box.min.x = w->box.min.x;
			}
			if (w->box.min.y < ret->box.min.y) {
				ret->box.min.y = w->box.min.y;
			}
			if (w->box.max.x > ret->box.max.x) {
				ret->box.max.x = w->box.max.x;
			}
			if (w->box.max.y > ret->box.max.y) {
				ret->box.max.y = w->box.max.y;
			}
			ret->page = w->page;
		}
		pdf_words_[ret->page].push_back(ret_o);
		
		return makeobj(& (pdf_words_[ret->page].back()), true, [this](pdf_txt_bbox * p) {
						   auto it = pdf_words_.find(p->page);
						   if (it != pdf_words_.end()) {
							   for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
								   if (& (*it2) == p) {
									   it->second.erase(it2);
									   break;
								   }
							   }
						   }
					   });
	}

	void TCL::imgui_draw_in(ImDrawList * draw, ImVec2 min, ImVec2 max) {
		currently_hovered_part_ = nullptr;
		if (pdf_img_) {
			{
				if (true) {
					pdf_img_->render(*draw, pdf_win_.sticky ? yflip(min) : boardview()->CoordToScreen(yflip(min)), pdf_win_.sticky ? yflip(max) : boardview()->CoordToScreen(yflip(max)), 0);
					ImVec2 spos = ImGui::GetMousePos();
					ImVec2 pos  = pdf_win_.sticky ? spos : boardview()->ScreenToCoord(spos);

					bool mouse_release = in(pdf_win_.box, pos) && pdf_win_.mouse_released; //ImGui::IsMouseReleased(0) && ! boardview()->m_draggingLastFrame;
					bool ctrl = ImGui::GetIO().KeyCtrl;
					bool mouse_click_handled = false;

					if (mouse_release) {
#if 0
						std::cerr << "pdf mouse release";
						if (pdf_win_.ignore_one_mouse_release) {
							std::cerr << " ignored";
							mouse_release = false;
						}
						std::cerr << "\n";
						pdf_win_.ignore_one_mouse_release = false;
#endif
						pdf_win_.mouse_released = false;
					}
					if (true) {
						auto & phi   = pdf_highlight_;
						//ImVec2 ppos  = { ppos1.x / psz1.x * phi.pdim.x, phi.pdim.y - ppos1.y / psz1.y * phi.pdim.y };
						ImVec2 ppos = board2pdf(pdf_win_, pdf_img_, pos, boardview()->m_current_side || pdf_win_.sticky);

						if (false && (boardview()->m_current_side || pdf_win_.sticky)) {
							ppos.y = phi.pdim.y - ppos.y;
						}
						//std::cerr << ppos << "\n"; //.x << " " << ppos.y << "\n";
							
						auto it = pdf_words_.find(phi.page);
						if (it != pdf_words_.end()) {
							bool seen_hovered = false;

							for (auto & w : it->second) {
								if (! w.hoverable && ! w.color) { continue; }

								uint32_t col = 0xff8080ff;
								auto c_shptr = lookup(w.cell);
								bool is_selected = false;
								bool is_hovered = !seen_hovered && in(pdf_win_.box, pos) && in(w.box, ppos);// > w.box.min && ppos < w.box.max;
								seen_hovered = is_hovered;
								bool is_visible = false;
								bool board_is_hovered = false;
								if (c_shptr) {
									if (!boardview()->BoardElementIsVisible(*c_shptr)) {
										col = 0xffff8080;
									} else {
										is_visible = true;
									}

									if ((is_selected = boardview()->PartIsHighlighted(*c_shptr))) {
										col = 0xff0000ff;
									}
									board_is_hovered = boardview()->currentlyHoveredPart.get() == w.cell;
								}
								if (w.color) {
									col = w.color;
								}
									
								//std::cerr << ppos.x << " " << ppos.y << " word: " << w.word << "\n";
								if (is_selected || board_is_hovered || is_hovered || w.color) {
									ImVec2 xpa, xpb{ w.box.min };
										
									for (int i = 0; i < 4; ++i) {
										xpa = xpb;
										if (i == 0)      { xpb = { w.box.max.x, w.box.min.y }; }
										else if (i == 1) { xpb = { w.box.max.x, w.box.max.y }; }
										else if (i == 2) { xpb = { w.box.min.x, w.box.max.y }; }
										else if (i == 3) { xpb = { w.box.min.x, w.box.min.y }; }
											
										ImVec2 bpa = pdf2board(pdf_win_, pdf_img_, xpa, boardview()->m_current_side || pdf_win_.sticky);
										ImVec2 bpb = pdf2board(pdf_win_, pdf_img_, xpb, boardview()->m_current_side || pdf_win_.sticky);

										if (in(pdf_win_.box, bpa) && in(pdf_win_.box, bpb)) {
											if (pdf_win_.sticky) {
												draw->AddLine(bpa, bpb, col, 5);
											} else {
													
												ImVec2 spa = boardview()->CoordToScreen(bpa);// + boardview()->m_boardHeight);
													
												ImVec2 spb = boardview()->CoordToScreen(bpb);// + boardview()->m_boardHeight);

												draw->AddLine(spa, spb, col, 5);
											}
										}
									}
									if (is_hovered && is_visible) currently_hovered_part_ = w.cell;
									//break;
								}
								if (is_hovered && mouse_release && c_shptr) {
									mouse_click_handled = true;
									if (! ctrl) {
										boardview()->m_partHighlighted.clear();
									}
									boardview()->m_partHighlighted.push_back(*c_shptr);
								}
							}
						}
					}
					if (mouse_release && ! mouse_click_handled) {
						if (! ctrl) {
							boardview()->m_partHighlighted.clear();
						}
					}
				}
			}
		}
	}

	int TCL::handle_mouse_drag(ImVec2 const & ppos, ImVec2 const & drag, int token) {
		ImGuiIO &io = ImGui::GetIO();
		ImVec2 pos  = pdf_win_.sticky ? ppos : boardview()->ScreenToCoord(ppos);

		static const int token_x_top = 8;
		static const int token_y_top = 16;
		static const int token_drag  = 128;
			
		if (token || in(pdf_win_.box, pos)) {
			pdf_win_.dragging = true;
			if (! token) token = 1;
			if (io.KeyShift) {
				ImVec2 bdrag = pdf_win_.sticky ? drag : boardview()->ScreenToCoord(drag, 0);
				//std::cerr << (yflip(pos) - pdf_win_.box.min) / pdf_win_.bdim << "\t" << pdf_win_.bdim << "\t" << pdf_win_.box.min << "\t" << pos << "\t" << yflip(pos) << "\n";

				bool at_corner = false;
				int xi = 0, yi = 0; // 0 = bottom, 1 = top
					
				if (!(token & token_drag)) {
					for (xi = 0; xi < 2; ++xi) {
						for (yi = 0; yi < 2; ++yi) {
							// 10% sized corners enable resize
							bbox box( { { 0.9f * xi, 0.9f * yi }, { 0.1f + 0.9f * xi, 0.1f + 0.9f * yi } } );
							if (in(box, (yflip(pos) - pdf_win_.box.min) / pdf_win_.bdim)) {
								at_corner = true;
								break;
							}
						}
						if (at_corner) break;
					}
				}
					
				if ((token & token_drag) || at_corner) {
					//std::cerr << token << " " << at_corner << " " << xi << " " << yi << " " << bdrag << " " << (yflip(pos) - pdf_win_.box.min) / pdf_win_.bdim << "\n";

					if (xi || (token & token_x_top)) {
						pdf_win_.box.max.x += bdrag.x;
					} else {
						pdf_win_.box.min.x += bdrag.x;
					}

					if (pdf_win_.sticky || boardview()->m_current_side) {
						if (yi || (token & token_y_top)) {
							pdf_win_.box.min.y += bdrag.y;
						} else {
							pdf_win_.box.max.y += bdrag.y;
						}
					} else {
						if (yi || (token & token_y_top)) {
							pdf_win_.box.max.y += bdrag.y;
						} else {
							pdf_win_.box.min.y += bdrag.y;
						}
					}
					//std::cerr << "resize\n";
					pdf_win_.update();
					if (token < 2) token = 128 + token_x_top * xi + token_y_top * yi;
				} else {
					pdf_win_.box.min += bdrag;
					pdf_win_.box.max += bdrag;
				}
				pdf_win_.reuse = true;
				return token;
			} else if (io.KeyCtrl || pdf_win_.sticky) {
				pdf_win_.dragging = true;
				ImVec2 bdrag = pdf_win_.sticky ? drag : ImVec2(1.0f, -1.0f) * boardview()->ScreenToCoord(drag, 0);
				pdf_img_->scroll(bdrag / pdf_win_.bdim);
				return token;
			}
		}
		return 0;
	}

	bool TCL::handle_mouse_wheel(float x, float y, float wh) {
		ImVec2 spos = { x, y };
		ImVec2 mpos = pdf_win_.sticky ? spos : boardview()->ScreenToCoord(spos);
		ImGuiIO &io = ImGui::GetIO();

		//std::cerr << mpos << " " << pdf_win_.box.min << " " << pdf_win_.box.max << "\n";
			
		if (mpos > pdf_win_.box.min && mpos < pdf_win_.box.max) {
			if (io.KeyShift) {
				ImVec2 pdfsz = pdf_win_.bdim;
				if (wh > 0) {
					pdf_win_.box.max += pdfsz / 10;
					pdf_win_.box.min -= pdfsz / 10;
				} else if (wh < 0) {
					pdf_win_.box.max -= pdfsz / 10;
					pdf_win_.box.min += pdfsz / 10;
				}
			} else if (io.KeyCtrl || pdf_win_.sticky) {
				ImVec2 t = (mpos - pdf_win_.box.min) / pdf_win_.bdim;
				if (! pdf_win_.sticky) {
					t.y = 1.0f - t.y;
				}
				pdf_img_->zoom(t, wh);
			} else {
				return false;
			}
			pdf_win_.update();
			return true;
		} else {
		}
		return false;
	}

	bool TCL::pdf_sticky(getopt<bool> const & toggle, opt<bool> const & v) {
		bool newv = toggle ? !pdf_win_.sticky : false;
		newv = v ? *v : newv;
		if (toggle || v) {
			if (! pdf_win_.sticky && newv) {
				pdf_win_.box.min = boardview()->CoordToScreen(pdf_win_.box.min);
				pdf_win_.box.max = boardview()->CoordToScreen(pdf_win_.box.max);
				std::swap(pdf_win_.box.min.y, pdf_win_.box.max.y);
			} else if (pdf_win_.sticky && !newv) {
				pdf_win_.box.min = boardview()->ScreenToCoord(pdf_win_.box.min);
				pdf_win_.box.max = boardview()->ScreenToCoord(pdf_win_.box.max);
				std::swap(pdf_win_.box.min.y, pdf_win_.box.max.y);
			}

			pdf_win_.sticky = newv;
			pdf_win_.update();
		}
		return pdf_win_.sticky;
	}

	void TCL::schem_highlight(list<float> x, opt<float> y, opt<float> w, opt<float> h) {	
		pdf_bbox & ref = pdf_highlight_;

		if (x.size() == 1 && y && w && h) {
			ref.box = { { x[0], *y }, { *w, *h } };
			ref.valid = true;
		} else if (x.size() == 4) {
			ref.box = { { x[0], x[1] }, { x[2], x[3] } };
			ref.valid = true;
		} else {
			ref.valid = false;
		}
		if (ref.valid) {
			ref.box.min -= ref.box.max / 2.0f;
			ref.box.max = ref.box.max * 2 + ref.box.min;
		}
	}

	void TCL::draw_page(int page, opt<Component *> const & inside) {
		for (auto & i : page_cache_) {
			if (i.first != page) {
				i.second.pdf_img->unload();
			}
		}

		page_cache * p = render_page(page);

		pdf_img_ = p->pdf_img;
		pdf_highlight_.valid  = false;
		pdf_highlight_.pdim   = p->dim;
		pdf_highlight_.page   = page;

		pdf_win_.wdim = p->dim;

		pdf_in_cell_ = nullptr;
		if (! pdf_win_.reuse) {
			if (inside) {
				pdf_in_cell_ = *inside;

				Component * c = pdf_in_cell_;
				ImVec2 omin = { float(c->outline[0].x), float(c->outline[0].y) };
				ImVec2 omax = omin;
				for (auto && p : c->outline) {
					if (p.x < omin.x) { omin.x = float(p.x); }
					if (p.y < omin.y) { omin.y = float(p.y); }
					if (p.x > omax.x) { omax.x = float(p.x); }
					if (p.y > omax.y) { omax.y = float(p.y); }
				}

				auto & pdfhi = pdf_highlight_;
				ImVec2 osz = { omax.x - omin.x, omax.y - omin.y };
				float pdfasp = pdfhi.pdim.x / pdfhi.pdim.y;
				if (pdfasp > osz.x / osz.y) {
					float corr = osz.y - osz.x / pdfasp;
					omin.y += corr / 2;
					omax.y -= corr / 2;
				} else {
					float corr = osz.x - osz.y * pdfasp;
					omin.x += corr / 2;
					omax.x -= corr / 2;
				}
				pdf_win_.box.min = omin;
				pdf_win_.box.max = omax;
			} else {
				pdf_win_.box.min = { 0, float(boardview()->m_boardHeight) };
				pdf_win_.box.max = { float(boardview()->m_boardWidth), 2 * float(boardview()->m_boardHeight) };
			}
		}
		pdf_win_.update();

		if (ImGui::IsMouseReleased(0)) {
			pdf_win_.ignore_one_mouse_release = true;
		}
		
		//std::cerr << pdf_win_.box.min << " " << pdf_win_.box.max << " " << pdf_win_.sticky << "\n";
	}

	TCL::page_cache * TCL::render_page(int page) {
		if (!pdoc_) {
			struct stat sbuf;
			if (stat(pdf_filename_.c_str(), &sbuf) >= 0) {
				auto flen  = sbuf.st_size;
				char * buf = new char[flen];
				std::ifstream is(pdf_filename_, std::ios::binary);
				is.read(buf, flen);
				GError * err;
				if ((pdoc_ = poppler_document_new_from_data(buf, flen, nullptr, &err)) == nullptr) {
					std::cerr << "poppler_document_new: " << err->message << "\n";
					return nullptr;
				}
			} else {
				std::cerr << "cannot open " << pdf_filename_ << "\n";
				return nullptr;
			}
		}

		auto pcache = page_cache_.find(page);
		if (pcache == page_cache_.end()) {
			PopplerPage * ppage = poppler_document_get_page(pdoc_, page - 1);
			if (! page) {
				std::cerr << "get_page\n";
				return nullptr;
			}
			double p_width, p_height;
			poppler_page_get_size(ppage, &p_width, &p_height);
				
			bool invert = false;
			const char * theme = boardview()->obvconfig.ParseStr("colorTheme", "default");
			if (strcmp(theme, "dark") == 0) {
				invert = true;
			}
			invert ^= pdf_invert_;
			const int highres = 8;
			int img_width  = int(p_width * highres);
			int img_height = int(p_height * highres);
			int img_buf_size = img_width * img_height * 4;
			unsigned char * img_buf = new unsigned char[img_buf_size];
			cairo_surface_t * surface = cairo_image_surface_create_for_data(img_buf,
																			CAIRO_FORMAT_ARGB32,
																			img_width, img_height, cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, img_width));

			if (auto err = cairo_surface_status(surface); err != CAIRO_STATUS_SUCCESS) {
				std::cerr << "surface_create: " << cairo_status_to_string(err) << "\n";
				return nullptr;
			}
				
			cairo_t * cr = cairo_create(surface);
				
			cairo_scale(cr, highres, highres);
				
			if (invert) {
				cairo_set_source_rgb (cr, 1., 1., 1.);
				cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
				cairo_paint(cr);
			}
				
			cairo_save(cr);
			poppler_page_render(ppage, cr);
			cairo_restore(cr);

			if (invert) {
				cairo_set_operator (cr, CAIRO_OPERATOR_DIFFERENCE);
				cairo_set_source_rgb(cr, 1., 1., 1.);
			} else {
				cairo_set_operator (cr, CAIRO_OPERATOR_DEST_OVER);
				cairo_set_source_rgb (cr, 1, 1, 1);
			}
			cairo_paint(cr);

			//cairo_restore(cr);
			helper_cairo_write_t whelp;
			cairo_surface_write_to_png_stream(surface, cairo_write_func, (void *) &whelp);
			//std::cerr << "render png " << whelp.data.size() << " bytes\n";

			const int thumb_ratio = 30;
			unsigned char * thumb = new unsigned char[(img_width / thumb_ratio + 1) * (img_height / thumb_ratio + 1) * 4];
			{
				const int ratio = thumb_ratio;
				for (int y = 0; y < img_height; y += ratio) {
					for (int x = 0; x < img_width; x += ratio) {
						for (int c = 0; c < 4; ++c) {
							thumb[((x / ratio) + (img_width / ratio) * (y / ratio)) * 4 + c]
								= img_buf[(x + img_width * y) * 4 + c];
						}
					}
				}
			}

			cairo_destroy (cr);
			page_cache cache_ent;

			if (true) {
				cache_ent.thumb = thumb;
				cache_ent.img_buf = new unsigned char[whelp.data.size()];
				memcpy(cache_ent.img_buf, whelp.data.data(), whelp.data.size());
				cache_ent.pdf_img = new Image(cache_ent.img_buf, whelp.data.size(), img_width, img_height, true, true, thumb, img_width / thumb_ratio, img_height / thumb_ratio); //("output.png");
				delete img_buf;
			} else {
				cache_ent.img_buf = img_buf;
				cache_ent.pdf_img = new Image(img_buf, 0, img_width, img_height, false, true);
			}
			cache_ent.dim = ImVec2(float(p_width), float(p_height));
			auto imgp = cache_ent.pdf_img;
			imgp->zoom_factor(boardview()->zoomFactor);
			//imgp->reload();
			cairo_surface_destroy(surface);
			page_cache_.insert(std::make_pair(page, cache_ent));
		}
		auto pcit = page_cache_.find(page);
		return &pcit->second;
		//pcit->second.pdf_img->reload(false);
	}

	void TCL::schematic_open_impl(std::string const & fname, bool bg) {
		globalParams = std::unique_ptr<GlobalParams>(new GlobalParams());
		pdf_filename_ = fname;
		pdf_ = PDFDocFactory().createPDFDoc(GooString(fname), nullptr, nullptr);
		if (! pdf_->isOk()) {
			std::cerr << "cannot open pdf file\n";
		}
		TextOutputDev * txt = new TextOutputDev(nullptr, false, false, false, false);
		if (! txt->isOk()) {
			std::cerr << "cannot create TextOutputDev\n";
			return;
		}
		int last_page = pdf_->getNumPages();
		struct occurence {
			int page;
			float x, y, width, height;
		};

		std::map<std::string, std::pair<Component *, std::vector<occurence> > > pagemap;
		std::map<std::string, std::pair<Net *, std::vector<occurence> > > pagemap_net;
		for (auto && ci : boardview()->m_board->Components()) {
			auto v = std::vector<occurence>();
			v.reserve(32);
			pagemap[ci->name] = std::make_pair(ci.get(), std::move(v));
		}

		for (auto && ni : boardview()->m_board->Nets()) {
			auto v = std::vector<occurence>();
			v.reserve(32);
			pagemap_net[ni->name] = std::make_pair(ni.get(), std::move(v));
		}
			
		for (int page = 1; page < last_page; ++page) {
			//render_page(page);
			pdf_->displayPage(txt, page, 72, 72, 0, true, false, false);
			TextWordList * wlist = txt->makeWordList();
			if (wlist && wlist->getLength()) {
				for (int wi = 0; wi < wlist->getLength(); ++wi) {
					TextWord * w = wlist->get(wi);
					std::string const & s = w->getText()->toStr();
					double x, y, x2, y2;
					w->getBBox(&x, &y, &x2, &y2);
					auto it = pagemap.find(s);
					Component * clickable_cell = nullptr;
					Net * clickable_net = nullptr;

					if (it != pagemap.end()) {
						it->second.second.push_back(occurence{page, float(x), float(y), float(x2 - x), float(y2 - y)});
						//clickable = true;
						clickable_cell = it->second.first;
					} else {
						auto it = pagemap_net.find(s);
						if (it != pagemap_net.end()) {
							//clickable = true;
							clickable_net = it->second.first;
						}
					}
					//if (clickable_cell || clickable_net) {
					if (true) {
						auto it = pdf_words_.find(page);
						if (it == pdf_words_.end()) {
							pdf_words_.insert(std::pair<int, pdf_words_value_t>(page, pdf_words_value_t()));
							it = pdf_words_.find(page);
						}
						ImVec2 min = { float(x), float(y) }, max = { float(x2), float(y2) };
						ImVec2 sz = (max - min) / 2;
						if (sz.x > sz.y) {
							min.y -= sz.y;
							max.y += sz.y;
						} else {
							min.x -= sz.x;
							max.x += sz.x;
						}
						bool clickable = clickable_net || clickable_cell;
						it->second.push_back({ { min, max }, s, clickable, clickable, clickable_cell, clickable_net, page });
					}
				}
			}
		}
		if (bg) {
			boardview()->sleep_mutex_lock();
		}
		for (auto && p : pagemap) {
			std::ostringstream oss;

			std::string str;

			std::unique_copy(p.second.second.begin(), p.second.second.end(),
							 boost::iterators::make_function_output_iterator
							 ([&str](occurence const & v) {
								 if (! str.empty()) str.append(" ");
								 str.append(std::to_string(v.page));
								 str.append(" ");
								 str.append(std::to_string(v.x));
								 str.append(" ");
								 str.append(std::to_string(v.y));
								 str.append(" ");
								 str.append(std::to_string(v.width));
								 str.append(" ");
								 str.append(std::to_string(v.height));
							 }), [](occurence const & a, occurence const & b) {
								 return a.page == b.page;
							 });
			cell_set_prop(p.second.first, "schem_pages", str);
		}
		if (bg) {
			schem_thread_joinable_ = true;
			boardview()->sleep_mutex_unlock();
		}
	}
#else
	void TCL::imgui_draw(ImDrawList *) {
	}
	bool TCL::handle_mouse_drag(ImVec2 const &, ImVec2 const &, bool) {
		return false;
	}
	bool TCL::handle_mouse_wheel(float, float, float) {
		return false;
	}
	bool TCL::grab_mouse_hover() {
		return false;
	}

#endif
	
	
}

#pragma GCC diagnostic pop
