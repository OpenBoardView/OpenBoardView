// -*- c++ -*-
#pragma once


#include <iostream>
#include <sys/time.h>
#include <fnmatch.h>
#include <stdlib.h>
#include <sys/stat.h>

//#define CPPTCL_NO_TCL_STUBS
#define USE_TCL_STUBS

#include <tcl.h>
#include <cpptcl/cpptcl.h>

#include <iterator>
#include <map>
#include <memory>
#include <sstream>
#include <fstream>
#include <set>
#include <thread>
#define _FUNCTION_DEF
#include <readline/readline.h>
#include <readline/history.h>
#undef _FUNCTION_DEF
#include <SDL.h>

#include "BoardView.h"
#include <regex>

//#include "TCLDUMMY.h"


#include <poppler/PDFDocFactory.h>
#include <poppler/TextOutputDev.h>
#include <poppler/CairoOutputDev.h>
#include <poppler/GlobalParams.h>

#include <poppler/glib/poppler.h>
#include <poppler/glib/poppler-document.h>

#include <boost/iterator/function_output_iterator.hpp>

inline bool operator>(ImVec2 const & a, ImVec2 const & b) {
	return a.x > b.x && a.y > b.y;
}
inline bool operator>=(ImVec2 const & a, ImVec2 const & b) {
	return a.x >= b.x && a.y >= b.y;
}
inline bool operator<(ImVec2 const & a, ImVec2 const & b) {
	return a.x < b.x && a.y < b.y;
}
inline bool operator<=(ImVec2 const & a, ImVec2 const & b) {
	return a.x <= b.x && a.y <= b.y;
}
inline bool operator==(ImVec2 const & a, ImVec2 const & b) {
	return a.x == b.x && a.y == b.y;
}
inline bool operator!=(ImVec2 const & a, ImVec2 const & b) {
	return a.x != b.x || a.y != b.y;
}
inline ImVec2 operator-(ImVec2 const & a, ImVec2 const & b) {
	return { a.x - b.x, a.y - b.y };
}
inline ImVec2 operator+(ImVec2 const & a, ImVec2 const & b) {
	return { a.x + b.x, a.y + b.y };
}
inline ImVec2 operator+=(ImVec2 & a, ImVec2 const & b) {
	a = { a.x + b.x, a.y + b.y };
	return a;
}
inline ImVec2 operator-=(ImVec2 & a, ImVec2 const & b) {
	a = { a.x - b.x, a.y - b.y };
	return a;
}
inline ImVec2 operator/(ImVec2 const & a, float f) {
	return { a.x / f, a.y / f };
}
inline ImVec2 operator*(ImVec2 const & a, float f) {
	return { a.x * f, a.y * f };
}
inline ImVec2 operator/(ImVec2 const & a, ImVec2 const & b) {
	return { a.x / b.x, a.y / b.y };
}
inline ImVec2 operator*(ImVec2 const & a, ImVec2 const & b) {
	return { a.x * b.x, a.y * b.y };
}
template <typename C, typename T>
std::basic_ostream<C, T> & operator<<(std::basic_ostream<C, T> & os, ImVec2 const & v) {
	os << v.x << "," << v.y;
	return os;
}


namespace OBV_Tcl {
	using namespace Tcl;
	
	static int of_pins_impl(Component * c, Net * n) {
		if (! c || !n) return -1;
		int ret = 0;
		for (auto && pi : c->pins) {
			if (pi->net == n) ++ret;
		}
		return ret;
	}
	
	template <typename T>
	struct of_pins_t {
		typedef T type;
		template <typename T1, typename T2>
		static int of_pins(T1 *, T2 *) {
			return -1;
		}
	};
	
	template <>
	struct of_pins_t<Component> {
		typedef Net type;
		static int of_pins(Component * c, Net * n) {
			return of_pins_impl(c, n);
		}
	};
	template <>
	struct of_pins_t<Net> {
		typedef Component type;
		static int of_pins(Net * n, type * c) {
			return of_pins_impl(c, n);
		}
	};

	class TCL {
		BoardView * b = nullptr;
		Tcl_Interp * tcl = nullptr;
		Tcl::interpreter * tcli = nullptr;
		
		Board * brd() { return b->m_board.get(); }
		BoardView * boardview() { return b; }

		template <typename T>
		using getopt = Tcl::getopt<T>;
	public :
		struct be_priv {
			std::map<std::string, std::string> properties;
		};
		
		struct cell_priv : public be_priv {
		};
		struct net_priv  : public be_priv {
		};
		struct pin_priv  : public be_priv {
		};

		std::string obj_prop(object const & o) {
			std::ostringstream oss;
			oss << o.get_object() << " " << o.get_object()->typePtr << " " << (o.get_object()->typePtr ? o.get_object()->typePtr->name : "");
			return oss.str();
		}

		template <typename OT>
		typename std::enable_if<std::is_same<OT, Net>::value || std::is_same<OT, Pin>::value || std::is_same<OT, Component>::value || std::is_same<OT, BRDBoard>::value, object>::type makeobj(OT * n) {
			Tcl_Obj * o = Tcl_NewObj();
			o->internalRep.otherValuePtr = (void *) n;
			auto it = tcli->obj_type_by_cppname_.find(std::type_index(typeid(OT)));
			bool ok = it != tcli->obj_type_by_cppname_.end();
			if (ok) {
				o->typePtr = it->second;
			} else {
				throw tcl_error("type lookup failed");
			}
			Tcl_InvalidateStringRep(o);
			return object(o);
		}

		std::vector<BRDBoard *> default_boards_;

		object default_boards(variadic<object> argv) {
			if (argv.size()) { }
			return object(false);
#if 0
			bool cleared = false;

