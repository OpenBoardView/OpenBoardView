#include "BVFile.h"

#include "utils.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <unordered_map>

/*
 * Minimal read-only Jet 4 reader, just enough to walk the rows of a table.
 *
 * Layout notes (after the mdbtools HACKING document):
 *  - the file is a sequence of 4096-byte pages, the first byte is the page type
 *    (0x01 data, 0x02 table definition);
 *  - page 2 is the definition of MSysObjects, the catalog, whose rows give the
 *    definition page of every other table;
 *  - a data page stores the page of its owning table definition at offset 4, a
 *    row count at offset 12 and then one 16-bit offset per row. Rows are packed
 *    from the end of the page, so a row ends where the previous one starts;
 *  - a row is: column count, fixed-size columns, variable-size columns, a table
 *    of variable column offsets (stored backwards) and a null bitmask.
 *
 * Data pages are found by scanning the whole file rather than through the usage
 * maps: board files are small and this keeps the reader short.
 */
namespace {

const size_t kPageSize      = 4096;
const uint8_t kPageData     = 0x01;
const uint8_t kPageTableDef = 0x02;

const uint16_t kRowDeleted  = 0x8000;
const uint16_t kRowOverflow = 0x4000;
const uint16_t kRowOffset   = 0x1fff;

enum JetType : uint8_t {
	kJetByte   = 2,
	kJetInt16  = 3,
	kJetInt32  = 4,
	kJetFloat  = 6,
	kJetDouble = 7,
	kJetText   = 10,
};

struct JetColumn {
	std::string name;
	uint8_t type          = 0;
	uint16_t number       = 0; // position in the null bitmask
	uint16_t var_index    = 0; // position in the variable offset table
	uint16_t fixed_offset = 0;
	uint16_t size         = 0;
	bool fixed            = false;
};

struct JetField {
	const uint8_t *data = nullptr; // nullptr when the field is NULL or absent
	size_t size         = 0;
};

typedef std::vector<JetField> JetRow; // indexed like the column vector

uint16_t get_u16(const uint8_t *p) {
	return p[0] | (p[1] << 8);
}

uint32_t get_u32(const uint8_t *p) {
	return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24);
}

void append_utf8(std::string &out, uint32_t cp) {
	if (cp < 0x80) {
		out += (char)cp;
	} else if (cp < 0x800) {
		out += (char)(0xC0 | (cp >> 6));
		out += (char)(0x80 | (cp & 0x3F));
	} else if (cp < 0x10000) {
		out += (char)(0xE0 | (cp >> 12));
		out += (char)(0x80 | ((cp >> 6) & 0x3F));
		out += (char)(0x80 | (cp & 0x3F));
	} else {
		out += (char)(0xF0 | (cp >> 18));
		out += (char)(0x80 | ((cp >> 12) & 0x3F));
		out += (char)(0x80 | ((cp >> 6) & 0x3F));
		out += (char)(0x80 | (cp & 0x3F));
	}
}

// Jet 4 text is UCS-2LE. A leading FF FE marks "unicode compression": one byte
// per character, with a 0x00 byte switching to and from plain UCS-2.
std::string jet_text(const JetField &f) {
	std::vector<uint16_t> units;
	const uint8_t *p = f.data;
	size_t n         = f.size;
	if (!p) return std::string();

	if (n >= 2 && p[0] == 0xFF && p[1] == 0xFE) {
		bool compressed = true;
		size_t i        = 2;
		while (i < n) {
			if (p[i] == 0) {
				compressed = !compressed;
				i++;
			} else if (compressed) {
				units.push_back(p[i]);
				i++;
			} else if (i + 1 < n) {
				units.push_back(get_u16(p + i));
				i += 2;
			} else {
				break;
			}
		}
	} else {
		for (size_t i = 0; i + 1 < n; i += 2) units.push_back(get_u16(p + i));
	}

	std::string out;
	for (size_t i = 0; i < units.size(); i++) {
		uint32_t cp = units[i];
		if (cp == 0) break;
		if (cp >= 0xD800 && cp < 0xDC00 && i + 1 < units.size() && units[i + 1] >= 0xDC00 && units[i + 1] < 0xE000) {
			cp = 0x10000 + ((cp - 0xD800) << 10) + (units[i + 1] - 0xDC00);
			i++;
		}
		append_utf8(out, cp);
	}
	return out;
}

