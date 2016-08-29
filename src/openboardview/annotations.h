#include "sqlite3.h"

#ifndef __ANNOTATIONS
#define __ANNOTATIONS
#define ANNOTATION_FNAME_LEN_MAX 2048

struct Annotation {
	int id;
	int side;
	string note, net, part, pin;
	double x, y;
	bool hovered;
};

struct Annotations {
	std::string filename;
	sqlite3 *sqldb;
	bool debug = false;
	vector<Annotation> annotations;

	int Init(void);

	int SetFilename(const std::string &f);
	int Load(void);
	int Close(void);
	int Add(char *net, char *part, double x, double y, char *annotation);
	void Remove(int id);
	void Add(int side, double x, double y, char *net, char *part, char *pin, char *note);
	void Update(int id, char *note);
	void GenerateList(void);
};

#endif