			for (std::size_t ai = 0; ai < alen; ++ai) {
				object ao = argv.is_list() ? argv.at_ref(ai, I) : argv;
			
				std::size_t llen = ao.is_list() ? ao.size(I) : 1;

				for (std::size_t aii = 0; aii < llen; ++aii) {
					Tcl_Obj * to = ao.is_list() ? ao.at_ref(aii, I).get_object() : ao.get_object();
					BRDBoard * b;

					if ((b = try_board(to))) {
						if (! cleared) { default_boards_.clear(); cleared = true; }
						default_boards_.push_back(b);
					}
				}
			}

			object r;
			for (std::size_t i = 0; i < default_boards_.size(); ++i) {
				r.append(makeobj(default_boards_[i]));
			}
		
			if (r.size(I) == 0) {
				bool found = false;
				for (std::size_t i = 0; i < boards_.size(); ++i) {
					if (boardview()->m_board.get() == static_cast<Board *>(boards_[i]))
						found = true;
					r.append(makeobj(boards_[i]));
				}
				if (!found) {
					r.append(makeobj(static_cast<BRDBoard *>(boardview()->m_board.get())));
				}
			}
			return r;
#endif
		}
		object get_boards(variadic<object> argv) {
			if (argv.size()) { }
			object r;
			bool found = false;
			for (std::size_t i = 0; i < boards_.size(); ++i) {
				if (boardview()->m_board.get() == static_cast<Board *>(boards_[i]))
					found = true;
				r.append(makeobj(boards_[i]));
			}
			if (!found) {
				r.append(makeobj(static_cast<BRDBoard *>(boardview()->m_board.get())));
			}
			return r;
		}

		void draw_board(BRDBoard * b) {
			for (std::size_t i = 0; i < boards_.size(); ++i) {
				if (boards_[i] == b) {
					current_draw_file_ = std::shared_ptr<BRDFile>(boardfiles_[i], [] (auto) { });
					break;
				}
			}
			current_draw_board_ = std::shared_ptr<BRDBoard>(b, [] (auto) { });
			boardview()->SetFile(current_draw_file_, current_draw_board_);
		}

		std::shared_ptr<BRDFile> current_draw_file_;
		std::shared_ptr<BRDBoard> current_draw_board_;

		static constexpr const char * get_nets_opt = "re all not gnd of filter";
		object get_nets(getopt<bool> const & use_regex, getopt<bool> const & all, getopt<list<Net> > const & not_, getopt<bool> const & gnd, getopt<any<list<Component>, list<Pin>, list<Net>, list<BRDBoard> > > const & of, getopt<std::string> const & filter_arg, variadic<std::string> const & match) {
			object r;

			object filter_o; int filter = 0; std::string filter_ns;

			if (filter_arg) {
				filter = filter_ns_ix++ % 1000;
				filter_ns = namespace_str(filter);
				namespace_open(*tcli, filter_ns);
				filter_o = object(std::string("namespace eval ") + filter_ns + " { expr { " + *filter_arg + " } }");
			}
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
			