double jet_number(const JetColumn &col, const JetField &f, double fallback = 0.0) {
	if (!f.data) return fallback;
	switch (col.type) {
		case kJetByte:
			if (f.size >= 1) return f.data[0];
			break;
		case kJetInt16:
			if (f.size >= 2) return (int16_t)get_u16(f.data);
			break;
		case kJetInt32:
			if (f.size >= 4) return (int32_t)get_u32(f.data);
			break;
		case kJetFloat:
			if (f.size >= 4) {
				float v;
				memcpy(&v, f.data, sizeof(v));
				return v;
			}
			break;
		case kJetDouble:
			if (f.size >= 8) {
				double v;
				memcpy(&v, f.data, sizeof(v));
				return v;
			}
			break;
		case kJetText: {
			std::string s = jet_text(f);
			return strtod(s.c_str(), nullptr);
		}
		default: break;
	}
	return fallback;
}

class JetDB {
  public:
	JetDB(const std::vector<char> &buf)
	    : data((const uint8_t *)buf.data())
	    , num_pages(buf.size() / kPageSize) {}

	// Row count recorded in the table definition
	uint32_t declaredRows(uint32_t tdef_page) {
		const uint8_t *p = page(tdef_page);
		return p && p[0] == kPageTableDef ? get_u32(p + 16) : 0;
	}

	// Page of the table definition, 0 if there is no such table
	uint32_t findTable(const std::string &name) {
		uint32_t found = 0;
		std::vector<JetColumn> cols;
		if (!readColumns(2, cols)) return 0;
		int id = column(cols, "Id"), nm = column(cols, "Name"), ty = column(cols, "Type");
		if (id < 0 || nm < 0 || ty < 0) return 0;
		readRows(2, cols, [&](const JetRow &row) {
			if (found || jet_number(cols[ty], row[ty], -1) != 1) return; // 1: local table
			if (compare_string_insensitive(jet_text(row[nm]), name)) found = (uint32_t)jet_number(cols[id], row[id]) & 0x00FFFFFF;
		});
		return found;
	}

	static int column(const std::vector<JetColumn> &cols, const std::string &name) {
		for (size_t i = 0; i < cols.size(); i++)
			if (compare_string_insensitive(cols[i].name, name)) return (int)i;
		return -1;
	}

	bool readColumns(uint32_t tdef_page, std::vector<JetColumn> &cols) {
		// A definition may span several pages chained through offset 4
		std::vector<uint8_t> def;
		uint32_t pg = tdef_page;
		for (size_t hops = 0; pg != 0; hops++) {
			const uint8_t *p = page(pg);
			if (!p || p[0] != kPageTableDef || hops > num_pages) return false;
			size_t skip = def.empty() ? 0 : 8;
			def.insert(def.end(), p + skip, p + kPageSize);
			pg = get_u32(p + 4);
		}

		if (def.size() < 63) return false;
		size_t num_cols     = get_u16(&def[45]);
		size_t num_real_idx = get_u32(&def[51]);
		if (num_real_idx > def.size()) return false;
		size_t pos = 63 + num_real_idx * 12;
		if (pos + num_cols * 25 > def.size()) return false;

		cols.assign(num_cols, JetColumn());
		for (size_t i = 0; i < num_cols; i++, pos += 25) {
			const uint8_t *c     = &def[pos];
			cols[i].type         = c[0];
			cols[i].number       = get_u16(c + 5);
			cols[i].var_index    = get_u16(c + 7);
			cols[i].fixed        = c[15] & 0x01;
			cols[i].fixed_offset = get_u16(c + 21);
			cols[i].size         = get_u16(c + 23);
		}
		for (size_t i = 0; i < num_cols; i++) {
			if (pos + 2 > def.size()) return false;
			size_t len = get_u16(&def[pos]);
			pos += 2;
			if (pos + len > def.size()) return false;
			JetField f;
			f.data       = &def[pos];
			f.size       = len;
			cols[i].name = jet_text(f);
			pos += len;
		}
		// Rows are decoded in bitmask order
		std::sort(cols.begin(), cols.end(), [](const JetColumn &a, const JetColumn &b) { return a.number < b.number; });
		return true;
	}

