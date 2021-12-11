// -*- c++ -*-
#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wparentheses"

#define HAVE_READLINE
#define OBV_USE_POPPLER
#include "TCLDUMMY.h"


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
#include <stack>
#include <optional>

#ifdef HAVE_READLINE
#define _FUNCTION_DEF
#include <readline/readline.h>
#include <readline/history.h>
#undef _FUNCTION_DEF
#endif
#include <SDL.h>

#include "BoardView.h"
#include "platform.h"
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
		
		std::string slave_eval(std::string const & code);
		void slave_pass_variable(std::string const & varname);
		
		Board * brd() { return b->m_board.get(); }
		BoardView * boardview() { return b; }

		template <typename T>
		using getopt = Tcl::getopt<T>;
	public :
		static TCL * this_s_;
		Tcl::interpreter * tcli = nullptr;

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
			int mark = 0;
		};

		struct page_cache {
			unsigned char * thumb = nullptr;
			unsigned char * img_buf = nullptr;
			Image * pdf_img = nullptr;
			ImVec2 dim;
			float quality;

			page_cache() = default;
			page_cache(page_cache && o) {
				img_buf = o.img_buf;
				pdf_img = o.pdf_img;
				dim     = o.dim;
				quality = o.quality;
				o.img_buf = nullptr;
				o.pdf_img = nullptr;
			}
			page_cache(page_cache const &) = delete;
			~page_cache() {
				if (img_buf) delete img_buf;
				if (pdf_img) delete pdf_img;
			}
		};

		static int schem_render_concurrency_;
		
		struct schematic {
			std::string filename;
			PDFDoc * doc = nullptr;
			PopplerDocument * poppler_doc = nullptr;

			std::vector<PopplerDocument *> render_docs;
			int render_docs_inflight = 0;
			
			typedef std::multimap<std::string, pdf_txt_bbox *> pdf_words_byname_t;
			typedef std::deque<pdf_txt_bbox> pdf_words_value_t;
			std::map<int, pdf_words_value_t> words;
			std::map<int, pdf_words_byname_t> words_by_name;

			std::multimap<int, page_cache> page_cache_;

			pdf_txt_bbox * add(pdf_txt_bbox const & w) {
				auto & wref = words[w.page];
				wref.push_back(w);
				auto & ins = wref.back();
				if (! w.word.empty()) {
					words_by_name[w.page].insert(std::pair(w.word, &ins));
				}
				return &wref.back();
			}
			void del(pdf_txt_bbox const & w) {
				if (! w.word.empty()) {
					auto it = words_by_name.find(w.page);
					if (it != words_by_name.end()) {
						for (auto it2 = it->second.lower_bound(w.word), it2end = it->second.upper_bound(w.word); it2 != it2end; ++it2) {
							if (it2->second == &w) {
								it->second.erase(it2);
								break;
							}
						}
					}
				}

				auto it = words.find(w.page);
				if (it != words.end()) {
					for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
						if (& (*it2) == &w) {
							it->second.erase(it2);
							break;
						}
					}
				}
			}
		};

		std::vector<schematic> schematics_;

		schematic * default_schematic_ = nullptr;
		
		std::optional<std::map<std::string, object> > get_variables_from_expr(interpreter *, std::string const & expr, bool command = false, bool throw_on_error = false);
		std::string join(std::optional<std::set<std::string> > const & v) {
			if (v) {
				std::ostringstream oss;
				std::copy(v->begin(), v->end(),
						  std::ostream_iterator<std::string>(oss, " "));
				return oss.str();
			}
			return "";
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
				r.append(tcli->makeobj(default_boards_[i]));
			}
		
			if (r.size(I) == 0) {
				bool found = false;
				for (std::size_t i = 0; i < boards_.size(); ++i) {
					if (boardview()->m_board.get() == static_cast<Board *>(boards_[i]))
						found = true;
					r.append(tcli->makeobj(boards_[i]));
				}
				if (!found) {
					r.append(tcli->makeobj(static_cast<BRDBoard *>(boardview()->m_board.get())));
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
				r.append(tcli->makeobj(boards_[i]));
			}
			if (!found) {
				r.append(tcli->makeobj(static_cast<BRDBoard *>(boardview()->m_board.get())));
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

		static constexpr const char * add_node_opt = "top bottom stub";
		void add_node(getopt<bool> const & top, getopt<bool> const & bottom, getopt<bool> const & stub, Pin * p1, Pin * p2, Net * n);
		
#ifdef OBV_USE_POPPLER
		static constexpr char const * distance_opt = "norm border";
		float distance_fun(getopt<bool> const & norm, getopt<bool> border, any<Component, Pin, pdf_txt_bbox> const & a, any<Component, Pin, pdf_txt_bbox> const & b);
		static constexpr char const * angle_opt = "ortho decay";
		float angle(getopt<bool> const & ortho, getopt<float> const & decay, any<pdf_txt_bbox, Pin> const & a, any<pdf_txt_bbox, Pin> const & b);
		object enclosed_box(list<pdf_txt_bbox> const & words);
#endif
		
		bool intersect(bbox const & a, bbox const & b) {
			if (in(a, b.min) || in(a, b.max) || in(a, ImVec2(b.min.x, b.max.y)) || in(a, ImVec2(b.max.x, b.min.y))) {
				return true;
			}
			return false;
		}
		
		static constexpr const char * get_schem_words_opt = "page filter re ref sort not of exact color bbox radius nearest";
		object get_schem_words(interpreter *, getopt<int> const & page, getopt<std::string> const & filter_arg, getopt<bool> const & use_regex, getopt<pdf_txt_bbox *> const & ref, getopt<std::string> const & sort, getopt<list<pdf_txt_bbox> > const & not_, getopt<any<list<Component> , list<Pin>, list<Net>, list<pdf_txt_bbox> > > const & of, getopt<std::string> const & exact, getopt<std::string> const & color, getopt<list<pdf_txt_bbox> > const & bbox, getopt<float> const & radius, getopt<int> const & nearest, variadic<std::string> const & match);
		static constexpr const char * get_nets_opt = "re all not gnd of filter";
		object get_nets(interpreter *, getopt<bool> const & use_regex, getopt<bool> const & all, getopt<list<Net> > const & not_, getopt<bool> const & gnd, getopt<any<list<Component>, list<Pin>, list<Net>, list<BRDBoard> > > const & of, getopt<std::string> const & filter_arg, variadic<std::string> const & match);
		static constexpr const char * get_pins_opt = "re all parallel filter via of notof not ref sort brief";
		object get_pins(interpreter *, getopt<bool> const & use_regex, getopt<bool> const & all, getopt<bool> const & parallel, getopt<std::string> const & filter_arg, getopt<bool> const & via, getopt<any<list<Component>, list<Net>, list<Pin>, list<BRDBoard> > > const & of, getopt<any<list<Component>, list<Net>, list<Pin>, list<BRDBoard> > > const & notof, getopt<list<Pin> > const & not_, getopt<Pin *> const & ref, getopt<std::string> const & sort, getopt<bool> const & brief, variadic<std::string> const & match);
		static constexpr const char * highlight_opt = "save toggle un all color colorindex intensity";
		void highlight(getopt<bool> const & save, getopt<bool> const & toggle, getopt<bool> const & un, getopt<bool> const & all, getopt<std::string> const & color, getopt<int> const & colorindex, getopt<float> const & inten, opt<list<any<Component, Pin, pdf_txt_bbox> > > const & obj);
		const char * get_cells_opt = "filter sort re of all top bottom not ref";
		object get_cells(interpreter *, getopt<std::string> const & filter_arg, getopt<std::string> const & sort, getopt<bool> const & use_regex, getopt<any<list<Component>, list<Net>, list<Pin>, list<pdf_txt_bbox> > > const & of, getopt<bool> const & all,
						 getopt<bool> const & top,
						 getopt<bool> const & bottom,
						 getopt<list<Component> > const & not_, getopt<Component *> const & ref, variadic<std::string> const & match);
		object get_schem_windows();

		
		static constexpr const char * generate_mark_opt = "set";
		int generate_mark(getopt<int> set) {
			if (set) {
				generate_mark_ = *set;
			}
			return generate_mark_++;
		}
		int generate_mark_ = 1;

		struct prop_stack_item {
			pdf_txt_bbox * word;
			int saved_mark;
			uint32_t saved_color;
		};

		struct prop_stack_frame {
			std::vector<prop_stack_item> items;
			bool mark;
			bool highlight;
			prop_stack_frame(bool m, bool h) : mark(m), highlight(h) { }
		};
		
		std::deque<prop_stack_frame> prop_stack_;
		bool prop_stack_enable_ = true;

		void man_prop_stack(std::ostream & os) {
			os << "A property stack frame will record all values that have been\n"
			   << "overwritten by regular means, such as by the 'highlight' or 'set_prop'\n"
			   << "commands.\n"
			   << "If a property stack is commited, the information is simply discarded.\n"
			   << "If it is aborted, the values will be restored in reverse order.\n"
			   << "prop_stack_enable [bool] keeps the content of the stack intact, but\n"
			   << "any changes done while it is disabled will not be restorable\n";
		}

		bool same_object(object const & a, object const & b) {
			return a.get_object() == b.get_object();
		}
		
		static constexpr const char * prop_begin_opt = "help mark highlight";
		void prop_begin(getopt<bool> const & help, getopt<bool> const & mark, getopt<bool> const & highlight);

		void prop_stack_info() {
			std::cerr << prop_stack_.size() << " frames\n";
			for (int i = prop_stack_.size() - 1; i >= 0; --i) {
				std::cerr << "frame " << i << " " << prop_stack_[i].items.size() << " items\n";
			}
		}

		void prop_commit() {
			if (prop_stack_.empty()) {
				throw tcl_error("property stack is empty");
			}
			prop_stack_.pop_front();
		}
		bool prop_stack_enable(opt<bool> v) {
			if (v) {
				prop_stack_enable_ = *v;
			}
			return prop_stack_enable_;
		}

		void prop_abort();

		static constexpr const char * prop_frame_opt = "current historic";
		object prop_frame(getopt<bool> const & current, getopt<bool> const & historic, int frame_arg);
		
		void prop_stack_try_save_mark(pdf_txt_bbox * w) {
			if (! prop_stack_enable_ || prop_stack_.empty()) return;
			auto & r = prop_stack_.front();
			if (r.mark) {
				r.items.push_back({ w, w->mark, 0 });
			}
		}
		void prop_stack_try_save_highlight(pdf_txt_bbox * w) {
			if (! prop_stack_enable_ || prop_stack_.empty()) return;
			auto & r = prop_stack_.front();
			if (r.highlight) {
				r.items.push_back({ w, 0, w->color });
			}
		}

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
		void namespace_close(interpreter * interp, std::string const & name) {
			interp->eval(std::string("namespace delete ") + name);
		}
		
		struct namespace_holder {
			namespace_holder(TCL & t, interpreter * interp, std::string const & name) : this_(t), interp_(interp), name_(name) {
				if (! name.empty()) {
					this_.namespace_open(*interp_, name);
				}
			}
			~namespace_holder() {
				if (! name_.empty()) {
					this_.namespace_close(interp_, name_);
				}
			}
			TCL & this_;
			interpreter * interp_;
			std::string name_;
		};
		
		namespace_holder make_namespace_holder(interpreter * interp, std::string const & name) {
			return namespace_holder(*this, interp, name);
		}
		
		bool filter_install(Component * c, object & obj, const char * varname) {
			return cell_get_prop_impl(varname, c, obj, nullptr);
		}
		bool filter_install(Net * c, object & obj, const char * varname) {
			return net_get_prop_impl(varname, c, obj, nullptr);
		}
		bool filter_install(Pin * c, object & obj, const char * varname) {
			return pin_get_prop_impl(varname, c, obj, nullptr);
		}
		bool filter_install(pdf_txt_bbox * c, object & obj, const char * varname) {
			return schem_word_get_prop_impl(varname, c, obj, nullptr);
		}

		struct cmpeq_t {
			bool write_;
			cmpeq_t(bool w) : write_(w) { }
			cmpeq_t() : write_(false) { }

			bool operator()(const char * a, const char * b, bool writable = false) {
				if (! write_ || writable) {
					return strcasecmp(a, b) == 0;
				}
				return false;
			}
		};
		struct record_strings_t : public std::vector<std::pair<std::string, bool> > {
			record_strings_t() { }
			record_strings_t(record_strings_t const &) = delete;

			bool operator()(const char * a, const char * b, bool writable = false) {
				const char * c = a ? a : b;

				if (c) {
					push_back(std::pair(c, writable));
				}
				return false;
			}
		};

		template <typename CMP=cmpeq_t>
		static object schem_word_get_prop(const char * prop, pdf_txt_bbox * p, CMP icmp = cmpeq_t()) {
			object ret;
			schem_word_get_prop_impl(prop, p, ret, nullptr, icmp);
			return ret;
		}
		template <typename CMP=cmpeq_t>
		static object cell_get_prop(const char * prop, Component * c, CMP icmp = cmpeq_t()) {
			object ret;
			cell_get_prop_impl(prop, c, ret, nullptr, icmp);
			return ret;
		}
		template <typename CMP=cmpeq_t>
		static object pin_get_prop(const char * prop, Pin * p, CMP icmp = cmpeq_t()) {
			object ret;
			pin_get_prop_impl(prop, p, ret, nullptr, icmp);
			return ret;
		}
		template <typename CMP=cmpeq_t>
		static object net_get_prop(const char * prop, Net * n, CMP icmp = cmpeq_t()) {
			object ret;
			net_get_prop_impl(prop, n, ret, nullptr, icmp);
			return ret;
		}
		template <typename CMP=cmpeq_t>
		static object schematic_get_prop(const char * prop, schematic * s, CMP icmp = cmpeq_t()) {
			object ret;
			schematic_get_prop_impl(prop, s, ret, nullptr, icmp);
			return ret;
		}
		
		template <typename CMP=cmpeq_t>
		static bool schem_word_get_prop_impl(const char * prop, pdf_txt_bbox * p, object & ret, object * set, CMP icmp = cmpeq_t());
		template <typename CMP=cmpeq_t>
		static bool cell_get_prop_impl(const char * prop, Component * c, object & ret, object * set, CMP icmp = cmpeq_t());
		template <typename CMP=cmpeq_t>
		static bool pin_get_prop_impl(const char * prop, Pin * p, object & ret, object * set, CMP icmp = cmpeq_t());
		template <typename CMP=cmpeq_t>
		static bool net_get_prop_impl(const char * prop, Net * n, object & ret, object * set, CMP icmp = cmpeq_t());
		template <typename CMP=cmpeq_t>
		static bool schematic_get_prop_impl(const char * prop, schematic * s, object & ret, object * set, CMP icmp = cmpeq_t());
		
		static char const ** cell_props_, ** pin_props_, ** net_props_, ** pdf_word_props_, ** schematic_props_;

		template <typename Fn>
		static char const ** enumerate_props_impl(Fn, cmpeq_t &) {
			return nullptr;
		}
		template <typename Fn>
		static char const ** enumerate_props_impl(Fn fn, record_strings_t & rec) {
			object obj;
			object set;
			fn(nullptr, nullptr, obj, &set, rec);

			char const ** p = new char const *[rec.size() + 1];
			for (std::size_t i = 0; i < rec.size(); ++i) {
				char * pp = new char[rec[i].first.size() + 2];
				strcpy(pp, rec[i].first.c_str());
				pp[rec[i].first.size() + 1] = rec[i].second ? 1 : 0;
				p[i] = const_cast<const char *>(pp);
			}
			p[rec.size()] = nullptr;
			rec.clear();
			return p;
		}
		static bool static_initialized_;
		static void static_initialize();

		template <typename T>
		using is_CNP = detail::contains<T, Pin, Net, Component, pdf_txt_bbox>; //std::integral_constant<bool, std::is_same<T, Pin>::value || std::is_same<T, Net>::value || std::is_same<T, Component>::value>;

		struct variables_installer {
			variables_installer(interpreter & interp, std::string const & ns, std::map<std::string, object> * vars)
				: interp_(interp), ns_(ns), vars_(vars) {
				if (vars) {
					for (auto it : *vars) {
						interp_.setVar(ns + "::" + it.first, it.second);
					}
				}
			}
			~variables_installer() {
				if (vars_) {
					for (auto it : *vars_) {
						interp_.unsetVar(ns_ + "::" + it.first);
					}
				}
			}
			interpreter & interp_;
			std::string ns_;
			std::map<std::string, object> * vars_;
		};
		variables_installer make_variables_installer(interpreter * tcli, std::string const & ns, std::map<std::string, object> * vars) {
			return variables_installer(*tcli, ns, vars);
		}
		
		struct disown {
			object & o_;
			void segfault() {
				char * p = (char *) 0;
				*p = 1;
			}
			disown(object & o) : o_(o) {
				o.disown();
				//if (o.get_object()->refCount != 1) {
				//	segfault();
				//}
			}
			~disown() {
				o_.reown();
				//if (o_.get_object()->refCount != 2) {
				//	segfault();
				//}
			}
		};

		object make_imvec2(float, float, bool);
		void print_imvec2(ImVec2 *);

		
		template <typename OT>
		bool sort_less(interpreter & I, std::string const & ns, std::map<std::string, object> * vars, object const & eval, OT * a, OT * b, OT * r) {
			const bool dump = false;

			struct {
				OT * o; const char * n;
			} v[] = { { a, "a" }, { b, "b" }, { r, "r" } };
			
			if (vars) {
				for (auto & i : *vars) {
					std::string const & name = i.first;
					if (name.size() > 2 && name[1] == '_') {
						disown d(i.second);
						
						bool ok = false;
						for (auto & vi : v) {
							if (name[0] == vi.n[0]) {
								if (filter_install(vi.o, i.second, name.substr(2).c_str())) {
									ok = true;
								}
								break;
							}
						}
						if (!ok) {
							throw tcl_error(std::string("illegal variable name in sort expression: " + name));
						}
					}
				}
			} else {
				throw tcl_error("internal error");
			}

			for (auto & vi : v) {
				if (! vi.o ) continue;
				if (dump) { std::cerr << "set var " << vi.n << "\n"; }
				if (! vars) {
					I.setVar(ns + "::" + vi.n, I.makeobj(vi.o));
				} else {
					if (auto it = vars->find(vi.n); it != vars->end()) {
						disown d(it->second);
						tcli->makeobj_inplace(vi.o, it->second, false, (void *) 0);
					}
				}
			}

			bool res = I.eval(eval);
			I.result_reset();
			return res;
		}

		template <typename OT>
		typename std::enable_if<is_CNP<OT>::value && is_CNP<typename of_pins_t<OT>::type>::value, bool>::type
		filter_match(interpreter * tcli, std::string const & ns, std::map<std::string, object> * vars, object const & eval, OT * p, typename of_pins_t<OT>::type * of = nullptr, OT * ref = nullptr) {
			if (ns.empty()) return true;

			if (vars) {
				for (auto & i : *vars) {
					const std::string & name = i.first;
					disown d(i.second);
					if (ref && name.compare(0, 2, "r_") == 0) {
						filter_install(ref, i.second, name.substr(2).c_str());
					} else if (name != "r" && name != "_") {
						filter_install(p, i.second, i.first.c_str());
					}
				}
			} else {
				throw;
			}
			typename std::remove_reference<decltype(*vars)>::type::iterator it;
			
			if ((it = vars->find("_")) != vars->end()) {
				if (vars) {
					disown d(it->second);
					tcli->makeobj_inplace(p, it->second, false, (void *) 0);
				} else {
					tcli->setVar(ns + "::_", tcli->makeobj(p));
				}
			}
			if (ref && (it = vars->find("r")) != vars->end()) {
				if (vars) {
					disown d(it->second);
					tcli->makeobj_inplace(ref, it->second, false, (void *) 0);
				} else {
					tcli->setVar(ns + "::r", tcli->makeobj(ref));
				}
			}
			
			if (!vars || vars->count("bundle")) {
				tcli->setVar(ns + "::bundle", object(of_pins_t<OT>::of_pins(p, of)));
			}
			bool res = tcli->eval(eval);
			tcli->result_reset();
			return res;
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

		object dup(object const & o) {
			return o.duplicate();
		}
		
		std::string objinfo(list<object> const & o);
		
		std::mutex create_interp_mutex_;
		struct tcl_thread {
			interpreter * interp;
			std::thread * thread;
			int id;
			object result;
			std::mutex exit_mutex;
			std::atomic<bool> finished;
			std::function<void(void)> cleanup;
		};
		std::vector<tcl_thread *> threads_;
		static inline int threadid_alloc_ = 0;

		std::string varinfo(interpreter *, std::string const & var);
		static constexpr const char * create_thread_opt = "detach noscanvars";
		object create_thread(interpreter *, getopt<bool> const & detach, getopt_d<bool> const & noscanvars, std::string const &, opt<list<std::string> > const &);

		static constexpr const char * join_thread_opt = "try";
		object join_thread(getopt<bool> const & try_, list<tcl_thread> const &);
		object get_threads();
		
		
		static bool extra_properties_get_impl(const char * prop, object & ret, be_priv * pr) {
			if (pr) {
				auto it = pr->properties.find(prop);
				if (it != pr->properties.end()) {
					ret = it->second;
					return true;
				}
			}
			//ret = object();
			return false;
		}

		static object extra_properties_get(const char * prop, be_priv * pr) {
			object ret;
			extra_properties_get_impl(prop, ret, pr);
			return ret;
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

		typedef any<Component, Net, Pin, Board, pdf_txt_bbox, schematic> any_obv_type;

		void cell_set_prop(Component * c, std::string const & name, std::string const & value) {
			if (! c->tcl_priv) { c->tcl_priv = (void *) new cell_priv(); }
			be_priv * pr = (be_priv *) c->tcl_priv;
			pr->properties[name] = value;
		}

		
		void set_prop(std::string const & prop, std::string const & value, list<any<Component, Net, Pin, pdf_txt_bbox> > const & obj);
		
		object get_prop(std::string const & prop, list<any_obv_type> const & obj);
		
		std::vector<std::string> keybind_targets() {
			std::vector<std::string> ret;
			//return { "testkb" };
			object r = tcli->eval("info commands");
			//interpreter I(r.get_interp(), false);
			for (std::size_t i = 0; i < r.size(); ++i) {
				std::string s = r.at_ref(i).get<std::string>();
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

		struct schem_pos_t {
			ImVec2 point;
			int page;
		};

		std::optional<schem_pos_t> schem_position();

		bool handle_keypress();
		
		static constexpr const char * trace_opt = "off report count print";
		void trace(getopt<bool> const & off, getopt<bool> const & report, getopt<bool> const & count, getopt<bool> const & print) {
			bool v = off ? false : true;
			bool none = !count && !print;
			
			if (report) {
				std::cerr << tcli->trace_count() << "\n";
			} else {
				if (print || none) {
					trace_ = v;
					tcli->trace(v);
				}
				if (count || none) {
					tcli->trace_count(0);
				}
			}
		}
		bool trace_ = false;

		void select(list<any_obv_type> obj);

		std::thread * schem_open_thread_ = nullptr;
		std::atomic<bool> schem_thread_joinable_ = false;

		bool in(bbox const & b, ImVec2 v) {
			return v > b.min && v < b.max;
		}
#ifdef OBV_USE_POPPLER
		Image * pdf_img_ = nullptr;
		unsigned char * pdf_img_buf_ = nullptr;

		struct pdf_window {
			bbox   box;
			bbox   saved_box;

			ImVec2 wdim;

			pdf_window() { }
			pdf_window(bbox const & bb, ImVec2 const & wd) : box(bb), saved_box(bb), wdim(wd) {
				update();
			}
			void update() {
				bdim     = box.max - box.min;
				aspect   = bdim.x / bdim.y;
				wb_ratio = wdim / bdim;
			}				
			void restore() {
				//box = saved_box;
			}
			
			ImVec2      bdim;     // derived
			float       aspect;   // derived
			ImVec2      wb_ratio; // derived
			int         page;
			bool        valid          = false;
			bool        reuse          = false;
			bool        sticky         = false;
			bool        saved_sticky   = false;
			bool        is_fullscreen  = false;
			bool        is_docked      = false;
			bool        ignore_one_mouse_release = false;
			bool        dragging       = false;
			bool        mouse_released = false;
			float       quality        = 0.0f;
			schematic * schematic_     = nullptr;
		};
		
		ImVec2 pdf2board(pdf_window const & w, Image * pdf, ImVec2 c, bool flip) {
			ImVec2 t = pdf->CoordToScreen(c / w.wdim);
			if (flip) {
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

		//typedef std::vector<pdf_txt_bbox> pdf_words_value_t;
		//std::map<int, pdf_words_value_t> pdf_words_;

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

		std::string schem_word_tooltip_;
		pdf_txt_bbox const * schem_word_tooltip_word_ = nullptr;
		
		void imgui_draw_in(ImDrawList * draw, ImVec2 min, ImVec2 max);
		ImVec2 yflip(ImVec2 const & v) {
			return { v.x, boardview()->m_current_side || pdf_win_.sticky ? v.y : (pdf_win_.box.min + pdf_win_.bdim - (v - pdf_win_.box.min)).y };
		}
		
		int handle_mouse_drag(ImVec2 const & ppos, ImVec2 const & drag, int token);

		uint64_t last_click_ = 0;
		bool handle_mouse_click(ImVec2 const & spos);
		
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
				if (*v != pdf_invert_) {
					//for (auto & s : schematics_) {
						//	s.page_cache_.clear();
					//}
				}
				pdf_invert_ = *v;
			}
			return pdf_invert_;
		}
		
		static constexpr const char * schem_window_opt = "toggle fullscreen docked sticky get";
		bool schem_window_cmd(getopt<bool> const & toggle, getopt<bool> const & fullscreen, getopt<bool> const & docked, getopt<bool> const & sticky, getopt<bool> const & get);
		
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
		struct helper_cairo_write_t {
			std::vector<unsigned char> data;
		};

		std::thread * draw_page_thread_ = nullptr;
		std::atomic<bool> draw_page_in_progress_ = false;
		std::mutex * draw_page_mutex_ = nullptr;
		
		static constexpr const char * draw_page_opt = "fullscreen docked background prefer_cache quality cache norender reusewin of origin zoom";
		void draw_page(getopt<bool> const & fullscreen, getopt<bool> const & docked, getopt<bool> const & background, getopt<bool> const & prefer_cache, getopt<float> const & quality, getopt<bool> const & cache, getopt<bool> const & norender, getopt<bool> const & reusewin, getopt<schematic *> const & of, getopt<std::string> const & center, getopt<float> const & zoom, int page, opt<Component *> const & inside);

		static constexpr const char * history_opt = "auto";
		object history(getopt<bool> const & automatic);
		
		static constexpr const char * get_schematics_opt = "";
		object get_schematics(); //std::getopt<std::list<pdf_txt_bbox> > const & of);

		std::vector<uint32_t> get_schem_word_colors(pdf_txt_bbox * word);
		
		static cairo_status_t cairo_write_func(void * c, const unsigned char * data, unsigned int len) {
			auto * p = (helper_cairo_write_t *) c;
			p->data.insert(p->data.end(), data, data + len);
			return CAIRO_STATUS_SUCCESS;
		}

		std::mutex render_page_mutex_;
		page_cache * render_page(int page, getopt<float> quality, schematic * sch, bool norender, bool prefer_cache);

		static constexpr const char * schematic_open_opt = "background preload preload_quality";
		void schematic_open(getopt<bool> const & background, getopt<bool> const & preload, getopt<float> const & preload_quality, std::string const & fname) {
			schem_thread_joinable_ = false;
			if (background) {
				schem_open_thread_ = new std::thread([=] { schematic_open_impl(fname, true, preload || preload_quality, preload_quality ? *preload_quality : 0.5f); });
			} else {
				schematic_open_impl(fname, false, preload || preload_quality, preload_quality ? *preload_quality : 0.5f);
			}
		}


		void schematic_open_impl(std::string const & fname, bool bg, bool preload, float preload_quality);
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

		int cover(int a, int b) {
			return a + b;
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
			eval("cell_hover_call", now ? tcli->makeobj(now) : object(""));
		}

		void notify_load_file() {
			if (! startup_script.empty()) {
				tcli->eval(startup_script);
				startup_script.clear();
			}
		}

		std::string startup_script;
		
		TCL(BoardView * bvv, bool * d, std::string const & script);

		void imbue_interpreter(interpreter *, bool);
		
		static void exit_proc(ClientData) {
			*this_s_->done_ = true;
		}

		bool farewell_newline_ = true;
		~TCL() {
#ifdef HAVE_READLINE
			rl_callback_handler_remove();
			rl_deprep_terminal();
			if (farewell_newline_) std::cerr << "\n";
			write_history((get_user_dir(UserDir::Data) + "/tcl_history").c_str());
#endif
		}

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
				auto res = this_s_->eval_impl(object("info commands"), true);
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
				
				std::string sline(line);
				object oline(sline);
				auto res = this_s_->eval_impl(oline);
				if (res.ok) {
					if (truncate && res.is_object_vector && res.str.size() > 256) {
						std::cerr << res.str.substr(0, 255) << " ...\n";
					} else {
						std::cerr << res.str << (res.str.size() ? "\n" : "");
					}
				}
				//std::cerr << "readline: " << line << "\n";
				add_history(line);
			}
			free(line);
		}
#endif // HAVE_READLINE
		//	std::thread thread_;

		struct eval_result {
			bool ok;
			bool is_object_vector;
			std::string str;
		};

		object last_result_;
	
		template <typename ...Args>
		eval_result eval(Args const &... args) {
			object a;
			(a.append(object(args)), ...);
			return eval_impl(a);
		}
		eval_result parse_eval(std::string const & cmd) {
			return eval("eval", cmd);
		}

		
		eval_result eval_impl(object const & cmd, bool internal = false) {
			if (trace_) {
				std::cerr << cmd.get<std::string>() << "\n";
			}

#if 0
			std::cerr << "eval ";
			for (std::size_t i = 0; i < cmd.size(); ++i) {
				std::cerr << cmd.at(i) << " ";
			}
			std::cerr << "\n";
#endif
			
			//object cmdo(cmd.c_str());
			try {
				object r = tcli->eval(cmd);

				//std::cerr << "result: " << tcl_typename(r) << " " << r.get<std::string>() << "\n";
			
				if (!internal) {
					last_result_ = r;
				}
				bool isvec = false;
				//interpreter I(r.get_interp(), false);
				if (r.is_list() && r.size()) {
					if (r.at_ref(0).get_object()->typePtr) {
						isvec = true;
					}
				}

				return { true, isvec, r.get<std::string>() };
			} catch (tcl_aborted & e) {
			} catch (std::runtime_error & e) {
				std::cerr << "while executing: " << cmd.get<std::string>() << ": " << e.what() << "\n";
				return { false, false, e.what() };
			}
			return { false, false, "" };
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
					if (res.ok) {
						if (res.is_object_vector && res.str.size() > 256) {
							std::cerr << res.str.substr(0, 255) << " ...\n";
						} else {
							std::cerr << res.str << (res.str.size() ? "\n" : "");
						}
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

#pragma GCC diagnostic pop
