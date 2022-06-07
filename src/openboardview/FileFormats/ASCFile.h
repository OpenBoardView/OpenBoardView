#pragma once

#include "BRDFileBase.h"

#include <vector>

#include "filesystem_impl.h"

class ASCFile : public BRDFileBase {
  public:
	typedef std::vector<char *>::iterator line_iterator_t;
	ASCFile(std::vector<char> &buf, const filesystem::path &filepath);

	//	static bool verifyFormat(std::vector<char> &buf);
	void parse_format(char *&p, char *&s, char *&arena, char *&arena_end, line_iterator_t &line_it);
	void parse_pin(char *&p, char *&s, char *&arena, char *&arena_end, line_iterator_t &line_it);
	void parse_nail(char *&p, char *&s, char *&arena, char *&arena_end, line_iterator_t &line_it);
	bool read_asc(const filesystem::path &filepath, void (ASCFile::*parser)(char *&, char *&, char *&, char *&, line_iterator_t&));
	bool load_and_parse(const filesystem::path &path, const std::string &filename, void (ASCFile::*parser)(char *&, char *&, char *&, char *&, line_iterator_t&));
	void update_counts();

  protected:
	bool m_firstformat = true;
	bool m_firstpin    = true;
	bool m_firstnail   = true;
};
