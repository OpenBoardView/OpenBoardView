// -*- c++ -*-
#pragma once

#define HAVE_READLINE
#define OBV_USE_POPPLER
//#include "TCLDUMMY.h"


#include <iostream>
#include <sys/time.h>
#include <fnmatch.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

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
#ifdef HAVE_READLINE
#define _FUNCTION_DEF
#include <readline/readline.h>
#include <readline/history.h>
#undef _FUNCTION_DEF
#endif
#include <SDL.h>

#include "BoardView.h"
#include "imgui_operators.h"
#include <regex>
#include <atomic>

#ifdef OBV_USE_POPPLER

//#error Enabling pdf rendering with poppler changes the license of openboardview. Uncomment this line to continue with this derivation of openboardview under GPL license

#include <poppler/PDFDocFactory.h>
#include <poppler/TextOutputDev.h>
#include <poppler/CairoOutputDev.h>
#include <poppler/GlobalParams.h>

#include <poppler/glib/poppler.h>
#include <poppler/glib/poppler-document.h>
#include <boost/iterator/function_output_iterator.hpp>
#endif


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

	namespace detail {
		template <typename TT, typename ...Ts>
		struct contains {
			static const bool value = false;
		};
		template <typename TT, typename ...Ts>
		struct contains<TT, TT, Ts...> {
			static const bool value = true;
		};
		template <typename TT, typename T, typename ...Ts>
		struct contains<TT, T, Ts...> {
			static const bool value = contains<TT, Ts...>::value;
		};
	}	
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
		struct bbox {
			ImVec2 min, max;
		};
		struct pdf_txt_bbox {
			bbox box;
			std::string word;
			bool hoverable = false;
			bool clickable = false;
			
			Component * cell = nullptr;
			Net * net = nullptr;
			int page;
			uint32_t color = 0;
		};
		
		template <typename OT, typename DEL>
		//typename std::enable_if<std::is_same<OT, Net>::value || std::is_same<OT, Pin>::value || std::is_same<OT, Component>::value || std::is_same<OT, BRDBoard>::value, object>::type makeobj(OT * n) {
		typename std::enable_if<detail::contains<OT, Net, Pin, Component, BRDBoard, pdf_txt_bbox>::value, object>::type makeobj(OT * n, bool owning = false, DEL delfn = []{}) {
			Tcl_Obj * o = Tcl_NewObj();
			//o->internalRep.otherValuePtr = (void *) n;
			o->internalRep.twoPtrValue.ptr1 = (void *) n;
			if (owning) {
				if constexpr (std::is_same<DEL, void *>::value) {
					if (delfn) { }
					o->internalRep.twoPtrValue.ptr2 = new interpreter::shared_ptr_deleter<OT>(n); //new interpreter::dyn_deleter<OT>(); //nullptr;
				} else {
					o->internalRep.twoPtrValue.ptr2 = new interpreter::shared_ptr_deleter<OT>(n, delfn); //new interpreter::dyn_deleter<OT>(); //nullptr;
				}
				//o->internalRep.twoPtrValue.ptr2 = new interpreter::dyn_deleter<OT>(); //nullptr;
			} else {
				o->internalRep.twoPtrValue.ptr2 = nullptr;
			}
			
			auto it = tcli->obj_type_by_cppname_.find(std::type_index(typeid(OT)));
			bool ok = it != tcli->obj_type_by_cppname_.end();
			if (ok) {
				o->typePtr = it->second;
			} else {
				throw tcl_error("type lookup failed");
			}
			Tcl_InvalidateStringRep(o);
			return object(o, true);
		}

		template <typename OT>
		typename std::enable_if<detail::contains<OT, Net, Pin, Component, BRDBoard, pdf_txt_bbox>::value, object>::type makeobj(OT * n, bool owning = false) {
			return makeobj(n, owning, (void *) 0 );
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
					//current_draw_file_ = obv_shared_ptr<BRDFile>(boardfiles_[i], [] (auto) { });
					current_draw_file_ = obv_shared_ptr<BRDFile>(boardfiles_[i]);
					break;
				}
			}
			//current_draw_board_ = obv_shared_ptr<BRDBoard>(b, [] (auto) { });
			current_draw_board_ = obv_shared_ptr<BRDBoard>(b);//, [] (auto) { });
			boardview()->SetFile(current_draw_file_, current_draw_board_);
		}

		obv_shared_ptr<BRDFile> current_draw_file_;
		obv_shared_ptr<BRDBoard> current_draw_board_;

		ImVec2 center(bbox const & b) {
			return b.min + (b.max - b.min) / 2.0f;
		}
		float distance(bbox const & a, bbox const & b) {
			ImVec2 ca = center(a), cb = center(b);
			ImVec2 d = ca - cb;
			return sqrt(abs(d.x * d.x) + abs(d.y * d.y));
		}
		
