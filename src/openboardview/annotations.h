
struct annotations {
	int Annotations_set_filename(char *f);
	int Annotations_load(void);
	int Annotations_save(void);
	int Annotations_add(char *net, char *part, double x, double y, char *annotation);
	int Annotations_del(int index);
	int Annotations_update(int index, char *annotation);
};
