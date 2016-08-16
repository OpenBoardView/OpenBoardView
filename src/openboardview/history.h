#ifndef FHISTORY
#define FHISTORY
#define FHISTORY_COUNT_MAX 20
#define FHISTORY_FNAME_LEN_MAX 2048
struct FHistory {
	int count;                                                // How many entries in the history array
	char history[FHISTORY_COUNT_MAX][FHISTORY_FNAME_LEN_MAX]; // Array of files in the history
	std::string fname;

	~FHistory();
	char *Trim_filename(char *s, int stops);
	int Set_filename(const std::string &name);
	int Load(void);
	int Prepend_save(char *newfile);
};
#endif
