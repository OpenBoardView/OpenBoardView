/* MDB Tools - A library for reading MS Access database file
 * Copyright (C) 2000 Brian Bruns
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* 2016-12-20: openboardview@think-future.com
 * add x64 (ubuntu) compile instructions:
 * gcc -L/usr/lib/x86_64-linux-gnu/ -o bv2raw bv2raw.c -I/usr/include/glib-2.0/ -I/usr/lib/x86_64-linux-gnu/glib-2.0/include/ -lglib-2.0 -lmdb
 * add short usage information:
 * Usage: bv2raw boardfile.bv > boardfile.bvr
 */

#include "mdbtools.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#undef MDB_BIND_SIZE
#define MDB_BIND_SIZE 200000

#define is_quote_type(x) (x==MDB_TEXT || x==MDB_OLE || x==MDB_MEMO || x==MDB_DATETIME || x==MDB_BINARY || x==MDB_REPID)
#define is_binary_type(x) (x==MDB_OLE || x==MDB_BINARY || x==MDB_REPID)

static char *escapes(char *s);

//#define DONT_ESCAPE_ESCAPE
	static void
print_col(FILE *outfile, gchar *col_val, int quote_text, int col_type, int bin_len, char *quote_char, char *escape_char, int bin_mode)
	/* quote_text: Don't quote if 0.
	*/
{
	size_t quote_len = strlen(quote_char); /* multibyte */

	size_t orig_escape_len = escape_char ? strlen(escape_char) : 0;

	/* double the quote char if no escape char passed */
	if (!escape_char)
		escape_char = quote_char;

	if (quote_text && is_quote_type(col_type)) {
		fputs(quote_char, outfile);
		while (1) {
			if (is_binary_type(col_type)) {
				if (bin_mode == MDB_BINEXPORT_STRIP)
					break;
				if (!bin_len--)
					break;
			} else /* use \0 sentry */
				if (!*col_val)
					break;

			if (quote_len && !strncmp(col_val, quote_char, quote_len)) {
				fprintf(outfile, "%s%s", escape_char, quote_char);
				col_val += quote_len;
#ifndef DONT_ESCAPE_ESCAPE
			} else if (orig_escape_len && !strncmp(col_val, escape_char, orig_escape_len)) {
				fprintf(outfile, "%s%s", escape_char, escape_char);
				col_val += orig_escape_len;
#endif
			} else if (is_binary_type(col_type) && *col_val <= 0 && bin_mode == MDB_BINEXPORT_OCTAL)
				fprintf(outfile, "\\%03o", *(unsigned char*)col_val++);
			else
				putc(*col_val++, outfile);
		}
		fputs(quote_char, outfile);
	} else
		fputs(col_val, outfile);
}



int main(int argc, char **argv) {
	unsigned int i;
	MdbHandle *mdb;
	MdbTableDef *table;
	MdbColumn *col;
	char **bound_values;
	int  *bound_lens; 
	FILE *outfile = stdout;
	char *delimiter = NULL;
	char *row_delimiter = NULL;
	char *quote_char = NULL;
	char *escape_char = NULL;
	int header_row = 1;
	int quote_text = 1;
	char *insert_dialect = NULL;
	char *date_fmt = NULL;
	char *namespace = NULL;
	char *str_bin_mode = NULL;
	int bin_mode = MDB_BINEXPORT_RAW;
	char *value;
	size_t length;
	int this_table;
	char *tables[] = { "Layout", "Pin", "Nail" };

	/* Process options */
	quote_char = g_strdup("");
	delimiter = g_strdup("\t");
	row_delimiter = g_strdup("\n");

	/* Open file */
	if (!(mdb = mdb_open(argv[1], MDB_NOFLAGS))) {
		/* Don't bother clean up memory before exit */
		exit(1);
	}


	fprintf(outfile,"BVRAW_FORMAT_1\n");
	for (this_table = 0; this_table < 3; this_table++) {

		table = mdb_read_table_by_name(mdb, tables[this_table], MDB_TABLE);
		if (!table) {
			fprintf(stderr, "Error: Table %s does not exist in this database.\n", argv[2]);
			/* Don't bother clean up memory before exit */
			exit(1);
		}

		fprintf(outfile,"<<%s>>\n",tables[this_table]);
		/* read table */
		mdb_read_columns(table);
		mdb_rewind_table(table);

		bound_values = (char **) g_malloc(table->num_cols * sizeof(char *));
		bound_lens = (int *) g_malloc(table->num_cols * sizeof(int));
		for (i=0;i<table->num_cols;i++) {
			/* bind columns */
			bound_values[i] = (char *) g_malloc0(MDB_BIND_SIZE);
			mdb_bind_column(table, i+1, bound_values[i], &bound_lens[i]);
		}
		if (header_row) {
			for (i=0; i<table->num_cols; i++) {
				col=g_ptr_array_index(table->columns,i);
				if (i)
					fputs(delimiter, outfile);
				fputs(col->name, outfile);
			}
			fputs(row_delimiter, outfile);
		}

		while(mdb_fetch_row(table)) {

			for (i=0;i<table->num_cols;i++) {
				if (i>0)
					fputs(delimiter, outfile);
				col=g_ptr_array_index(table->columns,i);
				if (!bound_lens[i]) {
					/* Don't quote NULLs */
					if (insert_dialect)
						fputs("NULL", outfile);
				} else {
					if (col->col_type == MDB_OLE) {
						value = mdb_ole_read_full(mdb, col, &length);
					} else {
						value = bound_values[i];
						length = bound_lens[i];
					}
					print_col(outfile, value, quote_text, col->col_type, length, quote_char, escape_char, bin_mode);
					if (col->col_type == MDB_OLE)
						free(value);
				}
			}
			if (insert_dialect) fputs(");", outfile);
			fputs(row_delimiter, outfile);
		}

		/* free the memory used to bind */
		for (i=0;i<table->num_cols;i++) {
			g_free(bound_values[i]);
		}
		g_free(bound_values);
		g_free(bound_lens);
		mdb_free_tabledef(table);
	}

	mdb_close(mdb);
	//g_option_context_free(opt_context);

	// g_free ignores NULL
	g_free(quote_char);
	g_free(delimiter);
	g_free(row_delimiter);
	g_free(insert_dialect);
	g_free(date_fmt);
	g_free(escape_char);
	g_free(namespace);
	g_free(str_bin_mode);
	return 0;
}

static char *escapes(char *s)
{
	char *d = (char *) g_strdup(s);
	char *t = d;
	char *orig = s;
	unsigned char encode = 0;

	for (;*s; s++) {
		if (encode) {
			switch (*s) {
				case 'n': *t++='\n'; break;
				case 't': *t++='\t'; break;
				case 'r': *t++='\r'; break;
				default: *t++='\\'; *t++=*s; break;
			}	
			encode=0;
		} else if (*s=='\\') {
			encode=1;
		} else {
			*t++=*s;
		}
	}
	*t='\0';
	g_free(orig);
	return d;
}
