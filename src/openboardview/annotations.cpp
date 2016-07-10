#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits.h>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "annotations.h"

int BoardView::Annotations_set_filename(char *f) {
	snprintf(annotations.fname, ANNOTATION_FNAME_LEN_MAX - 1, "%s", f);
	return 0;
}

int BoardView::Annotations_load(void) {
	return 0;
}

int BoardView::Annotations_save(void) {
	return 0;
}

int BoardView::Annotations_add(char *net, char *part, double x, double y, char *annotation) {
	return 0;
}

int BoardView::Annotations_del(int index) {
	return 0;
}

int BoardView::Annotations_update(int index, char *annotation) {
	return 0;
}