	void readRows(uint32_t tdef_page, const std::vector<JetColumn> &cols, const std::function<void(const JetRow &)> &visit) {
		bool has_var = false;
		for (auto &c : cols) has_var |= !c.fixed;

		JetRow row;
		for (size_t pn = 1; pn < num_pages; pn++) {
			const uint8_t *p = page(pn);
			if (p[0] != kPageData || get_u32(p + 4) != tdef_page) continue;

			size_t num_rows = get_u16(p + 12);
			if (14 + num_rows * 2 > kPageSize) continue;

			for (size_t r = 0; r < num_rows; r++) {
				uint16_t entry = get_u16(p + 14 + r * 2);
				if (entry & kRowDeleted) continue;
				if (entry & kRowOverflow) { // row moved to another page, not seen in board files
					overflow_rows++;
					continue;
				}
				size_t start = entry & kRowOffset;
				size_t end   = r == 0 ? kPageSize : (get_u16(p + 14 + (r - 1) * 2) & kRowOffset);
				if (start >= end || end > kPageSize) continue;

				if (decodeRow(p + start, end - start, cols, has_var, row)) visit(row);
			}
		}
	}

	unsigned int overflow_rows = 0;

  private:
	const uint8_t *data;
	size_t num_pages;

	const uint8_t *page(size_t n) {
		return n < num_pages ? data + n * kPageSize : nullptr;
	}

	bool decodeRow(const uint8_t *r, size_t len, const std::vector<JetColumn> &cols, bool has_var, JetRow &row) {
		if (len < 2) return false;
		size_t row_cols = get_u16(r);
		size_t mask_len = (row_cols + 7) / 8;
		if (len < 2 + mask_len) return false;
		const uint8_t *mask = r + len - mask_len;

		// Variable column offsets sit just before the bitmask, last column first
		size_t row_var_cols     = 0;
		const uint8_t *var_offs = nullptr;
		if (has_var) {
			if (len < 2 + mask_len + 2) return false;
			row_var_cols = get_u16(r + len - mask_len - 2);
			if (row_var_cols > row_cols || len < 2 + mask_len + 2 + (row_var_cols + 1) * 2) return false;
			var_offs = r + len - mask_len - 4;
		}
		size_t row_fixed_cols = row_cols - row_var_cols;

		row.assign(cols.size(), JetField());
		size_t fixed_found = 0;
		for (size_t i = 0; i < cols.size(); i++) {
			const JetColumn &c = cols[i];
			bool is_null       = c.number / 8 >= mask_len || !(mask[c.number / 8] & (1 << (c.number % 8)));

			if (c.fixed) {
				if (fixed_found >= row_fixed_cols) continue; // column added after this row was written
				fixed_found++;
				size_t start = 2 + (size_t)c.fixed_offset;
				if (is_null || start + c.size > len) continue;
				row[i].data = r + start;
				row[i].size = c.size;
			} else {
				if (c.var_index >= row_var_cols || is_null) continue;
				size_t start = get_u16(var_offs - c.var_index * 2);
				size_t end   = get_u16(var_offs - (c.var_index + 1) * 2);
				if (start > end || end > len) continue;
				row[i].data = r + start;
				row[i].size = end - start;
			}
		}
		return true;
	}
};

