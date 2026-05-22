#include <string>
#include <vector>

#include "sqlite3.h"

#ifndef _ANNOTATIONS_H_
#define _ANNOTATIONS_H_

struct Annotation {
	int id;
	int side;
	std::string note, net, part, pin;
	double x, y;
	bool hovered;
};

class Annotations {
private:
	std::string filename;
	sqlite3 *sqldb;

public:
	std::vector<Annotation> annotations;

	bool Init();

	void SetFilename(const std::string &f);
	bool Load();
	bool Open(bool create);
	bool Close();
	bool Remove(int id);
	bool Add(int side, double x, double y, const std::string &net, const std::string &part, const std::string &pin, const std::string &note);
	bool Update(int id, const std::string &note);
	bool GenerateList();
};

#endif//_ANNOTATIONS_H_