			auto try_append = [&](Net * n, Component * of = nullptr) {
				if ((gnd || !n->is_ground)
					&& (all || dup.find(n) == dup.end())
					&& not_set.find(n) == not_set.end()
					&& matches(match, re, n->name, use_regex)
					&& filter_match(*tcli, filter_ns, filter_o, n, of)) {
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

		static bool argp(const std::string & arg, const char * str) {
			return arg.compare(1, arg.size() - 1, str) == 0;
		}		

		static bool matches(const variadic<std::string> & glob, const std::regex & re, const std::string & s, bool use_re) {
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

		int filter_ns_ix = 1;

		std::string namespace_str(int ix) {
			std::ostringstream oss;
			oss << "_obv_filter_" << ix;
			return oss.str();
		}

		void namespace_open(interpreter & I, std::string const & ns) {
			try {
				I.eval(std::string("namespace eval ") + ns + " { set _obv_nsopen 1 }");
			} catch (std::exception & e) {
				std::cerr << e.what() << "\n";
			}
		}

		static constexpr const char * get_pins_opt = "re all parallel filter of";
		object get_pins(getopt<bool> const & use_regex, getopt<bool> const & all, getopt<bool> const & parallel, getopt<std::string> const & filter_arg, getopt<any<list<Component>, list<Net>, list<Pin>, list<BRDBoard> > > const & of, variadic<std::string> const & match) {
			object r;
			object filter_o;
			int filter = 0;
			std::string filter_ns;

			if (filter_arg) {
				filter = filter_ns_ix++ % 1000;
				filter_ns = namespace_str(filter);
				namespace_open(*tcli, filter_ns);
				filter_o = object(std::string("namespace eval ") + filter_ns + " { expr { " + *filter_arg + " } }");
			}

			std::regex re;
			if (use_regex && match.size() == 1) re = match[0];

			std::set<Pin *> dup;
			bool match_has_slash = true; //std::find(match.begin(), match.end(), '/') != match.end();

			auto try_append = [&](Pin * c) {
				std::string m(c->name);
				if (match_has_slash) {
					m = c->component->name + "/" + c->name;
				}
				if ((all || dup.find(c) == dup.end())
					&& matches(match ? match : variadic<std::string>{}, re, m, use_regex)
					&& filter_match(*tcli, filter_ns, filter_o, c)) {
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

		void filter_install(interpreter & I, std::string const & ns, Component * c) {
			for (int i = 0; cell_props_[i]; ++i) {
				I.setVar(ns + "::" + cell_props_[i], cell_get_prop(cell_props_[i], c));
			}
			if (c->tcl_priv) {
				be_priv * pr = (be_priv *) c->tcl_priv;
				for (auto && i : pr->properties) {
					I.setVar(ns + "::" + i.first, i.second);
				}
			}
		}
		void filter_install(interpreter & I, std::string const & ns, Net * n) {
			for (int i = 0; net_props_[i]; ++i) {
				I.setVar(ns + "::" + net_props_[i], net_get_prop(net_props_[i], n));
			}		
		}
		void filter_install(interpreter & I, std::string const & ns, Pin * p) {
			for (int i = 0; pin_props_[i]; ++i) {
				I.setVar(ns + "::" + pin_props_[i], pin_get_prop(pin_props_[i], p));
			}
		}

		static constexpr char const * cell_props_[] = { "type", "pins", "has_gnd", nullptr };
		static constexpr char const * pin_props_ [] = { "type", "ispad", nullptr };
		static constexpr char const * net_props_ [] = { "isgnd", nullptr };

		template <typename T>
		using is_CNP = std::integral_constant<bool, std::is_same<T, Pin>::value || std::is_same<T, Net>::value || std::is_same<T, Component>::value>;

		template <typename OT>
		typename std::enable_if<is_CNP<OT>::value && is_CNP<typename of_pins_t<OT>::type>::value, bool>::type filter_match(interpreter & I, std::string const & ns, object const & eval, OT * p, typename of_pins_t<OT>::type * of = nullptr) {
			if (ns.empty()) return true;

			filter_install(I, ns, p);
			I.setVar(ns + "::bundle", object(of_pins_t<OT>::of_pins(p, of)));

			return I.eval(eval);
			try {
			} catch (std::exception const & e) {
				std::cerr << eval.get<std::string>() << ": " << e.what() << "\n";
				return false;
			} catch (...) {
				std::cerr << "unknown exception\n";
				return false;
			}
		}
		static bool is_dummy(Component * c) {
			return c->component_type == Component::kComponentTypeDummy;
		}

		struct Area {
			Point center;
			enum shape_t {
				circle,
				rect
			};
			shape_t shape;
			int size;
			uint32_t shade = 0;
			float intensity = 1.0;
		};
		
#if 0
		static constexpr const char * get_area_opt = "of circle rect overlap";
		object get_area(getopt<list<Component> > of, getopt<bool> circle, getopt<bool> rect, getopt<bool> overlap) {
			if (of && *of) {
				for (Component * c : *of) {
				}
			}
		}
#endif
		const char * get_cells_opt = "filter re of all top bottom not";
		object get_cells(getopt<std::string> const & filter_arg, getopt<bool> const & use_regex, getopt<any<list<Component>, list<Net>, list<Pin> > > const & of, getopt<bool> const & all,
						 getopt<bool> const & top,
						 getopt<bool> const & bottom,
						 getopt<list<Component> > const & not_, variadic<std::string> const & match) {
			object r;
			int filter = 0;
			std::string filter_ns;
			object filter_o;
			std::regex re;

			if (use_regex && match) re = match[0];

			if (filter_arg) {
				filter = filter_ns_ix++ % 1000;
				filter_ns = namespace_str(filter);
				namespace_open(*tcli, filter_ns);
				filter_o = object(std::string("namespace eval ") + namespace_str(filter) + " { expr { " + *filter_arg + " } }");
			}
			
			std::set<Component *> dup;
			std::set<Component *> not_set;

			if (not_ && *not_) {
				for (Component * c : *not_) {
					not_set.insert(c);
				}
			}

			auto try_append = [&](Component * c) {
				if (!is_dummy(c)
					&& ((! top && ! bottom) || (top && c->board_side == kBoardSideTop) || (bottom && c->board_side == kBoardSideBottom))
					&& (all || dup.find(c) == dup.end())
					&& not_set.find(c) == not_set.end()
					&& matches(match, re, c->name, use_regex)
					&& filter_match(*tcli, filter_ns, filter_o, c)) {
					r.append(makeobj(c));
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
						  });
			} else {
				bool found = !default_boards_.empty();
				for (std::size_t bi = 0; bi < (default_boards_.empty() ? boards_.size() : default_boards_.size()); ++bi) {
					BRDBoard * b = default_boards_.empty() ? boards_[bi] : default_boards_[bi];
					if (static_cast<Board *>(b) == boardview()->m_board.get()) found = true;
					for (std::size_t i = 0; i < b->Components().size(); ++i) {
						//if (!is_dummy(c) && matches(match, re, c->name, use_regex) && filter_match(I, filter, filter_o, c) && dup.find(c) == dup.end()) {
						try_append(&(*b->Components()[i]));
					}
				}
				if (!found) {
					Board * b = boardview()->m_board.get();
					if (b) {
						for (std::size_t i = 0; i < b->Components().size(); ++i) {
							//if (!is_dummy(c) && matches(match, re, c->name, use_regex) && filter_match(I, filter, filter_o, c) && dup.find(c) == dup.end()) {
							try_append(&(*b->Components()[i]));
						}
					}
				}				
			}
			return r;
		}

		object selection() {
			object r;

			for (auto && pi : boardview()->m_pinHighlighted) {
				r.append(makeobj(pi.get()));
			}
			for (auto && ci : boardview()->m_partHighlighted) {
				r.append(makeobj(ci.get()));
			}
			return r;
		}

		struct highlight_delay {
			Component * c;
			uint32_t shade;
		};
		std::deque<highlight_delay> highlight_delay_;
		static const bool highlight_delay_enable_ = false;
		uint64_t highlight_delay_time_ = 0;

		static uint64_t time_ms() {
			timeval t;
			gettimeofday(&t, nullptr);
			return t.tv_sec * 1000 + t.tv_usec / 1000;
		}
	
		void highlight_delay_poll() {
			if (highlight_delay_enable_ && ! highlight_delay_.empty()) {
				uint64_t now = time_ms();
				if (highlight_delay_time_ + 30 < now) {
					highlight_delay & d = highlight_delay_.front();
					if (d.c->shade_color_ != d.shade) {
						d.c->shade_color_ = d.shade;
						highlight_delay_time_ = now;
					}
					highlight_delay_.pop_front();
				}
			}
		}
	
		static constexpr const char * highlight_opt = "un all color intensity";
		void highlight(getopt<bool> const & un, getopt<bool> const & all, getopt<std::string> const & color, getopt<float> const & inten, opt<list<any<Component, Pin> > > const & obj) {
			uint32_t color_v = 0xffaaaaaa;

			if (color) {
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
							 });
				}
			}
		}

		static bool icmp(const char * p1, const char * p2) {
			return strcasecmp(p1, p2) == 0;
		}
		static bool icmp(const std::string & str, const char * str2) {
			return icmp(str.c_str(), str2);
		}
		object extra_properties_get(const char * prop, be_priv * pr) {
			if (pr) {
				auto it = pr->properties.find(prop);
				if (it != pr->properties.end()) {
					return object(it->second);
				}
			}
			return object("");
		}

		object cell_get_prop(const char * prop, Component * c) {
			if (icmp(prop, "type")) {
				std::ostringstream oss;
				oss << c->component_type;
				return object(oss.str());
			} else if (icmp(prop, "pins")) {
				return object((int) c->pins.size());
				//std::ostringstream oss;
				//oss << c->pins.size();
				//return oss.str();
			} else if (icmp(prop, "has_gnd")) {
				for (auto && pi : c->pins) {
					Net * n = pi->net;
					if (n && n->is_ground) {
						return object(true);
					}
				}
				return object(false);
			}
			return extra_properties_get(prop, (be_priv *) c->tcl_priv);
		}
		object pin_get_prop(const char * prop, Pin * p) {
			if (icmp(prop, "type")) {
				return object("the_type");
			} else if (icmp(prop, "isgnd")) {
				return object(p->net && p->net->is_ground);
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
							return object(true);
						}
					}
				}
				return object(false);
			}
			return extra_properties_get(prop, (be_priv *) p->tcl_priv);
		}
		object net_get_prop(const char * prop, Net * n) {
			if (icmp(prop, "isgnd")) {
				return object(n->is_ground);
			}
			return extra_properties_get(prop, (be_priv *) n->tcl_priv);
		}

