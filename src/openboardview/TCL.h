// -*- c++ -*-
#pragma once


#include <iostream>
#include <sys/time.h>
#include <fnmatch.h>

#define CPPTCL_NO_TCL_STUBS
//#define USE_TCL_STUBS
#include <tcl.h>

#include "/home/sascha/tmp/cpptcl/cpptcl/cpptcl.h"
//#include "/home/sascha/tmp/my_cpptcl/cpptcl/cpptcl/cpptcl.h"
//#include "/home/sascha/tmp/cpptcl_orig/cpptcl/cpptcl/cpptcl.h"

#include <iterator>
#include <map>
#include <memory>
#include <sstream>
#include <set>

#include "/home/sascha/tmp/cpptcl/cpptcl.cc"
//#include "/home/sascha/tmp/my_cpptcl/cpptcl/cpptcl.cc"
//#include "/home/sascha/tmp/cpptcl_orig/cpptcl/cpptcl.cc"

#include "BoardView.h"
#include <regex>


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

	//static BoardView * bv;
	Board * brd() { return b->m_board; }
	BoardView * boardview() { return b; }

	//static Tcl::interpreter * tclis;
	
	static void dup_net(Tcl_Obj * src, Tcl_Obj * dst) {
		dst->internalRep.otherValuePtr = src->internalRep.otherValuePtr;
		//		dst->refCount = src->refCount + 1;
		dst->typePtr = &net_objtype_;
		//std::cerr << "dup_net\n";
	}
	static void str_net(Tcl_Obj * o) {
		Net * n = (Net *) o->internalRep.otherValuePtr;

		//std::cerr << "str_net ";
		//std::cerr << n << " ";
		//std::cerr << "str_net " << n << " " << n->name << " " << n->name.size() << "\n";
		o->bytes = Tcl_Alloc(n->name.size() + 1);
		strncpy(o->bytes, n->name.c_str(), n->name.size());
		o->bytes[n->name.size()] = 0;
		o->length = n->name.size();
	}
	static int set_net(Tcl_Interp *, Tcl_Obj *) {
		std::cerr << "set_net not impl\n";
		return TCL_OK;
	}

	static void dup_cell(Tcl_Obj * src, Tcl_Obj * dst) {
		dst->internalRep.otherValuePtr = src->internalRep.otherValuePtr;
		//		dst->refCount = src->refCount + 1;
		dst->typePtr = &cell_objtype_;
#if 0
		std::cerr << "dup_cell " << src << " -> " << dst << " intern=" << src->internalRep.otherValuePtr << " " << src->length << " " << src->typePtr << "/" << (src->typePtr ? src->typePtr->name : "") << " {";
		for (int i = 0; i < src->length; ++i) {
			std::cerr << src->bytes[i];
		}
		std::cerr << "}\n";
#endif
	}

	static void free_cell(Tcl_Obj * o) {
		if (o) { }
		//std::cerr << "free_cell " << o << " not impl\n";
		//char * p = nullptr;
		//*p = 0;
	}
	static void str_cell(Tcl_Obj * o) {
		Component * c = (Component *) o->internalRep.otherValuePtr;

		//std::cerr << "str_cell ";
		//std::cerr << c << " ";
		//std::cerr << "str_cell " << c << " " << c->name << " " << c->name.size() << " ";
		o->bytes = Tcl_Alloc(c->name.size() + 1);
		strncpy(o->bytes, c->name.c_str(), c->name.size());
		o->bytes[c->name.size()] = 0;
		//std::cerr << (void *) o->bytes << " " << o->bytes << " ";
		o->length = c->name.size();
		//int len = 0;
		//std::cerr << Tcl_GetStringFromObj(o, &len) << "\n";
	}
	static int set_cell(Tcl_Interp *, Tcl_Obj *) {
		std::cerr << "set_cell not impl\n";
		return TCL_OK;
	}

	static void dup_pin(Tcl_Obj * src, Tcl_Obj * dst) {
		dst->internalRep.otherValuePtr = src->internalRep.otherValuePtr;
		//		dst->refCount = src->refCount + 1;
		dst->typePtr = &pin_objtype_;
		//std::cerr << "dup_pin\n";
	}
	static void str_pin(Tcl_Obj * o) {
		Pin * p = (Pin *) o->internalRep.otherValuePtr;

		//std::cerr << "str_cell ";
		//std::cerr << c << " ";
		//std::cerr << "str_cell " << c << " " << c->name << " " << c->name.size() << " ";
		o->bytes = Tcl_Alloc(p->name.size() + 1 + (p->component ? 1 + p->component->name.size() : 0));
		if (p->component) {
			int at = 0;
			strncpy(o->bytes, p->component->name.c_str(), p->component->name.size());
			at += p->component->name.size();
			o->bytes[at] = '/';
			strncpy(o->bytes + at + 1, p->name.c_str(), p->name.size());
			o->bytes[at + 1 + p->name.size()] = 0;
			o->length = at + 1 + p->name.size();
		} else {
			strncpy(o->bytes, p->name.c_str(), p->name.size());
			o->bytes[p->name.size()] = 0;
			o->length = p->name.size();
		}
			//std::cerr << (void *) o->bytes << " " << o->bytes << " ";
		//int len = 0;
		//std::cerr << Tcl_GetStringFromObj(o, &len) << "\n";
	}
	static int set_pin(Tcl_Interp *, Tcl_Obj *) {
		std::cerr << "set_pin not impl\n";
		return TCL_OK;
	}
	
	static Tcl_ObjType net_objtype_, cell_objtype_, pin_objtype_;

	struct TClass {
		TClass(int i) : v(i) { }
		int get() { return v; }
		void set(int vv) { v = vv; }
		void var(object const & o) {
			interpreter i(o.get_interp(), false);
			std::cerr << "num_args: " << o.size(i) << "\n";
			for (std::size_t j = 0; j < o.size(i); ++j) {
				std::cerr << j << ": " << o.at(j, i).get<std::string>() << "\n";
			}
		}
		int getc() const { return v; }
		int v;
	};