struct BVPin {
	std::string name;
	std::string net;
	int number = 0;
	double x   = 0;
	double y   = 0;
	bool top   = false;
	int layer  = -1;
};

struct BVPart {
	std::string name;
	std::vector<BVPin> pins;
};

// The tables store inches, OBV uses mils
int to_mil(double inches) {
	return (int)lround(inches * 1000.0);
}

} // namespace

const char *BVFile::keep(const std::string &s) {
	strings.push_back(s);
	return strings.back().c_str();
}

bool BVFile::verifyFormat(std::vector<char> &buf) {
	static const char jet[] = "Standard Jet DB";
	static const char ace[] = "Standard ACE DB";
	if (buf.size() < 0x18) return false;
	return !memcmp(&buf[4], jet, sizeof(jet)) || !memcmp(&buf[4], ace, sizeof(ace));
}

BVFile::BVFile(std::vector<char> &buf) {
	ENSURE_OR_FAIL(buf.size() >= 3 * kPageSize, error_msg, return);

	// 0 is Jet 3 (Access 97): 2048-byte pages and a different row layout
	uint32_t version = get_u32((const uint8_t *)&buf[0x14]);
	if (version == 0) {
		error_msg = "This .bv file is a Jet 3 (Access 97) database, only Jet 4 and later are supported.";
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "BVFile: %s", error_msg.c_str());
		return;
	}

	JetDB db(buf);
	uint32_t pin_table    = db.findTable("Pin");
	uint32_t nail_table   = db.findTable("Nail");
	uint32_t layout_table = db.findTable("Layout");
	if (!pin_table) {
		error_msg = "No Pin table found, this database is not a boardview file (or it is encrypted).";
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "BVFile: %s", error_msg.c_str());
		return;
	}

	std::vector<JetColumn> cols;

	// Board outline
	if (layout_table && db.readColumns(layout_table, cols)) {
		int cx = JetDB::column(cols, "X"), cy = JetDB::column(cols, "Y"), cgroup = JetDB::column(cols, "Group");
		if (cx >= 0 && cy >= 0) {
			std::vector<std::pair<int, BRDPoint>> points; // group, point
			bool single_group = true;
			db.readRows(layout_table, cols, [&](const JetRow &row) {
				if (!row[cx].data || !row[cy].data) return;
				int group = cgroup >= 0 ? (int)jet_number(cols[cgroup], row[cgroup]) : 0;
				if (!points.empty() && points.back().first != group) single_group = false;
				points.push_back({group, BRDPoint(to_mil(jet_number(cols[cx], row[cx])), to_mil(jet_number(cols[cy], row[cy])))});
			});
			if (single_group) {
				for (auto &p : points) format.push_back(p.second);
			} else {
				// Several polylines: do not join the end of one to the start of the next
				for (size_t i = 1; i < points.size(); i++)
					if (points[i].first == points[i - 1].first)
						outline_segments.push_back({points[i - 1].second, points[i].second});
			}
		}
	}

	// Parts and pins, one row per pin. Keep the parts in file order but do not
	// rely on the pins of a part being stored together.
	std::vector<BVPart> bvparts;
	ENSURE_OR_FAIL(db.readColumns(pin_table, cols), error_msg, return);
	{
		int cpart = JetDB::column(cols, "Part"), ctb = JetDB::column(cols, "TB"), cnum = JetDB::column(cols, "Pin");
		int cname = JetDB::column(cols, "Name"), cx = JetDB::column(cols, "X"), cy = JetDB::column(cols, "Y");
		int clayer = JetDB::column(cols, "Layer"), cnet = JetDB::column(cols, "Net");
		if (cpart < 0 || cx < 0 || cy < 0) {
			error_msg = "The Pin table has no Part, X or Y column.";
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "BVFile: %s", error_msg.c_str());
			return;
		}

		std::unordered_map<std::string, size_t> part_index;
		db.readRows(pin_table, cols, [&](const JetRow &row) {
			if (!row[cpart].data || !row[cx].data || !row[cy].data) return;
			std::string part_name = jet_text(row[cpart]);
			auto it               = part_index.find(part_name);
			if (it == part_index.end()) {
				it = part_index.insert({part_name, bvparts.size()}).first;
				bvparts.push_back(BVPart());
				bvparts.back().name = part_name;
			}

			BVPin pin;
			pin.x = jet_number(cols[cx], row[cx]);
			pin.y = jet_number(cols[cy], row[cy]);
			if (cnum >= 0) pin.number = (int)jet_number(cols[cnum], row[cnum]);
			if (cname >= 0) pin.name = jet_text(row[cname]);
			if (cnet >= 0) pin.net = jet_text(row[cnet]);
			if (ctb >= 0) pin.top = jet_text(row[ctb]) == "(T)";
			if (clayer >= 0) pin.layer = (int)jet_number(cols[clayer], row[clayer], -1);
			bvparts[it->second].pins.push_back(pin);
		});
	}

	for (auto &bvpart : bvparts) {
		BRDPart part;
		part.name          = keep(bvpart.name);
		part.part_type     = BRDPartType::SMD;
		part.mounting_side = bvpart.pins.front().top ? BRDPartMountingSide::Top : BRDPartMountingSide::Bottom;
		parts.push_back(part);

		for (auto &bvpin : bvpart.pins) {
			BRDPin pin;
			pin.part  = parts.size();
			pin.pos.x = to_mil(bvpin.x);
			pin.pos.y = to_mil(bvpin.y);
			if (bvpin.number > 0) pin.snum = keep(std::to_string(bvpin.number));
			if (!bvpin.name.empty()) pin.name = keep(bvpin.name);
			if (!bvpin.net.empty()) pin.net = keep(bvpin.net);
			// Layer 1 is top, 2 is bottom and 0 a through-hole pin present on both
			if (bvpin.layer == 0)
				pin.side = BRDPinSide::Both;
			else
				pin.side = bvpin.top ? BRDPinSide::Top : BRDPinSide::Bottom;
			pins.push_back(pin);
		}
		parts.back().end_of_pins = pins.size();
	}

	// Test points
	if (nail_table && db.readColumns(nail_table, cols)) {
		int cx = JetDB::column(cols, "X"), cy = JetDB::column(cols, "Y"), ctb = JetDB::column(cols, "TB");
		int cnet = JetDB::column(cols, "NetName");
		if (cx >= 0 && cy >= 0) {
			db.readRows(nail_table, cols, [&](const JetRow &row) {
				if (!row[cx].data || !row[cy].data) return;
				BRDNail nail;
				nail.pos.x = to_mil(jet_number(cols[cx], row[cx]));
				nail.pos.y = to_mil(jet_number(cols[cy], row[cy]));
				nail.side  = (ctb >= 0 && jet_text(row[ctb]) == "(T)") ? BRDPartMountingSide::Top : BRDPartMountingSide::Bottom;
				if (cnet >= 0) {
					std::string net = jet_text(row[cnet]);
					if (!net.empty()) nail.net = keep(net);
				}
				nails.push_back(nail);
			});
		}
	}

	if (db.overflow_rows)
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "BVFile: skipped %u rows stored on overflow pages", db.overflow_rows);
	if (pins.size() < db.declaredRows(pin_table))
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
		            "BVFile: read %u of %u pins, the file may be truncated or damaged",
		            (unsigned int)pins.size(),
		            (unsigned int)db.declaredRows(pin_table));

	num_parts  = parts.size();
	num_pins   = pins.size();
	num_format = format.size();
	num_nails  = nails.size();

	if (pins.empty()) {
		error_msg = "The Pin table is empty.";
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "BVFile: %s", error_msg.c_str());
		return;
	}

	valid = true;
}
