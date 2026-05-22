#include <string>

#include <SDL.h>

#include "Annotations.h"

void Annotations::SetFilename(const std::string &f) {
	filename = f;
}

bool Annotations::Init() {
	static const char sql_table_create[] =
	    "CREATE TABLE annotations("
	    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
	    "visible INTEGER,"
	    "pin TEXT,"
	    "part TEXT,"
	    "net TEXT,"
	    "posx INTEGER,"
	    "posy INTEGER,"
	    "side INTEGER,"
	    "note TEXT);";

	if (!sqldb) {
		return false;
	}

	/* Execute SQL statement */
	char *zErrMsg = 0;
	int r = sqlite3_exec(sqldb, sql_table_create, NULL, 0, &zErrMsg);
	if (r != SQLITE_OK) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Annotations SQL error: %s", zErrMsg);
		sqlite3_free(zErrMsg);
		return false;
	}

	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "%s", "Annotations table created successfully");

	return true;
}

bool Annotations::Load() {
	return Open(false);
}

bool Annotations::Open(bool create) {
	std::string sqlfn                        = filename;
	auto pos                                 = sqlfn.rfind('.');
	if (pos != std::string::npos) sqlfn[pos] = '_';
	sqlfn += ".sqlite3";

	sqldb = nullptr;
	int flags = SQLITE_OPEN_READWRITE;

	if (create) {
		flags |= SQLITE_OPEN_CREATE;
	}

	int r = sqlite3_open_v2(sqlfn.c_str(), &sqldb, flags, NULL);

	if (r != SQLITE_OK) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Annotations database %s could not be opened: %s", sqlfn.c_str(), sqlite3_errmsg(sqldb));
		sqldb = nullptr;
		return false;
	}

	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Annotations database %s opened successfully", sqlfn.c_str());

	Init();
	GenerateList();

	return true;
}

bool Annotations::Close() {
	int r = sqlite3_close(sqldb);
	sqldb = nullptr;

	if (r != SQLITE_OK) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s", "Annotations database could not be closed");
		return false;
	}

	return true;
}

bool Annotations::GenerateList() {
	static const char sql[] = "SELECT id,side,posx,posy,net,part,pin,note FROM annotations WHERE visible=1;";

	sqlite3_stmt *stmt;
	int r = sqlite3_prepare_v2(sqldb, sql, -1, &stmt, NULL);
	if (r != SQLITE_OK) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Annotations SELECT statement could not be prepared: %s", sqlite3_errmsg(sqldb));
		return false;
	}

	annotations.clear();
	while ((r = sqlite3_step(stmt)) == SQLITE_ROW) {
		Annotation ann;
		ann.id      = sqlite3_column_int(stmt, 0);
		ann.side    = sqlite3_column_int(stmt, 1);
		ann.x       = sqlite3_column_int(stmt, 2);
		ann.y       = sqlite3_column_int(stmt, 3);
		ann.hovered = false;

		const char *p = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
		if (!p) p     = "";
		ann.net       = p;

		p         = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
		if (!p) p = "";
		ann.part  = p;

		p         = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
		if (!p) p = "";
		ann.pin   = p;

		p         = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 7));
		if (!p) p = "";
		ann.note  = p;

		annotations.push_back(ann);

		SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Annotations added: %d(%d:%f,%f) Net:%s Part:%s Pin:%s: Note:%s",
				ann.id, ann.side, ann.x, ann.y, ann.net.c_str(), ann.part.c_str(), ann.pin.c_str(), ann.note.c_str());
	}

	sqlite3_finalize(stmt);

	if (r != SQLITE_DONE) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Annotations SELECT statement failed: %s", sqlite3_errmsg(sqldb));
		// if you return/throw here, don't forget the finalize
		return false;
	}

	return true;
}

