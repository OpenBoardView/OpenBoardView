#pragma once

#include "BRDFile.h"

class CSTFile : public BRDFile {
  public:
	CSTFile(std::vector<char> &buf);
	~CSTFile() {
		free(file_buf);
	}

  private:
	void gen_outline();
	void update_counts();

	unsigned int num_nets = 0;
	std::vector<char *> nets;
};
