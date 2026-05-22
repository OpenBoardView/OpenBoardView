#pragma once

#include "BRDFileBase.h"

class CSTFile : public BRDFileBase {
  public:
	CSTFile(std::vector<char> &buf);

  private:
	void gen_outline();
	void update_counts();

	unsigned int num_nets = 0;
	std::vector<char *> nets;
};
