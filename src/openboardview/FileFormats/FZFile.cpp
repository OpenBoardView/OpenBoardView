#include "FZFile.h"
#include "utils.h"

#include <algorithm>
#include <cctype>
#include <clocale>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <zlib.h>

// Decoding an .fz file. You still need the key of course.
// https://en.wikipedia.org/wiki/RC6 here you can read it all up.

// Looking at the paper the algo is straight forward, not sure about endianness.
// This will put out some .bin file you would decompress using zlib.

static inline uint32_t rotl32(uint32_t a, int32_t b) {
	return (a << b) | (a >> (32 - b));
}

template<size_t N>
std::string FZFile::fz_key_to_string(const std::array<uint32_t, N> &fzkey) {
	std::stringstream sstr;
	for (size_t i = 0; i < N; i += 4) {
		for (size_t j = 0; j < 4; j++) {
			sstr << " 0x" << std::setfill ('0') << std::setw(8) << std::hex << fzkey[i + j];
		}
		sstr << std::endl;
	}
	return sstr.str();
}

template<size_t N>
bool FZFile::check_fz_key(const std::array<uint32_t, N> &fzkey) const {
	auto key_parity = getKeyParity();
	bool valid_key = true;
	for (size_t i = 0; i < N; i++) { // Compute parity for each 32-bit word of FZ key
		uint32_t tmp = fzkey[i];
		tmp ^= tmp >> 16;
		tmp ^= tmp >> 8;
		tmp ^= tmp >> 4;
		tmp ^= tmp >> 2;
		tmp ^= tmp >> 1;
		tmp = (~tmp) & 1;
		valid_key = valid_key && (tmp == key_parity[i]);
	}
	return valid_key;
}

const std::array<uint32_t, 44> FZFile::getKeyParity() const {
	return {{0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1}};
}

const std::string FZFile::getKeyErrorMsg() const {
	return "Invalid FZ key\nFZ Key:\n";
}

/*
 * Decrypt an RC6 encrypted buffer using key
 */
void FZFile::decode(char *source, size_t size) const {
	// Along the lines of http://people.csail.mit.edu/rivest/pubs/RRSY98.pdf
	// (page 3, 2.2)
	int32_t logw = 5;
	uint32_t r   = 20;

	uint32_t A = 0;
	uint32_t B = 0;
	uint32_t C = 0;
	uint32_t D = 0;

	uint8_t currentByte;
	uint8_t ibuf[16] = {0}; // you'll see

	// RC6 algo from the paper, basically 1:1
	for (size_t pos = 0; pos < size; ++pos) {
		B = B + key[0];
		D = D + key[1];
		for (uint32_t i = 1; i < (r + 1); ++i) { // loop offset by 1
			uint32_t t = rotl32(B * (2 * B + 1), logw);
			uint32_t u = rotl32(D * (2 * D + 1), logw);
			A          = rotl32(A ^ t, u) + key[2 * i];
			C          = rotl32(C ^ u, t) + key[2 * i + 1];

			uint32_t tmp = A;
			A            = B;
			B            = C;
			C            = D;
			D            = tmp;
		}
		A = A + key[2 * r + 2];
		C = C + key[2 * r + 3]; // not used I guess

		// rolling over the the uint8_t string
		// buf[pos] xor A -> is our resulting byte
		currentByte = source[pos];
		source[pos] = ((uint8_t)(currentByte ^ (A & 0xFF)));
		// fprintf(stdout,"%c",source[pos]);

		// pushing in a stream of buf[pos] chars 'from the right'
		// and shift whole array 8 bits to the left
		for (uint32_t i = 0; i < 15; ++i) {
			ibuf[i] = ibuf[i + 1];
		}
		ibuf[15] = currentByte;

		// align 4 consequent int32s to that buffer
		// (A, B, C, D) = (buf[0], buf[1], buf[2], buf[3])
		// byte order?!
		A = ibuf[0] | ibuf[1] << 8 | ibuf[2] << 16 | ibuf[3] << 24;
		B = ibuf[4] | ibuf[5] << 8 | ibuf[6] << 16 | ibuf[7] << 24;
		C = ibuf[8] | ibuf[9] << 8 | ibuf[10] << 16 | ibuf[11] << 24;
		D = ibuf[12] | ibuf[13] << 8 | ibuf[14] << 16 | ibuf[15] << 24;
	}
}

/*
 * Sets content_size to the length of the compressed content from the decoded fz
 * file
 */
