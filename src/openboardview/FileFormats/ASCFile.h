#pragma once

#include "BRDFile.h"
class ASCFile : public BRDFile {
  public:
	ASCFile(std::vector<char> &buf, const std::string &filename);
	~ASCFile() {
		free(file_buf);
	}

	//	static bool verifyFormat(std::vector<char> &buf);
	void parse_format(char *&p, char *&s, char *&arena, char *&arena_end, char **&lines);
	void parse_pin(char *&p, char *&s, char *&arena, char *&arena_end, char **&lines);
	void parse_nail(char *&p, char *&s, char *&arena, char *&arena_end, char **&lines);
	bool read_asc(const std::string &filename, void (ASCFile::*parser)(char *&, char *&, char *&, char *&, char **&));
	void update_counts();

  protected:
	bool m_firstformat = true;
	bool m_firstpin    = true;
	bool m_firstnail   = true;
};