#ifdef OBV_USE_POPPLER
		static constexpr char const * distance_opt = "norm border";
		float distance_fun(getopt<bool> const & norm, getopt<bool> border, any<Component, Pin, pdf_txt_bbox> const & a, any<Component, Pin, pdf_txt_bbox> const & b);
		static constexpr char const * angle_opt = "ortho";
		float angle(getopt<bool> const & ortho, pdf_txt_bbox * a, pdf_txt_bbox * b);
		object enclosed_box(list<pdf_txt_bbox> const & words);
#endif
		
		bool intersect(bbox const & a, bbox const & b) {
			if (in(a, b.min) || in(a, b.max) || in(a, ImVec2(b.min.x, b.max.y)) || in(a, ImVec2(b.max.x, b.min.y))) {
				return true;
			}
			return false;
		}
		
		static constexpr const char * get_schem_words_opt = "page filter re ref sort not of exact color bbox radius";
		object get_schem_words(getopt<int> const & page, getopt<std::string> const & filter_arg, getopt<bool> const & use_regex, getopt<pdf_txt_bbox *> const & ref, getopt<std::string> const & sort, getopt<list<pdf_txt_bbox> > const & not_, getopt<any<list<Component> , list<Pin>, list<Net> > > const & of, getopt<std::string> const & exact, getopt<std::string> const & color, getopt<list<pdf_txt_bbox> > const & bbox, getopt<float> const & radius, variadic<std::string> const & match);
		static constexpr const char * get_nets_opt = "re all not gnd of filter";
		object get_nets(getopt<bool> const & use_regex, getopt<bool> const & all, getopt<list<Net> > const & not_, getopt<bool> const & gnd, getopt<any<list<Component>, list<Pin>, list<Net>, list<BRDBoard> > > const & of, getopt<std::string> const & filter_arg, variadic<std::string> const & match);
		static constexpr const char * get_pins_opt = "re all parallel filter of brief";
		object get_pins(getopt<bool> const & use_regex, getopt<bool> const & all, getopt<bool> const & parallel, getopt<std::string> const & filter_arg, getopt<any<list<Component>, list<Net>, list<Pin>, list<BRDBoard> > > const & of, getopt<bool> const & brief, variadic<std::string> const & match);
		static constexpr const char * highlight_opt = "un all color intensity";
		void highlight(getopt<bool> const & un, getopt<bool> const & all, getopt<std::string> const & color, getopt<float> const & inten, opt<list<any<Component, Pin, pdf_txt_bbox> > > const & obj);
		const char * get_cells_opt = "filter re of all top bottom not";
		object get_cells(getopt<std::string> const & filter_arg, getopt<bool> const & use_regex, getopt<any<list<Component>, list<Net>, list<Pin>, list<pdf_txt_bbox> > > const & of, getopt<bool> const & all,
						 getopt<bool> const & top,
						 getopt<bool> const & bottom,
						 getopt<list<Component> > const & not_, variadic<std::string> const & match);

		static bool matches(const variadic<std::string> & glob, const std::regex & re, const std::string & s, bool use_re);

		int filter_ns_ix = 1;
		int sort_ns_ix = 1;
		
		std::string namespace_str(int ix) {
			std::ostringstream oss;
			oss << "_obv_filter_" << ix;
			return oss.str();
		}
		std::string sort_namespace_str(int ix) {
			return std::string("_obv_sort_") + std::to_string(ix);
		}

		void namespace_open(interpreter & I, std::string const & ns) {
			try {
				I.eval(std::string("namespace eval ") + ns + " { set _obv_nsopen 1 }");
			} catch (std::exception & e) {
				std::cerr << e.what() << "\n";
			}
		}

		void filter_install(interpreter & I, std::string const & ns, Component * c, const char * prefix = nullptr) {
			for (int i = 0; cell_props_[i]; ++i) {
				I.setVar(ns + "::" + (prefix ? prefix : "") + cell_props_[i], cell_get_prop(cell_props_[i], c));
			}
			if (c->tcl_priv) {
				be_priv * pr = (be_priv *) c->tcl_priv;
				for (auto && i : pr->properties) {
					I.setVar(ns + "::" + i.first, i.second);
				}
			}
		}
		void filter_install(interpreter & I, std::string const & ns, Net * n, const char * prefix = nullptr) {
			for (int i = 0; net_props_[i]; ++i) {
				I.setVar(ns + "::" + (prefix ? prefix : "") + net_props_[i], net_get_prop(net_props_[i], n));
			}		
		}
		void filter_install(interpreter & I, std::string const & ns, Pin * p, const char * prefix = nullptr) {
			for (int i = 0; pin_props_[i]; ++i) {
				I.setVar(ns + "::" + (prefix ? prefix : "") + pin_props_[i], pin_get_prop(pin_props_[i], p));
			}
		}
		void filter_install(interpreter & I, std::string const & ns, pdf_txt_bbox * p, const char * prefix = nullptr) {
			for (int i = 0; pdf_word_props_[i]; ++i) {
				I.setVar(ns + "::" + (prefix ? prefix : "") + pdf_word_props_[i], schem_word_get_prop(pdf_word_props_[i], p));
			}
		}

		struct cmpeq_t {
			bool operator()(const char * a, const char * b) {
				return strcasecmp(a, b) == 0;
			}
		};
		struct record_strings_t : public std::vector<std::string> {
			bool operator()(const char * a, const char * b) {
				const char * c = a ? a : b;
				if (c) {
					push_back(c);
				}
				return false;
			}
		};
		
		template <typename CMP=cmpeq_t>
		static object schem_word_get_prop(const char * prop, pdf_txt_bbox * p, CMP icmp = cmpeq_t());
		template <typename CMP=cmpeq_t>
		static object cell_get_prop(const char * prop, Component * c, CMP icmp = cmpeq_t());
		template <typename CMP=cmpeq_t>
		static object pin_get_prop(const char * prop, Pin * p, CMP icmp = cmpeq_t());
		template <typename CMP=cmpeq_t>
		static object net_get_prop(const char * prop, Net * n, CMP icmp = cmpeq_t());

		static char const ** cell_props_, ** pin_props_, ** net_props_, ** pdf_word_props_;

		template <typename Fn>
		static char const ** enumerate_props_impl(Fn, cmpeq_t &) {
			return nullptr;
		}
		template <typename Fn>
		static char const ** enumerate_props_impl(Fn fn, record_strings_t & rec) {
			fn(nullptr, nullptr, rec);
			char const ** p = new char const *[rec.size() + 1];
			for (std::size_t i = 0; i < rec.size(); ++i) {
				p[i] = strdup(rec[i].c_str());
			}
			p[rec.size()] = nullptr;
			rec.clear();
			return p;
		}
		static bool static_initialized_;
		static void static_initialize();
		
		template <typename T>
		using is_CNP = detail::contains<T, Pin, Net, Component, pdf_txt_bbox>; //std::integral_constant<bool, std::is_same<T, Pin>::value || std::is_same<T, Net>::value || std::is_same<T, Component>::value>;

		template <typename OT>
		bool sort_less(interpreter & I, std::string const & ns, object const & eval, OT * a, OT * b, OT * r) {
			filter_install(I, ns, a, "a_");
			filter_install(I, ns, b, "b_");
			filter_install(I, ns, r, "r_");
			object ao = I.makeobj(a), bo = I.makeobj(b);

			I.setVar(ns + "::a", ao);
			I.setVar(ns + "::b", bo);

			object ro;
			if (r) {
				ro = I.makeobj(r);
				I.setVar(ns + "::r", ro);
			}
			return I.eval(eval);
		}

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

		object selection();

		struct highlight_delay {
			Component * c;
			uint32_t shade;
		};
		std::deque<highlight_delay> highlight_delay_;
		static const bool highlight_delay_enable_ = false;
		uint64_t highlight_delay_time_ = 0;

		uint32_t color_parse(std::string const & c) {
			std::istringstream iss(c);
			uint32_t color_v = 0;
			char dummy;
			
			iss >> dummy >> std::hex >> color_v;
			return color_v;
		}

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
	
		static object extra_properties_get(const char * prop, be_priv * pr) {
			if (pr) {
				auto it = pr->properties.find(prop);
				if (it != pr->properties.end()) {
					return object(it->second);
				}
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

		typedef any<Component, Net, Pin, Board, pdf_txt_bbox> any_obv_type;

		void cell_set_prop(Component * c, std::string const & name, std::string const & value) {
			if (! c->tcl_priv) { c->tcl_priv = (void *) new cell_priv(); }
			be_priv * pr = (be_priv *) c->tcl_priv;
			pr->properties[name] = value;
		}

		void set_prop(std::string const & prop, std::string const & value, list<any<Component, Net, Pin> > const & obj) {
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
						});

				pr->properties[prop] = value;
			}
		}
		
		object get_prop(std::string const & prop, list<any_obv_type> const & obj);
		
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

		void component_select_event() {
			try {
				tcli->eval(std::string("cell_select_event"));
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

		void select(list<any_obv_type> obj);

		std::thread * schem_open_thread_ = nullptr;
		std::atomic<bool> schem_thread_joinable_ = false;

		bool in(bbox const & b, ImVec2 v) {
			return v > b.min && v < b.max;
		}
#ifdef OBV_USE_POPPLER
		PDFDoc * pdf_ = nullptr;
		std::string pdf_filename_;
		PopplerDocument * pdoc_ = nullptr;

		Image * pdf_img_ = nullptr;
		unsigned char * pdf_img_buf_ = nullptr;


		
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
			}				

			ImVec2 bdim;     // derived
			float  aspect;   // derived
			ImVec2 wb_ratio; // derived
			int    page;
			bool   valid = false;
			bool   reuse = false;
			bool   sticky = false;
			bool   ignore_one_mouse_release = false;
			bool   dragging = false;
			bool   mouse_released = false;
		};
		
		ImVec2 pdf2board(pdf_window const & w, Image * pdf, ImVec2 c, bool flip) {
			ImVec2 t = pdf->CoordToScreen(c / w.wdim);
			if (!flip) {
				//c.y = w.wdim.y - c.y;
				t.y = 1.0f - t.y;
			}
			
			return w.box.min + t * w.bdim;
			ImVec2 pr = w.wdim / c;
			return w.box.min + pr * w.bdim;
		}
		ImVec2 board2pdf(pdf_window const & w, Image * pdf, ImVec2 c, bool flip) {
			c = c - w.box.min;
			if (!flip) {
				c.y = w.bdim.y - c.y;
			}

			return pdf->ScreenToCoord((c) / w.bdim) * w.wdim;// * w.wb_ratio;
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


		typedef std::vector<pdf_txt_bbox> pdf_words_value_t;
		std::map<int, pdf_words_value_t> pdf_words_;

		std::optional<obv_shared_ptr<Component> > lookup(Component * c) {
			if (c) {
				for (auto && ci : boardview()->m_board->Components()) {
					if (ci.get() == c) return ci;
				}
			}
			return { };
		}

		bool grab_mouse_hover() {
			ImVec2 ppos = ImGui::GetMousePos();
			ImVec2 pos = pdf_win_.sticky ? ppos : boardview()->ScreenToCoord(ppos);
			
			if (pos > pdf_win_.box.min && pos < pdf_win_.box.max) {
				return true;
			}
			return false;
		}
		
		void imgui_draw_in(ImDrawList * draw, ImVec2 min, ImVec2 max);
		ImVec2 yflip(ImVec2 const & v) {
			return { v.x, boardview()->m_current_side || pdf_win_.sticky ? (pdf_win_.box.min + pdf_win_.bdim - (v - pdf_win_.box.min)).y : v.y };
		}

		int handle_mouse_drag(ImVec2 const & ppos, ImVec2 const & drag, int token);

		bool handle_mouse_click(ImVec2 const & spos) {
			if (pdf_win_.sticky ? in(pdf_win_.box, spos) : in(pdf_win_.box, boardview()->ScreenToCoord(spos))) {
				if (pdf_win_.dragging || boardview()->m_draggingLastFrame) {
					pdf_win_.ignore_one_mouse_release = true;
				} else {
					pdf_win_.mouse_released = true;
				}
				pdf_win_.dragging = false;
				return true;
			}
			return false;
		}
		
		void imgui_draw(ImDrawList * draw) {
			if (pdf_img_) {
				//imgui_draw_in(draw, pdf_min_, pdf_max_);
				imgui_draw_in(draw, pdf_win_.box.min, pdf_win_.box.max);
			}
		}
		bool handle_mouse_wheel(float x, float y, float wh);

		bool pdf_invert_ = false;
		
		bool pdf_invert(opt<bool> const & v) {
			if (v) {
				pdf_invert_ = *v;
			}
			return pdf_invert_;
		}
		
		bool pdf_sticky(getopt<bool> const & toggle, opt<bool> const & v);
		
		void schem_highlight(list<float> x, opt<float> y, opt<float> w, opt<float> h);
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
		struct page_cache {
			unsigned char * thumb = nullptr;
			unsigned char * img_buf = nullptr;
			Image * pdf_img = nullptr;
			ImVec2 dim;
		};

		std::map<int, page_cache> page_cache_;

		struct helper_cairo_write_t {
			std::vector<unsigned char> data;
		};
		
		void draw_page(int page, opt<Component *> const & inside);

		static cairo_status_t cairo_write_func(void * c, const unsigned char * data, unsigned int len) {
			auto * p = (helper_cairo_write_t *) c;
			p->data.insert(p->data.end(), data, data + len);
			return CAIRO_STATUS_SUCCESS;
		}

		page_cache * render_page(int page);

		static constexpr const char * schematic_open_opt = "background";
		void schematic_open(getopt<bool> const & background, std::string const & fname) {
			schem_thread_joinable_ = false;
			if (background) {
				schem_open_thread_ = new std::thread([fname, this] { schematic_open_impl(fname, true); });
			} else {
				schematic_open_impl(fname, false);
			}
		}

		void schematic_open_impl(std::string const & fname, bool bg);
#else // OBV_USE_POPPLER
		void imgui_draw(ImDrawList *);

		bool handle_mouse_drag(ImVec2 const &, ImVec2 const &, bool);
		bool handle_mouse_wheel(float, float, float);
		bool grab_mouse_hover();
		bool handle_mouse_click(ImVec2 const & spos) {
			if (spos.x) { }
			return false;
		}
#endif // OBV_USE_POPPLER

		Component * currently_hovered_part_ = nullptr;
		
		Component * currently_hovered_part() {
			return currently_hovered_part_;
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
#ifdef HAVE_READLINE
			rl_callback_handler_remove();
#endif
			farewell_newline_ = false;
			*done_ = true;
		}

		static constexpr const char * report_prop_opt = "verbose";
		void report_prop(getopt<bool> const & verbose, list<any_obv_type> const & obj);

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
				tcli->eval(startup_script);
				//std::cerr << (std::string) r << "\n";
				startup_script.clear();
			}
		}

		std::string startup_script;
		
		TCL(BoardView * bvv, bool * d, std::string const & script);

		static void exit_proc(ClientData) {
			*this_s_->done_ = true;
		}

		bool farewell_newline_ = true;
		~TCL() {
#ifdef HAVE_READLINE
			rl_callback_handler_remove();
			rl_deprep_terminal();
			if (farewell_newline_) std::cerr << "\n";
#endif
		}

		static TCL * this_s_;