char *FZFile::split(char *file_buf, size_t buffer_size, size_t &content_size, char *&descr, size_t &descr_size) {
	unsigned char *ufile_buf = reinterpret_cast<unsigned char *>(file_buf);
	auto len = (ufile_buf[buffer_size - 1] << 24) + (ufile_buf[buffer_size - 2] << 16) + (ufile_buf[buffer_size - 3] << 8) +
	           ufile_buf[buffer_size - 4]; // read last 4 bytes as little endian 32-bit int.
	if (len < 0 || static_cast<unsigned>(len) > buffer_size) return nullptr;
	descr_size   = static_cast<unsigned>(len);
	content_size = buffer_size - descr_size + 4;
	descr        = file_buf + content_size;
	return file_buf + 4;
}

/*
 * Inflates the zlib compressed data from buffer
 */
char *FZFile::decompress(char *file_buf, size_t buffer_size, size_t &output_size) {
	output_size = buffer_size;
	if (buffer_size == 0) return nullptr;

	char *output = (char *)calloc(output_size, sizeof(char));

	z_stream zst;
	zst.next_in   = (Bytef *)file_buf;
	zst.avail_in  = buffer_size;
	zst.total_out = 0;
	zst.zalloc    = Z_NULL;
	zst.zfree     = Z_NULL;

	if (inflateInit(&zst) != Z_OK) {
		free(output);
		return nullptr;
	}

	int ret;
	do {
		// If our output buffer is too small
		if (zst.total_out >= output_size) {
			// Increase size of output buffer
			char *buf = (char *)calloc(output_size + buffer_size / 2, sizeof(char));
			memcpy(buf, output, output_size);
			output_size += buffer_size / 2;
			free(output);
			output = buf;
		}

		zst.next_out  = (Bytef *)(output + zst.total_out);
		zst.avail_out = output_size - zst.total_out;
	} while ((ret = inflate(&zst, Z_SYNC_FLUSH)) == Z_OK);

	if (ret != Z_STREAM_END) printf("Error %d: %s\n", ret, zst.msg);

	if (inflateEnd(&zst) != Z_OK) {
		free(output);
		return nullptr;
	}

	return output;
}

/*
 * Creates fake points for outline using outermost pins plus some margin.
 */
#define OUTLINE_MARGIN 20
void FZFile::gen_outline() {
	// Determine board outline
	int minx =
	    std::min_element(pins.begin(), pins.end(), [](BRDPin a, BRDPin b) { return a.pos.x < b.pos.x; })->pos.x - OUTLINE_MARGIN;
	int maxx =
	    std::max_element(pins.begin(), pins.end(), [](BRDPin a, BRDPin b) { return a.pos.x < b.pos.x; })->pos.x + OUTLINE_MARGIN;
	int miny =
	    std::min_element(pins.begin(), pins.end(), [](BRDPin a, BRDPin b) { return a.pos.y < b.pos.y; })->pos.y - OUTLINE_MARGIN;
	int maxy =
	    std::max_element(pins.begin(), pins.end(), [](BRDPin a, BRDPin b) { return a.pos.y < b.pos.y; })->pos.y + OUTLINE_MARGIN;
	format.push_back({minx, miny});
	format.push_back({maxx, miny});
	format.push_back({maxx, maxy});
	format.push_back({minx, maxy});
	format.push_back({minx, miny});
}
#undef OUTLINE_MARGIN

/*
 * Updates element counts
 */
void FZFile::update_counts() {
	num_parts  = parts.size();
	num_pins   = pins.size();
	num_format = format.size();
	num_nails  = nails.size();
}

