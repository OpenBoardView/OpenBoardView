#include "SExpr.h"

#include <cctype>
#include <cstdlib>
#include <cstring>

// KiCad files nest ~10 deep; anything past this is not a board file, it's an attack or a bug.
static const int kMaxDepth = 128;

const std::string &SExpr::head() const {
	static const std::string empty;
	if (!is_list || list.empty() || list.front().is_list) return empty;
	return list.front().atom;
}

const SExpr *SExpr::find(const char *name) const {
	for (const SExpr &c : list)
		if (c.is_list && c.head() == name) return &c;
	return nullptr;
}

std::vector<const SExpr *> SExpr::find_all(const char *name) const {
	std::vector<const SExpr *> out;
	for (const SExpr &c : list)
		if (c.is_list && c.head() == name) out.push_back(&c);
	return out;
}

const std::string &SExpr::str(size_t i) const {
	static const std::string empty;
	if (!is_list || i >= list.size() || list[i].is_list) return empty;
	return list[i].atom;
}

double SExpr::num(size_t i, double def) const {
	const std::string &s = str(i);
	if (s.empty()) return def;
	char *endp  = nullptr;
	double v    = strtod(s.c_str(), &endp);
	if (endp == s.c_str()) return def;
	return v;
}

SExprParser::SExprParser(const char *begin_, const char *end_) : p(begin_), end(end_), begin(begin_) {}

void SExprParser::skip(const char *head) {
	m_skip.push_back(head);
}

bool SExprParser::fail(const char *what) {
	if (m_error.empty()) {
		m_error = what;
		m_error += " at byte ";
		m_error += std::to_string(static_cast<long long>(p - begin));
	}
	return false;
}

void SExprParser::skip_ws() {
	while (!at_end() && isspace(static_cast<unsigned char>(*p))) ++p;
}

bool SExprParser::read_atom(std::string &out) {
	out.clear();
	if (at_end()) return fail("unexpected end of input");
	if (*p == '"') {
		++p;
		while (!at_end() && *p != '"') {
			if (*p == '\\') {
				++p;
				if (at_end()) break;
				switch (*p) {
					case 'n': out += '\n'; break;
					case 't': out += '\t'; break;
					default: out += *p; break; // \" and \\ and anything else: literal
				}
			} else {
				out += *p;
			}
			++p;
		}
		if (at_end()) return fail("unterminated string");
		++p; // closing quote
		return true;
	}
	const char *start = p;
	while (!at_end() && !isspace(static_cast<unsigned char>(*p)) && *p != '(' && *p != ')') ++p;
	if (p == start) return fail("expected atom");
	out.assign(start, p - start);
	return true;
}

// `p` is just past a '('. Consume through the matching ')', honouring quoted strings.
bool SExprParser::skip_list() {
	int depth = 1;
	while (!at_end() && depth > 0) {
		char c = *p++;
		if (c == '"') {
			while (!at_end() && *p != '"') {
				if (*p == '\\') ++p;
				if (!at_end()) ++p;
			}
			if (at_end()) return fail("unterminated string");
			++p;
		} else if (c == '(') {
			++depth;
		} else if (c == ')') {
			--depth;
		}
	}
	if (depth != 0) return fail("unbalanced parentheses");
	return true;
}

bool SExprParser::parse_list(SExpr &out, int depth) {
	// `p` is just past the '('
	if (depth > kMaxDepth) return fail("nesting too deep");
	out.is_list = true;
	out.atom.clear();
	out.list.clear();

	skip_ws();
	// Decide up front whether this whole list is one we skip: peek at the head atom.
	if (!at_end() && *p != '(' && *p != ')' && !m_skip.empty()) {
		const char *save = p;
		std::string head;
		if (!read_atom(head)) return false;
		bool skip_it = false;
		for (const std::string &s : m_skip)
			if (s == head) {
				skip_it = true;
				break;
			}
		if (skip_it) {
			// Leave a marker with just the head so callers can still see what was there.
			SExpr h;
			h.atom = head;
			out.list.push_back(h);
			return skip_list();
		}
		p = save;
	}

	while (true) {
		skip_ws();
		if (at_end()) return fail("unexpected end of input inside list");
		if (*p == ')') {
			++p;
			return true;
		}
		if (*p == '(') {
			++p;
			out.list.emplace_back();
			if (!parse_list(out.list.back(), depth + 1)) return false;
		} else {
			out.list.emplace_back();
			if (!read_atom(out.list.back().atom)) return false;
		}
	}
}

bool SExprParser::parse(SExpr &root) {
	m_error.clear();
	skip_ws();
	if (at_end() || *p != '(') return fail("expected '('");
	++p;
	if (!parse_list(root, 0)) return false;
	return true;
}