#ifdef HAVE_READLINE
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
			bool nonws = false;
			for (int i = 0; i < len; ++i) {
				if (! isspace(line[i])) {
					nonws = true;
					break;
				}
			}
			if (nonws) {
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
			}
			free(line);
		}
#endif // HAVE_READLINE
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

		bool pollfd(int sleep) {
			if (*done_) return false;
			bool ret = false;
			fd_set fd_r;
			FD_ZERO(&fd_r);
			FD_SET(0, &fd_r);
			int maxfd = 1;
			{
				int fd = BoardView::m_wakeup_pipe[0];
				if (fd < 0) {
					BoardView::wakeup();
				}
				fd = BoardView::m_wakeup_pipe[0];
				if (fd >= 0) {
					FD_SET(fd, &fd_r);
					if (fd > maxfd) maxfd = fd;
				}
			}
			timeval tout;

			tout.tv_sec = 0; tout.tv_usec = sleep;

			boardview()->sleep_mutex_unlock();
			int r = ::select(maxfd + 1, &fd_r, nullptr, nullptr, &tout);
			boardview()->sleep_mutex_lock();

			if (schem_thread_joinable_) {
				//std::cerr << "pdf load thread exit...\n";
				schem_open_thread_->join();
				schem_open_thread_ = nullptr;
				schem_thread_joinable_ = false;
			}

			if (r) {
				if (FD_ISSET(0, &fd_r)) {
#ifndef HAVE_READLINE
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
				} else if (FD_ISSET(BoardView::m_wakeup_pipe[0], &fd_r)) {
					char buf[1024];
					read(BoardView::m_wakeup_pipe[0], buf, sizeof(buf));
					ret = true;
				}
			}
			highlight_delay_poll();
			return ret;
		}
	};
}

typedef OBV_Tcl::TCL TCL;