		std::string tcl_typename(object const & o) {
			return tcl_typename(o.get_object());
			if (o.get_object()->typePtr && o.get_object()->typePtr->name) {
				return o.get_object()->typePtr->name;
			} else {
				return "(notype)";
			}
		}
		std::string tcl_typename(Tcl_Obj * o) {
			if (o->typePtr) {
				if (o->typePtr->name) {
					return o->typePtr->name;
				}
				return "(notypename)";
			}
			return "(notype)";
		}

		typedef any<Component, Net, Pin, Board> any_obv_type;

		void cell_set_prop(Component * c, std::string const & name, std::string const & value) {
			if (! c->tcl_priv) { c->tcl_priv = (void *) new cell_priv(); }
			be_priv * pr = (be_priv *) c->tcl_priv;
			pr->properties[name] = value;
		}
			
		
		void set_prop(std::string const & prop, std::string const & value, list<any<Component, Net, Pin> > const & obj) {
			for (auto a : obj) {
				be_priv * pr;
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
						});

				pr->properties[prop] = value;
			}
		}
		
		object get_prop(std::string const & prop, list<any_obv_type> const & obj) {
			object r;
			std::size_t sz = obj.size();
			for (std::size_t i = 0; i < sz; ++i) {
				auto le = obj.at(i);

				le.visit([&] (Component * c) { r.append(cell_get_prop(prop.c_str(), c)); },
						 [&] (Net       * n) { r.append(net_get_prop (prop.c_str(), n)); },
						 [&] (Pin       * p) { r.append(pin_get_prop (prop.c_str(), p)); });

			}
			return r;
		}
		
		std::vector<std::string> keybind_targets() {
			std::vector<std::string> ret;
			//return { "testkb" };
			object r = tcli->eval("info commands");
			interpreter I(r.get_interp(), false);
			for (std::size_t i = 0; i < r.size(I); ++i) {
				std::string s = r.at_ref(i, I).get<std::string>();
				if (s.compare(0, 3, "kb_") == 0) {
					ret.push_back(s.substr(3));
				}
			}
			return ret;
		}

		void keypress(std::string const & k) {
			try {
				tcli->eval(std::string("kb_") + k);
			} catch (std::exception & e) {
				tcli->terr() << e.what() << "\n";
			}
		}
	
		bool trace(opt<bool> v) {
			if (v) {
				trace_ = *v;
				tcli->trace(*v);
			}
			return trace_;
		}
		bool trace_ = false;

		void select(list<any_obv_type> obj) {
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

		PDFDoc * pdf_ = nullptr;
		std::string pdf_filename_;
		PopplerDocument * pdoc_ = nullptr;

		Image * pdf_img_ = nullptr;
		unsigned char * pdf_img_buf_ = nullptr;

		struct bbox {
			ImVec2 min, max;
		};

		struct pdf_window {
			bbox   box;
			ImVec2 wdim;

			pdf_window() { }
			pdf_window(bbox const & bb, ImVec2 const & wd) : box(bb), wdim(wd) {
				update();
			}
			void update() {
				bdim     = box.max - box.min;
				aspect   = bdim.x / bdim.y;
				wb_ratio = wdim / bdim;
				//std::cerr << "wb " << wb_ratio << " " << wdim << " " << bdim << "\n";
				//std::cerr << box.min << " " << box.max << "\n";
			}				

			
			ImVec2 bdim;     // derived
			float  aspect;   // derived
			ImVec2 wb_ratio; // derived
			int    page;
			bool   valid = false;
			bool   reuse = false;
			bool   sticky = false;
		};
		
		ImVec2 pdf2board(pdf_window const & w, ImVec2 c, bool flip) {
			if (!flip) {
				c.y = w.wdim.y - c.y;
			}
			return w.box.min + c / w.wb_ratio;
			ImVec2 pr = w.wdim / c;
			return w.box.min + pr * w.bdim;
		}
		ImVec2 board2pdf(pdf_window const & w, ImVec2 c, bool flip) {
			if (!flip) { }
			return (c - w.box.min) * w.wb_ratio;
			ImVec2 pr = (c - w.box.min) / w.bdim;
			return pr * w.wdim;
		}

		pdf_window pdf_win_;
		
		struct pdf_bbox {
			//float x, y, w, h;
			bbox box;
			bool valid = false;
			ImVec2 pdim;
			int page;
		};

		pdf_bbox pdf_highlight_;

		Component * pdf_in_cell_ = nullptr;
		//ImVec2 pdf_min_, pdf_max_;

		void imgui_draw(ImDrawList * draw) {
			if (pdf_img_) {
				//imgui_draw_in(draw, pdf_min_, pdf_max_);
				imgui_draw_in(draw, pdf_win_.box.min, pdf_win_.box.max);
			}
		}

		struct pdf_txt_bbox {
			bbox box;
			//float x, y, xmax, ymax;
			std::string word;
		};
		typedef std::vector<pdf_txt_bbox> pdf_words_value_t;
		std::map<int, pdf_words_value_t> pdf_words_;

		void imgui_draw_in(ImDrawList * draw, ImVec2 min, ImVec2 max) {
			if (pdf_img_) {
				{
					if (true) {
						ImVec2 min_flip = min;
						ImVec2 max_flip = max;
						std::swap(min_flip.y, max_flip.y);

						if (boardview()->m_current_side) {
							pdf_img_->render(*draw, boardview()->CoordToScreen(min.x, max.y), boardview()->CoordToScreen(max.x, min.y), 0);
						} else {
							pdf_img_->render(*draw, pdf_win_.sticky ? min_flip : boardview()->CoordToScreen(min), pdf_win_.sticky ? max_flip : boardview()->CoordToScreen(max), 0);
						}
						ImVec2 spos = ImGui::GetMousePos();
						ImVec2 pos  = boardview()->ScreenToCoord(spos.x, spos.y);
						
						if (pos >= min && pos <= max) {
							ImVec2 ppos1 = pos - min;
							ImVec2 psz1  = max - min;
							auto & phi = pdf_highlight_;
							ImVec2 ppos = { ppos1.x / psz1.x * phi.pdim.x, phi.pdim.y - ppos1.y / psz1.y * phi.pdim.y };
							//std::cerr << ppos.x << " " << ppos.y << "\n";
							
							auto it = pdf_words_.find(phi.page);
							if (it != pdf_words_.end()) {
								for (auto & w : it->second) {
									if (ppos.x > w.box.min.x && ppos.y > w.box.min.y && ppos.x < w.box.max.x && ppos.y < w.box.max.y) {
										//std::cerr << ppos.x << " " << ppos.y << " word: " << w.word << "\n";
										
										ImVec2 xpa, xpb{ w.box.min };
										
										for (int i = 0; i < 4; ++i) {
											xpa = xpb;
											if (i == 0)      { xpb = { w.box.max.x, w.box.min.y }; }
											else if (i == 1) { xpb = { w.box.max.x, w.box.max.y }; }
											else if (i == 2) { xpb = { w.box.min.x, w.box.max.y }; }
											else if (i == 3) { xpb = { w.box.min.x, w.box.min.y }; }
											
											ImVec2 bpa = pdf2board(pdf_win_, xpa, boardview()->m_current_side);
											ImVec2 bpb = pdf2board(pdf_win_, xpb, boardview()->m_current_side);
											
											ImVec2 spa = boardview()->CoordToScreen(bpa.x, bpa.y);// + boardview()->m_boardHeight);
											ImVec2 spb = boardview()->CoordToScreen(bpb.x, bpb.y);// + boardview()->m_boardHeight);
											
											draw->AddLine(spa, spb, 0xff00ffff, 5);
										
										}
										
										break;
									}
								}
							}
						}
					}
				}

				pdf_bbox & ref = pdf_highlight_;
				if (ref.valid) {
					ImVec2 xpa, xpb{ ref.box.min.x, ref.box.min.y };

					for (int i = 0; i < 4; ++i) {
						xpa = xpb;
						if (i == 0)      { xpb = { ref.box.max.x, ref.box.min.y }; }
						else if (i == 1) { xpb = { ref.box.max.x, ref.box.max.y }; }
						else if (i == 2) { xpb = { ref.box.min.x, ref.box.max.y }; }
						else if (i == 3) { xpb = { ref.box.min.x, ref.box.min.y }; }

						ImVec2 bpa = pdf2board(pdf_win_, xpa, boardview()->m_current_side);
						ImVec2 bpb = pdf2board(pdf_win_, xpb, boardview()->m_current_side);
						
						ImVec2 spa = pdf_win_.sticky ? bpa : boardview()->CoordToScreen(bpa.x, bpa.y);
						ImVec2 spb = pdf_win_.sticky ? bpb : boardview()->CoordToScreen(bpb.x, bpb.y);

						draw->AddLine(spa, spb, 0xff0000ff, 10);
					}
				}
			}
		}

		bool handle_mouse_drag(ImVec2 const & ppos, ImVec2 const & drag, bool token) {
			ImGuiIO &io = ImGui::GetIO();
			ImVec2 pos = pdf_win_.sticky ? ppos : boardview()->ScreenToCoord(ppos);

			
			if (token || (pos > pdf_win_.box.min && pos < pdf_win_.box.max)) {
				if (io.KeyCtrl) {
					//ImVec2 bmin = boardview()->ScreenToCoord(0, 0);
					//ImVec2 bmax = boardview()->ScreenToCoord(ImGui::GetWindowSize());

					ImVec2 bdrag = pdf_win_.sticky ? drag : boardview()->ScreenToCoord(drag, 0);
					
					pdf_win_.box.min += bdrag;
					pdf_win_.box.max += bdrag;
					pdf_win_.reuse = true;
					return true;
				}
			}
			return false;
		}
		
		bool handle_mouse_wheel(float x, float y, float wh) {
			ImVec2 spos = { x, y };
			ImVec2 mpos = pdf_win_.sticky ? spos : boardview()->ScreenToCoord(spos);
			ImGuiIO &io = ImGui::GetIO();

			//std::cerr << mpos << " " << pdf_win_.box.min << " " << pdf_win_.box.max << "\n";
			
			if (mpos > pdf_win_.box.min && mpos < pdf_win_.box.max) {
				if (io.KeyCtrl) {
					ImVec2 pdfsz = pdf_win_.bdim;
					if (wh > 0) {
						pdf_win_.box.max += pdfsz / 10;
						pdf_win_.box.min -= pdfsz / 10;
					} else if (wh < 0) {
						pdf_win_.box.max -= pdfsz / 10;
						pdf_win_.box.min += pdfsz / 10;
					}
				} else if (io.KeyShift || pdf_win_.sticky) {
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


		bool pdf_sticky(opt<bool> const & v) {
			if (v) {
				if (! pdf_win_.sticky && *v) {
					pdf_win_.box.min = boardview()->CoordToScreen(pdf_win_.box.min);
					pdf_win_.box.max = boardview()->CoordToScreen(pdf_win_.box.max);
					std::swap(pdf_win_.box.min.y, pdf_win_.box.max.y);
				} else if (pdf_win_.sticky && !*v) {
					pdf_win_.box.min = boardview()->ScreenToCoord(pdf_win_.box.min);
					pdf_win_.box.max = boardview()->ScreenToCoord(pdf_win_.box.max);
					std::swap(pdf_win_.box.min.y, pdf_win_.box.max.y);
				}

				pdf_win_.sticky = *v;
			}
			return pdf_win_.sticky;
		}
		
		// e__l
		void schem_highlight(list<float> x, opt<float> y, opt<float> w, opt<float> h) {	
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
				ref.box.min -= ref.box.max / 2;// x -= ref.w / 2; //* 10 / 100;
				ref.box.max += ref.box.max; // * 20 / 100;
				ref.box.max += ref.box.min;
			}
		}
		void undraw_page() {
			pdf_highlight_.valid = false;
			if (pdf_img_) {
				delete pdf_img_;
				pdf_img_ = nullptr;
			}
			if (pdf_img_buf_) {
				delete pdf_img_buf_;
				pdf_img_buf_ = nullptr;
			}
			pdf_win_.reuse = false;
			pdf_win_.sticky = false;
		}
		void draw_page(int page, opt<Component *> inside) {
			if (!pdoc_) {
				struct stat sbuf;
				if (stat(pdf_filename_.c_str(), &sbuf) >= 0) {
					auto flen = sbuf.st_size;
					char * buf = new char[flen];
					std::ifstream is(pdf_filename_, std::ios::binary);
					is.read(buf, flen);
					GError * err;
					if ((pdoc_ = poppler_document_new_from_data(buf, flen, nullptr, &err)) == nullptr) {
						std::cerr << "poppler_document_new: " << err->message << "\n";
						return;
					}
				} else {
					std::cerr << "cannot open " << pdf_filename_ << "\n";
					return;
				}
			}

			PopplerPage * ppage = poppler_document_get_page(pdoc_, page - 1);
			if (! page) {
				std::cerr << "get_page\n";
				return;
			}
			double p_width, p_height;
			poppler_page_get_size(ppage, &p_width, &p_height);

			bool invert = false;
			const char * theme = boardview()->obvconfig.ParseStr("colorTheme", "default");
			if (strcmp(theme, "dark") == 0) {
				invert = true;
			}
			int img_width  = int(p_width * 8);
			int img_height = int(p_height * 8);
			unsigned char * img_buf = new unsigned char[img_width * img_height * 4 + 1 * 1024];
			cairo_surface_t * surface = cairo_image_surface_create_for_data(img_buf,
																			CAIRO_FORMAT_ARGB32,
																			img_width, img_height, cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, img_width));

			if (auto err = cairo_surface_status(surface); err != CAIRO_STATUS_SUCCESS) {
				std::cerr << "surface_create: " << cairo_status_to_string(err) << "\n";
				return;
			}
			
			cairo_t * cr = cairo_create(surface);

			cairo_scale(cr, 8, 8);
			
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

			cairo_destroy (cr);

			if (pdf_img_) {
				delete pdf_img_;
			}
			if (pdf_img_buf_) {
				delete pdf_img_buf_;
			}
			pdf_img_buf_ = img_buf;
			pdf_img_ = new Image(img_buf, img_width, img_height, true); //("output.png");
			pdf_img_->reload();
			cairo_surface_destroy(surface);

			pdf_highlight_.valid  = false;
			pdf_highlight_.pdim.x = float(p_width);
			pdf_highlight_.pdim.y = float(p_height);
			pdf_highlight_.page = page;

			pdf_win_.wdim = { float(p_width), float(p_height) };
			
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
		}

		std::thread * schem_open_thread_ = nullptr;
		std::atomic<bool> schem_thread_joinable_ = false;

		static constexpr const char * schematic_open_opt = "background";
		void schematic_open(getopt<bool> const & background, std::string const & fname) {
			schem_thread_joinable_ = false;
			if (background) {
				schem_open_thread_ = new std::thread([fname, this] { schematic_open_impl(fname, true); });
			} else {
				schematic_open_impl(fname, false);
			}
		}

		void schematic_open_impl(std::string const & fname, bool bg) {
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
				pdf_->displayPage(txt, page, 72, 72, 0, true, false, false);
				TextWordList * wlist = txt->makeWordList();
				if (wlist && wlist->getLength()) {
					for (int wi = 0; wi < wlist->getLength(); ++wi) {
						TextWord * w = wlist->get(wi);
						std::string const & s = w->getText()->toStr();
						double x, y, x2, y2;
						w->getBBox(&x, &y, &x2, &y2);
						auto it = pagemap.find(s);
						bool clickable = false;
						if (it != pagemap.end()) {
							it->second.second.push_back(occurence{page, float(x), float(y), float(x2 - x), float(y2 - y)});
							clickable = true;
						} else {
							auto it = pagemap_net.find(s);
							if (it != pagemap_net.end()) {
								clickable = true;
							}
						}
						if (clickable) {
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

							it->second.push_back({ { min, max }, s });
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
		
		object file_history() {
			object r;

			for (int i = 0; i < boardview()->fhistory.count; ++i) {
				r.append(object(boardview()->fhistory.history[i]));
			}
			return r;
		}

		typedef std::vector<BRDBoard *> boards_t;
		boards_t boards_;
		typedef std::vector<BRDFile *> boardfiles_t;
		boardfiles_t boardfiles_;
	
		void obv_open(std::string const & fname) {
			//boardview()->LoadFile(fname);

			BRDFile * f = boardview()->loadBoard(fname);
			if (f) {
				boardfiles_.push_back(f);
			
				boards_.push_back(new BRDBoard(f));
			}
		}

		void quit() {
			rl_callback_handler_remove();
			farewell_newline_ = false;
			*done_ = true;
		}

		void report_prop(list<any_obv_type> obj) {
			std::ostringstream oss;

			for (std::size_t ai = 0; ai < obj.size(); ++ai) {
				auto le = obj.at(ai);

				be_priv * pr = nullptr;
				//oss << "object=" << to << " type=" << (to->typePtr && to->typePtr->name ? to->typePtr->name : "(none)") << "[" << to->typePtr << "]\n";
				if (Net * n = le.as<Net>()) {
					for (int i = 0; net_props_[i]; ++i) {
						oss << net_props_[i] << ":" << net_get_prop(net_props_[i], n).get<std::string>() << "\n";
					}
					pr = (be_priv *) n->tcl_priv;
				} else if (Component * c = le.as<Component>()) {
					for (int i = 0; cell_props_[i]; ++i) {
						oss << cell_props_[i] << ": " << cell_get_prop(cell_props_[i], c) << "\n";
					}
					pr = (be_priv *) c->tcl_priv;
				} else if (Pin * p = le.as<Pin>()) {
					for (int i = 0; pin_props_[i]; ++i) {
						oss << pin_props_[i] << ": " << pin_get_prop(pin_props_[i], p) << "\n";
					}
					pr = (be_priv *) p->tcl_priv;
				}
				if (pr) {
					for (auto it = pr->properties.begin(); it != pr->properties.end(); ++it) {
						oss << it->first << ": " << it->second << "\n";
					}
				}
			}
			std::cerr << oss.str();
		}

		object last_result() {
			return last_result_;
		}
	
		bool * done_ = nullptr;

		void notify_pin_hover(Pin * now, Pin * before) {
			if (now && before) { }
			//std::cerr << "hover pin " << now << " " << (now ? now->name : "") << "\n";
		}
		void notify_part_hover(Component * now, Component * before) {
			if (before) { }
			//std::cerr << "hover cell " << now << " " << (now ? now->name : "") << " " << before << " " << (before ? before->name : "") << "\n";
			object argv;
			argv.append(object("cell_hover_call"));
			if (now) {
				argv.append(makeobj(now));
			} else {
				argv.append(object(""));
			}
			
			//std::cerr << argv.get<std::string>() << "\n";
			try {
				tcli->eval(argv);
			} catch (std::exception & e) {
				std::cerr << e.what() << "\n";
			}
		}

		void notify_load_file() {
			if (! startup_script.empty()) {
				auto r = tcli->eval(startup_script);
				std::cerr << (std::string) r << "\n";
				startup_script.clear();
			}
		}

		std::string startup_script;
		
		TCL(BoardView * bvv, bool * d, std::string const & script) : b(bvv), done_(d) {
			tcl = Tcl_CreateInterp();
		
			tcli = new interpreter(tcl, true);

			typedef TCL this_t;

			struct cell_ops : public interpreter::type_ops {
				static void str(Tcl_Obj * o) {
					Component * c = (Component *) o->internalRep.otherValuePtr;
					//std::cerr << "str_cell ";
					//std::cerr << c << " ";
					//std::cerr << "str_cell " << c << " " << c->name << " " << c->name.size() << " ";
					str_impl(o, c->name);
				}
			};
			struct net_ops : public interpreter::type_ops {
				static void str(Tcl_Obj * o) {
					Net * n = (Net *) o->internalRep.otherValuePtr;
					str_impl(o, n->name);
				}
			};
			struct pin_ops : public interpreter::type_ops {
				static void str(Tcl_Obj * o) {
					Pin * p = (Pin *) o->internalRep.otherValuePtr;
					str_impl(o, p->component->name + "/" + p->name);
				}
			};
			struct board_ops : public interpreter::type_ops {
				static void str(Tcl_Obj * o) {
					BRDBoard * b = (BRDBoard *) o->internalRep.otherValuePtr;
					std::ostringstream oss;
					oss << "brd" << (void *) b;
					str_impl(o, oss.str());
				}
			};
			tcli->type<cell_ops> ("cell",  (Component *) nullptr);
			tcli->type<net_ops>  ("net",   (Net       *) nullptr);
			tcli->type<pin_ops>  ("pin",   (Pin       *) nullptr);
			tcli->type<board_ops>("board", (BRDBoard  *) nullptr);
			
			tcli->def(this)
				.def("get_nets",        &this_t::get_nets,  policies(), get_nets_opt)
				.def("get_cells",       &this_t::get_cells, policies(), get_cells_opt)
				.def("get_pins",        &this_t::get_pins,  policies(), get_pins_opt)
				.def("get_boards",      &this_t::get_boards)
				.def("default_boards",  &this_t::default_boards)
				.def("draw_board",      &this_t::draw_board)
				.def("report_prop",     &this_t::report_prop)
				.def("highlight",       &this_t::highlight, policies(), this_t::highlight_opt)
				.def("selection",       &this_t::selection)
				.def("schem_open",      &this_t::schematic_open, policies(), schematic_open_opt)
				.def("schem_sticky",    &this_t::pdf_sticky)
				.def("draw_page",       &this_t::draw_page)
				.def("undraw_page",     &this_t::undraw_page)
				.def("schem_highlight", &this_t::schem_highlight)
				.def("get_prop",        &this_t::get_prop)
				.def("set_prop",        &this_t::set_prop)
				.def("trace",           &this_t::trace)
				.def("select",          &this_t::select)
				.def("file_history",    &this_t::file_history)
				.def("obv_open",        &this_t::obv_open)
				.def("exit",            &this_t::quit)
				.def("last_result",     &this_t::last_result);

			tcli->defvar("default_intensity", boardview()->m_default_intensity);
			tcli->defvar("dual_draw", boardview()->m_draw_both_sides);
			
#if 0
			tcli->class_<ImVec2>("ImVec2", init<float, float>())
				.defvar("x", &ImVec2::x)
				.defvar("y", &ImVec2::y);
#endif
			
			Tcl_Init(tcl);

			this_s_ = this;
			rl_prep_terminal(0);
			rl_attempted_completion_function = rl_complete;
			rl_callback_handler_install("> ", rl_cb);

			Tcl_SetExitProc(exit_proc);
			tcli->eval("source library.tcl");
			startup_script = script;
		}

		static void exit_proc(ClientData) {
			*this_s_->done_ = true;
		}
	
		bool farewell_newline_ = true;
		~TCL() {
			rl_callback_handler_remove();
			rl_deprep_terminal();
			if (farewell_newline_) std::cerr << "\n";
		}

		static TCL * this_s_;

		static char ** rl_complete(const char * text, int start, int end) {
			if (start && end) { }
			rl_attempted_completion_over = 1;
			return rl_completion_matches(text, rl_completion_generator);
		}
		static char * rl_completion_generator(const char * text, int state) {
			static std::size_t ix = std::string::npos, len = 0;
			static std::string commands;
		
			if (state == 0) {
				auto res = this_s_->eval("info commands", true);
				commands = res.str;
				ix = 0;
				len = strlen(text);
			}

			while (ix != std::string::npos) {
				std::size_t pos = commands.find(' ', ix);
				if (pos == std::string::npos) {
					if (commands.compare(ix, std::min(commands.size() - ix, len), text, len) == 0) {
						char * ret = strdup(commands.c_str() + ix);
						ix = pos;
						return ret;
					}
					ix = pos;
				} else {
					if (commands.compare(ix, std::min(pos - ix, len), text, len) == 0) {
						char * ret = strdup(commands.substr(ix, pos - ix).c_str());
						ix = pos + 1;
						return ret;
					}
					ix = pos + 1;
				}
			}
			return nullptr;
		}

		static void rl_cb(char * line) {
			if (! line) {
				this_s_->eval("exit");
				free(line);
				this_s_->farewell_newline_ = true;
				return;
			}
			int len = strlen(line);
			bool truncate = true;
			if (len > 1 && line[len - 1] == '~') {
				line[len - 1] = 0;
				truncate = false;
			}

			auto res = this_s_->eval(line);
			if (truncate && res.is_object_vector && res.str.size() > 256) {
				std::cerr << res.str.substr(0, 255) << " ...\n";
			} else {
				std::cerr << res.str << (res.str.size() ? "\n" : "");
			}
			//std::cerr << "readline: " << line << "\n";
			add_history(line);
			free(line);
		}
	
		//	std::thread thread_;
	
	
		struct eval_result {
			bool is_object_vector;
			std::string str;
		};

		object last_result_;
	
		eval_result eval(const std::string & cmd, bool internal = false) {
			if (trace_) {
				std::cerr << cmd << "\n";
			}

			object cmdo(cmd.c_str());
			try {
				object r = tcli->eval(cmdo);

				//std::cerr << "result: " << tcl_typename(r) << " " << r.get<std::string>() << "\n";
			
				if (!internal) {
					last_result_ = r;
				}
				bool isvec = false;
				if (r.is_list()) {
					interpreter I(r.get_interp(), false);
					if (r.at_ref(0, I).get_object()->typePtr) {
						isvec = true;
					}
				}

				return { isvec, r.get<std::string>() };
			} catch (std::runtime_error & e) {
				std::cerr << "while executing: " << cmd << "\n";
				return { false, e.what() };
			}
			return { false, "" };
		}

		void pollfd(int sleep) {
			if (*done_) return;
			fd_set fd_r;

			FD_ZERO(&fd_r);
			FD_SET(0, &fd_r);
			timeval tout;

			tout.tv_sec = 0; tout.tv_usec = sleep;

			boardview()->sleep_mutex_unlock();
			int r = ::select(1, &fd_r, nullptr, nullptr, &tout);
			boardview()->sleep_mutex_lock();

			if (schem_thread_joinable_) {
				std::cerr << "pdf load thread exit...\n";
				schem_open_thread_->join();
				schem_open_thread_ = nullptr;
				schem_thread_joinable_ = false;
			}
			if (r && FD_ISSET(0, &fd_r)) {
#if 0
				char buf[1024];

				fgets(buf, sizeof(buf), stdin);
				//std::cerr << "cmd: " << buf;
				auto res = eval(buf);
				if (res.is_object_vector && res.str.size() > 256) {
					std::cerr << res.str.substr(0, 255) << " ...\n";
				} else {
					std::cerr << res.str << (res.str.size() ? "\n" : "");
				}
#else
				rl_callback_read_char();
#endif
			}
			highlight_delay_poll();
		}
	};
}

typedef OBV_Tcl::TCL TCL;