bool Annotations::Add(int side, double x, double y, const std::string &net, const std::string &part, const std::string &pin, const std::string &note) {
	static const char sql[] = "INSERT INTO annotations (visible, side, posx, posy, net, part, pin, note) VALUES (1, ?, ?, ?, ?, ?, ?, ?);";

	if (!sqldb  && !Open(true)) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s", "Annotations could not be added, database failed to open");
		return false;
	}

	sqlite3_stmt *stmt;
	int r = sqlite3_prepare_v2(sqldb, sql, -1, &stmt, NULL);
	if (r != SQLITE_OK) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Annotations INSERT statement could not be prepared: %s", sqlite3_errmsg(sqldb));
		return false;
	}

	r = sqlite3_bind_int(stmt, 1, side);

	if (r == SQLITE_OK) {
		r = sqlite3_bind_double(stmt, 2, x);
	}

	if (r == SQLITE_OK) {
		r = sqlite3_bind_double(stmt, 3, y);
	}

	if (r == SQLITE_OK) {
		r = sqlite3_bind_text(stmt, 4, net.c_str(), -1, SQLITE_TRANSIENT);
	}

	if (r == SQLITE_OK) {
		r = sqlite3_bind_text(stmt, 5, part.c_str(), -1, SQLITE_TRANSIENT);
	}

	if (r == SQLITE_OK) {
		r = sqlite3_bind_text(stmt, 6, pin.c_str(), -1, SQLITE_TRANSIENT);
	}

	if (r == SQLITE_OK) {
		r = sqlite3_bind_text(stmt, 7, note.c_str(), -1, SQLITE_TRANSIENT);
	}

	if (r != SQLITE_OK) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Annotations INSERT statement parameter could not be bound: %s", sqlite3_errmsg(sqldb));
		r = sqlite3_finalize(stmt);
		return false;
	}

	r = sqlite3_step(stmt);

	sqlite3_finalize(stmt);

	if (r != SQLITE_DONE && r != SQLITE_OK) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Annotations INSERT statement failed: %s", sqlite3_errmsg(sqldb));
	}

	return true;
}

bool Annotations::Remove(int id) {
	static const char sql[] = "UPDATE annotations SET visible = 0 WHERE id=?;";

	sqlite3_stmt *stmt;
	int r = sqlite3_prepare_v2(sqldb, sql, -1, &stmt, NULL);
	if (r != SQLITE_OK) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Annotations UPDATE statement could not be prepared: %s", sqlite3_errmsg(sqldb));
		return false;
	}

	r = sqlite3_bind_int(stmt, 1, id);

	if (r != SQLITE_OK) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Annotations UPDATE statement parameter could not be bound: %s", sqlite3_errmsg(sqldb));
		r = sqlite3_finalize(stmt);
		return false;
	}

	r = sqlite3_step(stmt);

	r = sqlite3_finalize(stmt);

	if (r != SQLITE_DONE && r != SQLITE_OK) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Annotations UPDATE statement failed: %s", sqlite3_errmsg(sqldb));
		return false;
	}

	return true;
}

bool Annotations::Update(int id, const std::string &note) {
	static const char sql[] = "UPDATE annotations SET note = ? WHERE id=?;";

	sqlite3_stmt *stmt;
	int r = sqlite3_prepare_v2(sqldb, sql, -1, &stmt, NULL);
	if (r != SQLITE_OK) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Annotations UPDATE statement could not be prepared: %s", sqlite3_errmsg(sqldb));
		return false;
	}

	r = sqlite3_bind_int(stmt, 2, id);

	if (r == SQLITE_OK) {
		r = sqlite3_bind_text(stmt, 1, note.c_str(), -1, SQLITE_TRANSIENT);
	}

	if (r != SQLITE_OK) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Annotations UPDATE statement parameter could not be bound: %s", sqlite3_errmsg(sqldb));
		r = sqlite3_finalize(stmt);
		return false;
	}

	r = sqlite3_step(stmt);

	r = sqlite3_finalize(stmt);

	if (r != SQLITE_DONE && r != SQLITE_OK) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Annotations UPDATE statement failed: %s %d", sqlite3_errmsg(sqldb), r);
		return false;
	}

	return true;
}
