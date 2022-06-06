#include "ASCFile.h"

#include "utils.h"
#include <cstring>
#include <cctype>
#include <clocale>
#include <cstdint>

/*bool ASCFile::verifyFormat(std::vector<char> &buf) {
    return find_str_in_buf("dd:1.3?,r?-=bb", buf) || ( find_str_in_buf("<<format.asc>>", buf) && find_str_in_buf("<<pins.asc>>",
buf) );
}*/

void ASCFile::parse_format(char *&p, char *&s, char *&arena, char *&arena_end, line_iterator_t &line_it) {
	if (m_firstformat) {
		line_it += 7; // Skip 7+1 unused lines before 1st point. Might not work with all files.
		m_firstformat = false;
		return; // lines++ in while loop
	}
	BRDPoint point;

	double x = READ_DOUBLE();
	point.x  = x * 1000.0f; // OBV uses integers
	double y = READ_DOUBLE();
	point.y  = y * 1000.0f;
	format.push_back(point);
}

void ASCFile::parse_pin(char *&p, char *&s, char *&arena, char *&arena_end, line_iterator_t &line_it) {
	if (m_firstpin) {
		line_it += 7; // Skip 7+1 unused lines before 1st part
		m_firstpin = false;
		return;
	}
	if (!strncmp(p, "Part", 4)) {
		p += 4; // Skip "Part" string
		BRDPart part;

		part.name      = READ_STR();
		part.part_type = BRDPartType::SMD;
		char *loc      = READ_STR();
		if (!strcmp(loc, "(T)"))
			part.mounting_side = BRDPartMountingSide::Top; // SMD part on top
		else
			part.mounting_side = BRDPartMountingSide::Bottom; // SMD part on bottom
		part.end_of_pins       = 0;
		parts.push_back(part);
	} else {
		BRDPin pin;

		pin.part = parts.size();
		/*int id =*/READ_INT(); // uint
		/*char *name =*/READ_STR();
		double posx = READ_DOUBLE();
		pin.pos.x   = posx * 1000.0f;
		double posy = READ_DOUBLE();
		pin.pos.y   = posy * 1000.0f;
		/*int layer =*/READ_INT(); // uint
		pin.net   = READ_STR();
		pin.probe = READ_UINT();
		switch (parts.back().mounting_side) {
			case BRDPartMountingSide::Top:    pin.side = BRDPinSide::Top;    break;
			case BRDPartMountingSide::Bottom: pin.side = BRDPinSide::Bottom; break;
			case BRDPartMountingSide::Both:   pin.side = BRDPinSide::Both;   break;
		}
		pins.push_back(pin);
		parts.back().end_of_pins = pins.size();
	}
}

void ASCFile::parse_nail(char *&p, char *&s, char *&arena, char *&arena_end, line_iterator_t &line_it) {
	if (m_firstnail) {
		line_it += 6; // Skip 6+1 unused lines before 1st nail
		m_firstnail = false;
		return;
	}
	BRDNail nail;
	p++; // Skip first char

	nail.probe  = READ_UINT();
	double posx = READ_DOUBLE();
	nail.pos.x  = posx * 1000.0f;
	double posy = READ_DOUBLE();
	nail.pos.y  = posy * 1000.0f;
	/*int type =*/READ_INT(); // uint
	/*char *grid =*/READ_STR();
	char *loc = READ_STR();
	if (!strcmp(loc, "(T)"))
		nail.side = BRDPartMountingSide::Top;
	else
		nail.side = BRDPartMountingSide::Bottom;
	/*char *netid =*/READ_STR(); // uint
	nail.net = READ_STR();
	nails.push_back(nail);
}

/*
 * Read one of the *.asc file
 * pins.asc, parts.asc (not supported), nets.asc (not supported), nails.asc, format.asc
 * *.bom files not supported either
 */
bool ASCFile::read_asc(const filesystem::path &filepath, void (ASCFile::*parser)(char *&, char *&, char *&, char *&, line_iterator_t&)) {
	if (filepath.empty()) return false;
	std::vector<char> buf = file_as_buffer(filepath, error_msg);
	if (buf.empty()) return false;

	ENSURE_OR_FAIL(buf.size() > 4, error_msg, return false);
	size_t file_buf_size = 3 * (1 + buf.size());
	file_buf             = (char *)calloc(1, file_buf_size);
	ENSURE_OR_FAIL(file_buf != nullptr, error_msg, return false);

	std::copy(buf.begin(), buf.end(), file_buf);
	file_buf[buf.size()] = 0;
	// This is for fixing degenerate utf8
	char *arena     = &file_buf[buf.size() + 1];
	char *arena_end = file_buf + file_buf_size - 1;
	*arena_end      = 0;

	std::vector<char *> lines;
	stringfile(file_buf, lines);

	std::vector<char *>::iterator line_it = lines.begin();
	while (line_it != lines.end()) {
		char *line = *line_it;
		++line_it;

		while (isspace((uint8_t)*line)) line++;
		if (!line[0]) continue;

		char *p = line;
		char *s = nullptr;

		(this->*parser)(p, s, arena, arena_end, line_it);
	}
	return true;
}

/*
 * Updates element counts
 */
void ASCFile::update_counts() {
	num_parts  = parts.size();
	num_pins   = pins.size();
	num_format = format.size();
	num_nails  = nails.size();
}

bool ASCFile::load_and_parse(const filesystem::path &path, const std::string &filename, void (ASCFile::*parser)(char *&, char *&, char *&, char *&, line_iterator_t&)) {
	auto filepath = lookup_file_insensitive(path, filename, error_msg);
	if (filepath.empty() || !error_msg.empty()) {
		return false;
	}
	return read_asc(filepath, parser);
}

/*
 * buf unused for now, read all files even if one of the supported *.asc was
 * passed
 */
ASCFile::ASCFile(std::vector<char> &buf, const filesystem::path &filepath) {
	std::error_code ec;
	auto directory = filesystem::weakly_canonical(filepath, ec);
	if (ec) {
		error_msg = ec.message();
		return;
	}
	directory = directory.parent_path();

	char *saved_locale = setlocale(LC_NUMERIC, "C"); // Use '.' as delimiter for strtod

	if (!load_and_parse(directory, "format.asc", &ASCFile::parse_format)
		|| !load_and_parse(directory, "pins.asc", &ASCFile::parse_pin)
		|| !load_and_parse(directory, "nails.asc", &ASCFile::parse_nail)) {
		valid = false;
	} else {
		valid = true;
	}

	update_counts();
	setlocale(LC_NUMERIC, saved_locale); // Restore locale
}