void FZFile::parse(std::vector<char> &buf, const std::array<uint32_t, 44> &fzkey) {
	auto buffer_size = buf.size();
	char *saved_locale;
	float multiplier = 1.0f;
	saved_locale     = setlocale(LC_NUMERIC, "C"); // Use '.' as delimiter for strtod

	if (check_fz_key(fzkey)) {
		key = fzkey;
	} else if (!check_fz_key(key)) { // Try to fallback to built-in key
		valid = false;
		error_msg = getKeyErrorMsg();
		error_msg += fz_key_to_string(fzkey);
		return;
	}

	ENSURE_OR_FAIL(buffer_size > 4, error_msg, return);
	size_t file_buf_size = 3 * (1 + buffer_size);
	file_buf             = (char *)calloc(1, file_buf_size);
	ENSURE_OR_FAIL(file_buf != nullptr, error_msg, return);

	std::copy(buf.begin(), buf.end(), file_buf);
	file_buf[buffer_size] = 0;
	// This is for fixing degenerate utf8
	char *arena     = &file_buf[buffer_size + 1];
	char *arena_end = file_buf + file_buf_size - 1;
	*arena_end      = 0;

	/*
	 * Some non-encrypted, but zip-encoded files are popping up now and then.
	 *
	 * Thanks to piernov for noticing the starting byte sequence
	 *
	 * Attempt to decode using the decode() call and subsequently
	 * split the file to get the content.  If that fails, then try again
	 * without decoding.
	 */

	uint8_t s1 = file_buf[4];
	uint8_t s2 = file_buf[5];
	if (!((s1 == 0x78) && ((s2 == 0x9C) || (s2 == 0xDA)))) {

		/*
		 * Doesn't have the zlib signature, so decode it first.
		 *
		 * 1 in ~2^16 chance of a false hit.
		 */
		FZFile::decode(file_buf, buffer_size); // RC6 decryption
		                                       // fprintf(stderr,"FZFile:Decoded\n");
	}

	size_t content_size = 0;
	size_t descr_size   = 0;
	char *descr;
	char *content = FZFile::split(file_buf, buffer_size, content_size, descr, descr_size); // then split it

	/*
  if (!content) {
	 // Decryption must have failed, so try again now without decrypting the data
	std::copy(buf.begin(), buf.end(), file_buf);
	content = FZFile::split(file_buf, buffer_size, content_size, descr, descr_size); // then split it
  }
  */

	ENSURE_OR_FAIL(content != nullptr, error_msg, return);
	ENSURE_OR_FAIL(content_size > 0, error_msg, return);
	content = FZFile::decompress(content, content_size, content_size); // decompress zlib content data
	ENSURE_OR_FAIL(content != nullptr, error_msg, return);
	ENSURE_OR_FAIL(content_size > 0, error_msg, return);

	ENSURE_OR_FAIL(content != descr, error_msg, return);
	ENSURE_OR_FAIL(descr_size > 0, error_msg, return);
	descr = FZFile::decompress(descr, descr_size, descr_size);
	ENSURE_OR_FAIL(descr != nullptr, error_msg, return);
	ENSURE_OR_FAIL(descr_size > 0, error_msg, return);

	int current_block = 0;
	std::unordered_map<std::string, int> parts_id; // map between part name and part number

	std::vector<char *> lines_content;
	stringfile(content, lines_content);

	std::vector<char *> lines_descr;
	stringfile(descr, lines_descr);

	// For some reason, some boards have COMMAs as decimal separators. Will wonders ever cease ( I realise this is a regional thing
	// )?
	std::replace(content, content + content_size, ',', '.');

	// Parse the content part (parts, pins, nails)

	for (char *line : lines_content) {
		//	fprintf(stdout,"%s\n", line);

		while (isspace((uint8_t)*line)) line++;
		if (!line[0]) continue;

		char *p = line;
		char *s;

		/*
		 * If we have a UNIT: line in the data, see if it's requesting millimeters and scale appropriatley.
		 * Default is units are thou (0.001")
		 */
		if (!strcmp(line, "UNIT:millimeters")) {
			multiplier = 25.4f;
		}

		if (line[0] == 'A') { // New block
			line += 2;        // skip "A!"
			if (!strncmp(line, "REFDES", 6)) {
				current_block = 1;
			} else if (!strncmp(line, "NET_NAME", 8)) {
				current_block = 2;
			} else if (!strncmp(line, "TESTVIA", 7)) {
				current_block = 3;
			} else if (!strncmp(line, "GRAPHIC_DATA_NAME", 17)) {
				current_block = 4;
			} else if (!strncmp(line, "CLASS", 5)) {
				current_block = 5;
			} else if (!strncmp(line, "LOGOInfo", 8)) {
				current_block = 6;
			} else if (!strncmp(line, "UnDrawSym", 9)) {
				current_block = 7;
			} else {
				current_block = -1;
			}
			continue;
		} else if (line[0] != 'S') // Unknown line type
			continue;              // jump to next line
		else
			p += 2; // Skip "S!"

		switch (current_block) {
			case 1: { // Parts
				BRDPart part;
				part.name = READ_STR();
				/*char *cic =*/READ_STR();
				/*char *sname =*/READ_STR();
				char *smirror = READ_STR();
				/*char *srotate =*/READ_STR();
				part.part_type = BRDPartType::SMD;
				if (!strcmp(smirror, "YES"))
					part.mounting_side = BRDPartMountingSide::Top; // SMD part on top
				else
					part.mounting_side = BRDPartMountingSide::Bottom; // SMD part on bottom
				part.end_of_pins       = 0;
				parts.push_back(part);
				parts_id[part.name] = parts.size();
			} break;
			case 2: { // Pins
				/* There are more then single FZ file variant even if all those files ahre same header looking like
				A!NET_NAME!REFDES!PIN_NUMBER!PIN_NAME!PIN_X!PIN_Y!TEST_POINT!RADIUS!

				There are at least 2 variants, placing BGA pin label in different places:
				S!SATA_GP1!SU1!AJ43!GPP_E1/SATAXPCIE1/SATAGP1!3315.86!1830.70!!6!
				S!SNN_FBA_WCKB45*!G1!0!AJ31!3140.51!1436.167!!7.48106!

				For first variant the PIN_NAME column is ignored until thre would be a filed for handling/displaying such info
				But for second varinat the PIN_NUMBER column is always the "0" literal, so PIN_NAME is used as name.
				*/
				BRDPin pin;
				pin.net    = READ_STR();
				char *part = READ_STR();
				pin.part   = parts_id.at(part);
				pin.snum   = READ_STR();
				char *name = READ_STR();

				// use name field as pin name if snum is empty string or "0" (decimal zero as string)
				bool name_is_pin_position_id = strlen(pin.snum) <= 1 && (pin.snum[0] == '\0' || pin.snum[0] == '0');
				if (name_is_pin_position_id)
				{
					pin.name = name;
				}
				double posx   = READ_DOUBLE();
				pin.pos.x     = posx * multiplier;
				double posy   = READ_DOUBLE();
				pin.pos.y     = posy * multiplier;
				pin.probe     = READ_UINT();
				double radius = READ_DOUBLE();
				radius /= 100;
				if (radius < 0.5f) radius = 0.5f;
				pin.radius                = radius * multiplier;
				switch (parts[pin.part - 1].mounting_side) {
					case BRDPartMountingSide::Top:    pin.side = BRDPinSide::Top;    break;
					case BRDPartMountingSide::Bottom: pin.side = BRDPinSide::Bottom; break;
					case BRDPartMountingSide::Both:   pin.side = BRDPinSide::Both;   break;
				}
				pins.push_back(pin);
			} break;
			case 3: {   // Nails
				p += 2; // Skip "Y!"
				BRDNail nail;
				nail.net = READ_STR();
				/*char *refdes =*/READ_STR();
				/*int pinnumber =*/READ_INT(); // uint
				/*char *pinname =*/READ_STR();

				double posx = READ_DOUBLE();
				nail.pos.x  = posx * multiplier;
				double posy = READ_DOUBLE();
				nail.pos.y  = posy * multiplier;
				char *loc   = READ_STR();
				if (!strcmp(loc, "T"))
					nail.side = BRDPartMountingSide::Top;
				else
					nail.side = BRDPartMountingSide::Bottom;
				/*double radius =*/READ_DOUBLE();
				nails.push_back(nail);
			} break;
			case 4: { // Drawing
			} break;
			case 5: { // Unknown
			} break;
			case 6: { // Logo/Info
			} break;
			case 7: { // Unknown
			} break;
		}
	}

	// Parse the descr part (parts info)
	// Note: Discard first 2 lines (board description, currently unused and table columns name)
	for (size_t i = 2; i < lines_descr.size(); ++i) {
		char *line = lines_descr[i];

		while (isspace((uint8_t)*line)) line++;
		if (!line[0]) continue;

		char *p = line;
		char *s;

		if (line[0] == 's') continue; // PARTNUMBER starting with 's' seems unused

		FZPartDesc pdesc;
		pdesc.partno      = READ_DESCR_STR();
		pdesc.description = READ_DESCR_STR();
		pdesc.quantity    = READ_DESCR_UINT();
		pdesc.locations   = split_string(READ_DESCR_STR());
		pdesc.partno2     = READ_DESCR_STR();
		partsDesc.push_back(pdesc);
	}

	for (auto &pdesc : partsDesc) {
		for (auto &partname : pdesc.locations) {
			auto iter = parts_id.find(partname);
			if (iter != parts_id.end()) { // Sometimes the desc part references stuff not in content part, ignore it
				parts.at(iter->second - 1).mfgcode = pdesc.description;
			}
		}
	}

	//	std::sort(pins.begin(), pins.end()); // sort vector by part num then pin num
	for (std::vector<int>::size_type i = 0; i < pins.size(); i++) {
		// update end_of_pins field
		if (pins[i].part > 0) parts[pins[i].part - 1].end_of_pins = i;
	}

	gen_outline();

	update_counts();

	setlocale(LC_NUMERIC, saved_locale); // Restore locale

	valid = current_block != 0;

	if (!valid) {
		error_msg += "FZ Key:\n" + fz_key_to_string(key);
	}
}
