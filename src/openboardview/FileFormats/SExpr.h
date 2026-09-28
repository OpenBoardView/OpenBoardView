#pragma once

#include <cstddef>
#include <string>
#include <vector>

// Minimal S-expression reader for KiCad-style files (`.kicad_pcb`, `.kicad_mod`, ...).
//
// A node is either an atom (bare token or "quoted string", already unescaped) or a list.
// Parsing never throws: malformed input stops early with `error` set and whatever was built so far.
// Subtrees whose head symbol is in the skip set are consumed without being materialised, which is
// what keeps multi-megabyte zone fills from costing anything.
struct SExpr {
	bool is_list = false;
	std::string atom;        // valid when !is_list
	std::vector<SExpr> list; // valid when is_list

	// First atom of a list ("footprint", "pad", ...); empty for atoms and empty lists.
	const std::string &head() const;
	// First child list whose head() == name, or nullptr.
	const SExpr *find(const char *name) const;
	// Every child list whose head() == name, in order.
	std::vector<const SExpr *> find_all(const char *name) const;
	// list[i] as an atom string; empty string if out of range or not an atom.
	const std::string &str(size_t i) const;
	// list[i] as a number; `def` if out of range or not numeric.
	double num(size_t i, double def = 0.0) const;
	size_t size() const {
		return list.size();
	}
};

class SExprParser {
  public:
	SExprParser(const char *begin, const char *end);

	// Heads whose entire subtree is skipped (e.g. "zone", "segment"). Match is on the first atom.
	void skip(const char *head);

	// Parses one top-level list into `root`. Returns false (with error() set) on malformed input.
	bool parse(SExpr &root);

	const std::string &error() const {
		return m_error;
	}

  private:
	bool parse_list(SExpr &out, int depth);
	bool skip_list();          // consume up to and including the matching ')' — `p` is just past '('
	bool read_atom(std::string &out);
	void skip_ws();
	bool at_end() const {
		return p >= end;
	}
	bool fail(const char *what);

	const char *p;
	const char *end;
	const char *begin;
	std::vector<std::string> m_skip;
	std::string m_error;
};
