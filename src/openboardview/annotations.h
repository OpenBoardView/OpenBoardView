
struct annotations {

#define ANNOTATION_NET_SIZE 256
#define ANNOTATION_PART_SIZE 256
#define ANNOTATION_FNAME_LEN_MAX 2048
	struct annotation {
		char net[ANNOTATION_NET_SIZE];
		char part[ANNOTATION_PART_SIZE];
		double x;
		double y;
		int side;
		char *comment;
	};

	struct annotations {
		char fname[ANNOTATION_FNAME_LEN_MAX];
		// vector of struct annotation
	};

	int Annotations_set_filename(char *f);
	int Annotations_load(void);
	int Annotations_save(void);
	int Annotations_add(char *net, char *part, double x, double y, char *annotation);
	int Annotations_del(int index);
	int Annotations_update(int index, char *annotation);
};
