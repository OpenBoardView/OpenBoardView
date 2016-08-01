

#ifndef _WIN32
#include <sqlite3.h>
#else
#include <winsqlite3/winsqlite.h>
#endif

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

	char filename[ANNOTATION_FNAME_LEN_MAX + 1];
	sqlite3 *sqldb;
	bool debug;
	vector<Annotation> annotations;

	int Init(void);

	int SetFilename(char *f);
	int Load(void);
	int Close(void);
	int Add(char *net, char *part, double x, double y, char *annotation);
	void Remove(int id);
	void Add(int side, double x, double y, char *net, char *part, char *pin, char *note);
	void Update(int id, char *note);
	void GenerateList(void);
};

#endif