public :
	static Component * try_cell(Tcl_Obj * o) {
		if (o->typePtr == &cell_objtype_) {
			return (Component *) o->internalRep.otherValuePtr;
		}
		return nullptr;
	}
	static Pin * try_pin(Tcl_Obj * o) {
		if (o->typePtr == &pin_objtype_) {
			return (Pin *) o->internalRep.otherValuePtr;
		}
		return nullptr;
	}
	static Net * try_net(Tcl_Obj * o) {
		if (o->typePtr == &net_objtype_) {
			return (Net *) o->internalRep.otherValuePtr;
		}
		return nullptr;
	}

	static void hello() {
		std::cerr << "hello\n";
	}
	static void hello3(int a, int b, int c) {
		std::cerr << "hello3 " << a << " " << b << " " << c << "\n";
	}
	static void hello13(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j, int k, int l, int m) {
		std::cerr << "hello13 " << a << b << c << d << e << f << g << h << i << j << k << l << m << "\n";
	}
	static void nothing() {
	}

	
	static void hellov(object const & o) {
		interpreter i(o.get_interp(), false);


		//Tcl_Channel ch = Tcl_GetStdChannel(TCL_STDOUT);
		std::ostringstream oss;
		
		oss << "num_args: " << o.size(i) << "\n";
		for (std::size_t j = 0; j < o.size(i); ++j) {
			oss << j << ": " << o.at(j, i).get<std::string>() << "\n";
		}
		//Tcl_Write(ch, oss.str().c_str(), oss.str().size());
		//tclis->terr() << oss.str();
	}

	std::string obj_prop(object const & o) {
		std::ostringstream oss;
		oss << o.get_object() << " " << o.get_object()->typePtr << " " << (o.get_object()->typePtr ? o.get_object()->typePtr->name : "");
		return oss.str();
	}

	template <typename OT>
	static typename std::enable_if<std::is_same<OT, Net>::value || std::is_same<OT, Pin>::value || std::is_same<OT, Component>::value, object>::type makeobj(OT * n) {
		Tcl_Obj * o = Tcl_NewObj();
		o->internalRep.otherValuePtr = (void *) n;
		if (std::is_same<OT, Net>::value) {
			o->typePtr = &net_objtype_;
		} else if (std::is_same<OT, Pin>::value) {
			o->typePtr = &pin_objtype_;
		} else if (std::is_same<OT, Component>::value) {
			o->typePtr = &cell_objtype_;
		}
		Tcl_InvalidateStringRep(o);
		return object(o);
	}

	
	object get_nets(object const & argv) {
		object r;

		interpreter I(argv.get_interp(), false);
		int argc = argv.size(I);

		std::string match;
		bool use_regex = false;
		object filter_o; int filter = 0; std::string filter_ns;
		int of_i = -1;
		bool all = false;
		int not_i = -1;
		bool gnd = false;
		
		for (int ai = 0; ai < argc; ++ai) {
			std::string arg = argv.at_ref(ai, I).get<std::string>();
			if (arg[0] == '-') {
				if (argp(arg, "of")) {
					of_i = ai + 1;
					++ai;
				} else if (argp(arg, "filter")) {
					filter = filter_ns_ix++ % 1000;
					filter_ns = namespace_str(filter);
					namespace_open(I, filter_ns);
					filter_o = object(std::string("namespace eval ") + filter_ns + " { expr { " + argv.at_ref(ai + 1, I).get<std::string>() + " } }");
					
					++ai;
				} else if (argp(arg, "re")) {
					use_regex = true;
				} else if (argp(arg, "all")) {
					all = true;
				} else if (argp(arg, "not")) {
					not_i = ai + 1;
					++ai;
				} else if (argp(arg, "gnd")) {
					gnd = true;
				}
			} else {
				match = argv.at_ref(ai, I).get<std::string>();
			}
		}
		std::regex re;
		if (use_regex) re = match;

		std::set<Net *> dup;

		std::set<Net *> not_set;

		if (not_i != -1) {
			object notof = argv.at_ref(not_i, I);
			std::size_t llen = notof.is_list() ? notof.size(I) : 1;

			for (std::size_t i = 0; i < llen; ++i) {
				Tcl_Obj * to = notof.is_list() ? notof.at_ref(i, I).get_object() : notof.get_object();
				Net * n = try_net(to);
				if (n) {
					not_set.insert(n);
				}
			}
		}
		
		
		auto try_append = [&](Net * n, Component * of = nullptr) {
			if ((gnd || !n->is_ground) && matches(match, re, n->name, use_regex) && filter_match(I, filter_ns, filter_o, n, of) && (all || dup.find(n) == dup.end()) && not_set.find(n) == not_set.end()) {
				r.append(makeobj(n));
				if (!all) dup.insert(n);
			}
			return true;
		};

		if (of_i != -1) {
			object of(argv.at_ref(of_i, I));
			std::size_t len = of.is_list() ? of.size(I) : 1;
				
			for (std::size_t j = 0; j < len; ++j) {
				Tcl_Obj * to = of.is_list() ? of.at_ref(j, I).get_object() : of.get_object();
				
				Pin * p; Component * c;
				if ((p = try_pin(to))) {
					if (p->net && try_append(p->net)) {
					}
				} else if ((c = try_cell(to))) {
					for (std::size_t pi = 0; pi < c->pins.size(); ++pi) {
						if (c->pins[pi]->net && try_append(c->pins[pi]->net, c)) {
						}
					}
				}
			}
		} else {
			for (std::size_t i = 0; i < brd()->Nets().size(); ++i) {
				Net * n = &(*brd()->Nets()[i]);
				
				if (try_append(n)) {
				}
			}
		}
		return r;
	}

	static bool argp(const std::string & arg, const char * str) {
		return arg.compare(1, arg.size() - 1, str) == 0;
	}		

	static bool matches(const std::string & glob, const std::regex & re, const std::string & s, bool use_re) {
		if (! glob.empty()) {
			if (use_re) {
				return std::regex_search(s, re);
			} else {
				return fnmatch(glob.c_str(), s.c_str(), 0) == 0;
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
	
	object get_pins(object const & argv) {
		object r;
		interpreter I(argv.get_interp(), false);
		int argc = argv.size(I);

		bool use_regex = false;
		std::string match;
		object filter_o;
		int filter = 0;
		std::string filter_ns;
		int of_i = -1;
		bool all = false;
		bool parallel = false;
		
		for (int ai = 0; ai < argc; ++ai) {
			std::string arg = argv.at_ref(ai, I).get<std::string>();
			if (arg[0] == '-') {
				if (argp(arg, "of")) {
					of_i = ai + 1;
					++ai;
				} else if (argp(arg, "re")) {
					use_regex = true;
				} else if (argp(arg, "filter")) {
					filter = filter_ns_ix++ % 1000;
					filter_ns = namespace_str(filter);
					namespace_open(I, filter_ns);
					filter_o = object(std::string("namespace eval ") + filter_ns + " { expr { " + argv.at_ref(ai + 1, I).get<std::string>() + " } }");
					++ai;
				} else if (argp(arg, "all")) {
					all = true;
				} else if (argp(arg, "parallel")) {
					parallel = true;
				}
			} else {
				match = argv.at_ref(ai, I).get<std::string>();
			}
		}
		std::regex re;
		if (use_regex) re = match;
		
		std::set<Pin *> dup;
		bool match_has_slash = true; //std::find(match.begin(), match.end(), '/') != match.end();
		
		auto try_append = [&](Pin * c) {
			std::string m(c->name);
			if (match_has_slash) {
				m = c->component->name + "/" + c->name;
			}
			//std::cerr << m << " " << match << "\n";
			if (matches(match, re, m, use_regex) && filter_match(I, filter_ns, filter_o, c) && (all || dup.find(c) == dup.end())) {
				r.append(makeobj(c));
				if (!all) dup.insert(c);
			}
			return true;
		};
		
		if (of_i != -1) {
			object of = argv.at_ref(of_i, I);
			int llen = of.is_list() ? of.size(I) : 1;
			
			for (int j = 0; j < llen; ++j) {
				Tcl_Obj * to = of.is_list() ? of.at_ref(j, I).get_object() : of.get_object();
				Net * n; Component * c; Pin * p;
				
				if ((n = try_net(to))) {
					for (auto && pi : n->pins) {
						//if (!is_dummy(c) && matches(match, re, c->name, use_regex) && filter_match(I, filter, filter_o, c) && dup.find(c) == dup.end()) {
						if (try_append(pi.get())) {
						}
					}
				} else if ((c = try_cell(to))) {
					//if (! is_dummy(c) && matches(match, re, c->name, use_regex) && filter_match(I, filter, filter_o, c) && dup.find(c) == dup.end()) {
					for (auto && pi : c->pins) {
						if (try_append(pi.get())) {
						}
					}
				} else if ((p = try_pin(to))) {
					if (try_append(p)) {
					}
					if (parallel && p->component.get()) {
						for (auto && pi : p->component->pins) {
							if (p->net == pi->net) {
								if (try_append(pi.get())) {
								}
							}
						}
					}
				}
			}
			
		} else {
			for (std::size_t i = 0; i < brd()->Pins().size(); ++i) {
				if (try_append(&(*brd()->Pins()[i]))) {
				}
			}
		}
		return r;
	}

	void filter_install(interpreter & I, std::string const & ns, Component * c) {
		for (int i = 0; cell_props_[i]; ++i) {
			I.setVar(ns + "::" + cell_props_[i], cell_get_prop(cell_props_[i], c));
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
	static constexpr char const * pin_props_[] = { "type", "ispad", nullptr };
	static constexpr char const * net_props_[] = { "isgnd", nullptr };

	template <typename T>
	using is_CNP = std::integral_constant<bool, std::is_same<T, Pin>::value || std::is_same<T, Net>::value || std::is_same<T, Component>::value>;

	
	template <typename OT>
	typename std::enable_if<is_CNP<OT>::value && is_CNP<typename of_pins_t<OT>::type>::value, bool>::type filter_match(interpreter & I, std::string const & ns, object const & eval, OT * p, typename of_pins_t<OT>::type * of = nullptr) {
		if (ns.empty()) return true;

		filter_install(I, ns, p);
		I.setVar(ns + "::bundle", object(of_pins_t<OT>::of_pins(p, of)));

		try {
			return I.eval(eval);
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
	
	object get_cells(object const & argv) {
		object r;
		//static char cell_filter_ix = 0x41;
		interpreter I(argv.get_interp(), false);
		int filter = 0;
		std::string filter_ns;
		object filter_o;
		std::string match;
		bool use_regex = false;
		int of_i = -1;
		bool all = false;
		int not_i = -1;
		
		for (std::size_t ai = 0; ai < argv.size(I); ++ai) {
			std::string arg = argv.at_ref(ai, I).get<std::string>();
			if (arg[0] == '-') {
				if (argp(arg, "filter")) {
					filter = filter_ns_ix++ % 1000;
					filter_ns = namespace_str(filter);
					namespace_open(I, filter_ns);
					filter_o = object(std::string("namespace eval ") + namespace_str(filter) + " { expr { " + argv.at_ref(ai + 1, I).get<std::string>() + " } }");
					++ai;
				} else if (argp(arg, "re")) {
					use_regex = true;
				} else if (argp(arg, "of")) {
					of_i = ai + 1;
					++ai;
				} else if (argp(arg, "all")) {
					all = true;
				} else if (argp(arg, "not")) {
					not_i = ai + 1;
					++ai;
				}
			} else {
				match = argv.at_ref(ai, I).get<std::string>();
			}
		}
		std::regex re;

		if (use_regex) re = match;
		
		std::set<Component *> dup;

		std::set<Component *> not_set;
		if (not_i != -1) {
			object notof = argv.at_ref(not_i, I);
			std::size_t llen = notof.is_list() ? notof.size(I) : 1;

			for (std::size_t i = 0; i < llen; ++i) {
				Tcl_Obj * to = notof.is_list() ? notof.at_ref(i, I).get_object() : notof.get_object();
				//std::cerr << "notloop " << i << " " << llen << " " << tcl_typename(to) << "\n";
				Component * c = try_cell(to);
				if (c) {
					//std::cerr << "NOT(" << c->name << ")" << notof.is_list() << " " << notof.size(I) << "\n";
					not_set.insert(c);
				}
			}
		}

		auto try_append = [&](Component * c) {
			if (!is_dummy(c) && matches(match, re, c->name, use_regex) && filter_match(I, filter_ns, filter_o, c) && (all || dup.find(c) == dup.end()) && not_set.find(c) == not_set.end()) {
				r.append(makeobj(c));
				if (!all) dup.insert(c);
			}
			return true;
		};
			
		if (of_i != -1) {
			object of = argv.at_ref(of_i, I);
			int llen = of.is_list() ? of.size(I) : 1;

			for (int j = 0; j < llen; ++j) {
				Tcl_Obj * to = of.is_list() ? of.at_ref(j, I).get_object() : of.get_object();
				Net * n; Pin * p; Component * cc;

				if ((n = try_net(to))) {
					for (auto && pi : n->pins) {
						//if (!is_dummy(c) && matches(match, re, c->name, use_regex) && filter_match(I, filter, filter_o, c) && dup.find(c) == dup.end()) {
						if (try_append(pi->component.get())) {
						}
					}
				} else if ((p = try_pin(to))) {
					//if (! is_dummy(c) && matches(match, re, c->name, use_regex) && filter_match(I, filter, filter_o, c) && dup.find(c) == dup.end()) {
					if (try_append(p->component.get())) {
					}
				} else if ((cc = try_cell(to))) {
					if (try_append(cc)) {
					}
				}
			}
		} else {
			for (std::size_t i = 0; i < brd()->Components().size(); ++i) {
				//if (!is_dummy(c) && matches(match, re, c->name, use_regex) && filter_match(I, filter, filter_o, c) && dup.find(c) == dup.end()) {
				if (try_append(&(*brd()->Components()[i]))) {
				}
			}
		}
		//std::cerr << "make cells " << r.get_object() << "\n";
		return r;
	}

	object selection(object const & argv) {
		object r;

		if (argv.is_list()) { }
		
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
	
	void highlight(object const & argv) {
		interpreter I(argv.get_interp(), false);
		int argc = argv.is_list() ? argv.size(I) : 1;
		bool un = false;
		bool all = false;
		uint32_t color = 0xffaaaaaa;
		int optind = 0;

		for (int ai = 0; ai < argc; ++ai) {
			const std::string arg = argv.is_list() ? argv.at_ref(ai, I).get<std::string>() : argv.get<std::string>();

			if (arg[0] == '-') {
				if (argp(arg, "un")) {
					un = true;
				} else if (argp(arg, "all")) {
					all = true;
				} else if (argp(arg, "color")) {
					std::string col = argv.at_ref(ai + 1, I).get<std::string>();
					if (col[0] == '#') {
						std::istringstream iss(col);
						char dummy;
						
						iss >> dummy >> std::hex >> color;
						color |= 0xff000000;
					} else {
						color = 0xffaaaaaa;
					}
					color &= 0x00ffffff;
					color |= 0x99000000;
					++ai;
				}
			} else {
				optind = ai;
				break;
			}
		}

		if (! un) all = false;

		if (un && all) {
			for (auto && ci : brd()->Components()) {
				ci->shade_color_ = 0;
			}
		} else {
			for (int li = optind; li < argc; ++li) {
				object o = argv.is_list() ? argv.at_ref(li, I) : argv;
				std::size_t llen = o.is_list() ? o.size(I) : 1;
				
				for (std::size_t lli = 0; lli < llen; ++lli) {
					Tcl_Obj * to = o.is_list() ? o.at_ref(lli, I).get_object() : o.get_object();
					
					bool found = false;
					if (to->typePtr == &pin_objtype_) {
						Pin * p = (Pin *) to->internalRep.otherValuePtr;
						for (auto && pi : brd()->Pins()) {
							if (pi.get() == p) {
								boardview()->m_pinHighlighted.push_back(pi);
								found = true;
								break;
							}
						}
					} else if (to->typePtr == &cell_objtype_) {
						Component * c = (Component *) to->internalRep.otherValuePtr;
						if (un) {
							c->shade_color_ = 0;
						} else {
							if (highlight_delay_enable_) {
								highlight_delay_.push_back(highlight_delay{c, color});
							} else {
								c->shade_color_ = color; //0xffff0000;
							}
						}
#if 0
						for (auto && ci : brd()->Components()) {
							if (ci.get() == c) {
								boardview()->m_partHighlighted.push_back(ci);
								found = true;
								break;
							}
						}
#endif
					} else {
						found = true;
					}
					
					if (! found) {
					}
				}
			}
		}
	}

	static bool icmp(const char * p1, const char * p2) {
		return strcasecmp(p1, p2) == 0;
	}
	static bool icmp(const std::string & str, const char * str2) {
		return icmp(str.c_str(), str2);
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
		return object(false);
	}
	static object pin_get_prop(const char * prop, Pin * p) {
		if (p) { }
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
		return object(false);
	}
	object net_get_prop(const char * prop, Net * n) {
		if (icmp(prop, "isgnd")) {
			return object(n->is_ground);
		}
		return object(false);
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
	
	object get_prop(object const & argv) {
		object r;
		
		Tcl::interpreter I(argv.get_interp(), false);

		if (r.size(I)) { }
		
		int argc = argv.size(I);
		
		if (argc < 2) {
			return object("0");
		}
		//std::cerr << "size " << argv.at(1, I).size() << "\n";
		std::string prop = argv.at_ref(0, I).get<std::string>();

		object of = argv.at_ref(1, I);
		std::size_t llen = of.is_list() ? of.size(I) : 1;
		
		for (std::size_t ai = 0; ai < llen; ++ai) {
			Tcl_Obj * to = of.is_list() ? of.at_ref(ai, I).get_object() : of.get_object();
			
			//std::cerr << of.is_list() << "to=" << to << " internal=" << to->internalRep.otherValuePtr << " objtype=" << tcl_typename(to) << "\n";
			Component * c; Net * n; Pin * p;
			if ((c = try_cell(to))) {
				//std::cerr << "cell: " << c << " " << c->name << "\n";
				//r.append(object(cell_get_prop(prop.c_str(), c)));
				r.append(cell_get_prop(prop.c_str(), c));
			} else if ((n = try_net(to))) {
				//std::cerr << "net: " << n << " " << n->name << "\n";
				
				r.append(net_get_prop(prop.c_str(), n));
			} else if ((p = try_pin(to))) {
				r.append(object(pin_get_prop(prop.c_str(), p)));
			}
		}
		//std::cerr << "get_prop(" << prop << ") res: " << r.size(I) << " " << tcl_typename(r) << " [0]=" << tcl_typename(r.at_ref(0, I)) << " " << r.at_ref(0, I) << "\n";
		//if (r.size(I) == 1) { return r.at(0, I); }
		return r;
	}

	void trace(bool v) {
		trace_ = v;
	}
	bool trace_ = false;
	
	void report_prop(object argv) {
		std::ostringstream oss;
		interpreter I(argv.get_interp(), false);
		
		int llen = argv.is_list() ? argv.size(I) : 1;
		for (int ai = 0; ai < llen; ++ai) {
			Tcl_Obj * to = argv.is_list() ? argv.at_ref(ai, I).get_object() : argv.get_object();
			
			Net * n; Pin *p; Component * c;
			
			if ((n = try_net(to))) {
				for (int i = 0; net_props_[i]; ++i) {
					oss << net_props_[i] << ":" << net_get_prop(net_props_[i], n).get<std::string>() << "\n";
				}
#if 0
				Net * n = (Net *) o->internalRep.otherValuePtr;
				oss << "obj: " << o << "\n";
				oss << "#pins: " << n->pins.size() << "\n";
				oss << "isgnd: " << n->is_ground << "\n";
				oss << "number: " << n->number << "\n";
#endif
			} else if ((c = try_cell(to))) {
				for (int i = 0; cell_props_[i]; ++i) {
					oss << cell_props_[i] << ": " << cell_get_prop(cell_props_[i], c) << "\n";
				}
#if 0
				Component * c = (Component *) o->internalRep.otherValuePtr;
				oss << "addr: " << c << "\n";
				oss << "obj: " << o << "\n";
				oss << "#pins: " << c->pins.size() << "\n";
#endif
			} else if ((p = try_pin(to))) {
				for (int i = 0; pin_props_[i]; ++i) {
					oss << pin_props_[i] << ": " << pin_get_prop(pin_props_[i], p) << "\n";
				}
			}
			std::cerr << oss.str();
		}
	}

	object last_result() {
		return last_result_;
	}
	
	void hello_m(int a, int b, int c) {
		std::cerr << "hello_m " << a << " " << b << " " << c << " " << this << "\n";
	}
	
	TCL(BoardView * bvv) : b(bvv) {
		//bv = bvv;
		tcl = Tcl_CreateInterp();

		net_objtype_ = { "net", nullptr, dup_net, str_net, set_net };
		cell_objtype_ = { "cell", free_cell, dup_cell, str_cell, set_cell	};
		pin_objtype_ = { "pin",	nullptr, dup_pin, str_pin, set_pin };
		
		Tcl_RegisterObjType(&net_objtype_);
		Tcl_RegisterObjType(&cell_objtype_);
		Tcl_RegisterObjType(&pin_objtype_);
		
		tcli = new interpreter(tcl, true);
		//tclis = tcli;
		tcli->def("hello3", hello3);
		tcli->def("hellov", hellov, variadic());
		//tcli->def("hello13", hello13);
		tcli->def("nothing", nothing);

		typedef TCL this_t;

		tcli->def(this)
			.def("get_nets", &this_t::get_nets, variadic())
			.def("get_cells", &this_t::get_cells, variadic())
			.def("get_pins", &this_t::get_pins, variadic())
			.def("report_prop", &this_t::report_prop)
			.def("highlight", &this_t::highlight, variadic())
			.def("selection", &this_t::selection, variadic())
			.def("get_prop", &this_t::get_prop, variadic())
			.def("trace", &this_t::trace)
			.def("last_result", &this_t::last_result);
		
		int * v = new int;
		int & lvar = *v;
		tcli->def("lambda", [&lvar] { std::cerr << "lambda " << lvar << "\n"; });
		tcli->def("lambdai", [&lvar] { ++lvar; });

		tcli->def("hello_m", &TCL::hello_m, this);
		
		tcli->class_<TClass>("TClass", init<int>()).def("get", &TClass::get).def("set", &TClass::set).def("var", &TClass::var, variadic()).def("getc", &TClass::getc);
		++lvar;

		tcli->eval("source library.tcl");
	}

	struct eval_result {
		bool is_object_vector;
		std::string str;
	};

	object last_result_;
	
	eval_result eval(const std::string & cmd) {
		if (trace_) {
			std::cerr << cmd << "\n";
		}

		object cmdo(cmd.c_str());
		try {
			object r = tcli->eval(cmdo);

			//std::cerr << "result: " << tcl_typename(r) << " " << r.get<std::string>() << "\n";
			
			last_result_ = r;
			bool isvec = false;
			if (r.is_list()) {
				interpreter I(r.get_interp(), false);
				if (r.at_ref(0, I).get_object()->typePtr) {
					isvec = true;
				}
			}

			return { isvec, r.get<std::string>() };
		} catch (std::runtime_error & e) {
			std::cerr << cmd << "\n";
			return { false, e.what() };
		}
		return { false, "" };
	}

	void pollfd() {
		fd_set fd_r;

		FD_ZERO(&fd_r);
		FD_SET(0, &fd_r);
		timeval tout;

		tout.tv_sec = tout.tv_usec = 0;

		int r = select(1, &fd_r, nullptr, nullptr, &tout);
		if (r && FD_ISSET(0, &fd_r)) {
			char buf[1024];

			fgets(buf, sizeof(buf), stdin);
			//std::cerr << "cmd: " << buf;
			auto res = eval(buf);
			if (res.is_object_vector && res.str.size() > 256) {
				std::cerr << res.str.substr(0, 255) << " ...\n";
			} else {
				std::cerr << res.str << (res.str.size() ? "\n" : "");
			}
		}
		highlight_delay_poll();
	}
};

Tcl_ObjType TCL::net_objtype_, TCL::cell_objtype_, TCL::pin_objtype_;
decltype(TCL::cell_props_) TCL::cell_props_;
decltype(TCL::pin_props_) TCL::pin_props_;
decltype(TCL::net_props_) TCL::net_props_;
