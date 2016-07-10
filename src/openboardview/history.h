#ifndef HISTORY
#define HISTORY
#define HISTORY_COUNT_MAX 20
#define HISTORY_FNAME_LEN_MAX 2048
struct FHistory {
	int count;                                              // How many entries in the history array
	char history[HISTORY_COUNT_MAX][HISTORY_FNAME_LEN_MAX]; // Array of files in the history
	char *fname;

	~FHistory();
	char *Trim_filename(char *s, int stops);
	int Set_filename(const char *f);
	int Load(void);
	int Prepend_save(char *newfile);
};
#endif
