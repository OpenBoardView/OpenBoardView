#include "BRDFile.h"

#include "BRDBoard.h"
#include "json11\json11.hpp"
#include "platform.h"

#include <vector>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <cerrno>

using namespace std;
using namespace json11;

BRDBoard::BRDBoard(const BRDFile* boardFile) 
	: m_file(boardFile)
{
	m_parts.assign(m_file->parts, m_file->parts + m_file->num_parts);
	m_pins.assign(m_file->pins, m_file->pins + m_file->num_pins);
	m_nails.assign(m_file->nails, m_file->nails + m_file->num_nails);
    m_points.assign(m_file->format, m_file->format + m_file->num_format);

	// sort NET names alphabetically and make vec unique
	m_netUnique = m_nails;
	sort(m_netUnique.begin(), m_netUnique.end(), &cmpAlphabetically);
	m_netUnique.erase(
		unique(m_netUnique.begin(), m_netUnique.end(), &namesEqual),
		m_netUnique.end());

    if (FetchPartAnnotations()) {
        // 
    }
}

BRDBoard::~BRDBoard()
{
}

//TODO: error handling
bool BRDBoard::FetchPartAnnotations() {
    const char* fname = "annotations.json";

    // Read annotation source file.
    ifstream json_file;
    stringstream file_buf;
    json_file.exceptions(ifstream::failbit | ifstream::badbit);
    try {
        json_file.open(fname);
        file_buf << json_file.rdbuf();
        json_file.close();
    }
    catch (ifstream::failure e) {
        return false;
    }

    // Parse JSON.
    string json_str = file_buf.str();
    string error;
    Json json = Json::parse(json_str, error);

    if (error.length() > 0)
        return false;

    map<string,string> annotations;
    for (auto part : json["part_annotations"].array_items()) {
        annotations[part["part"].string_value()] = part["name"].string_value();
    }

    // Set / apply annotations.
    #pragma warning(disable : 4996)
    for (auto& part : m_parts) {
        string part_name(part.name);
        if (annotations.count(part_name)) {
            delete[] part.annotation;
            char *buf = new char[annotations[part_name].length()];
            strcpy(buf, annotations[part_name].c_str());
            part.annotation = buf;
        }
    }

    return true;
}

std::vector<BRDPart> BRDBoard::Parts()
{	
	return m_parts;
}

std::vector<BRDPin> BRDBoard::Pins()
{
	return m_pins;
}

std::vector<BRDNail> BRDBoard::Nails()
{
	return m_nails;
}

std::vector<BRDPoint> BRDBoard::OutlinePoints()
{
    return m_points;
}

std::vector<BRDNail> BRDBoard::UniqueNetNails()
{
	return m_netUnique;
}

EBoardType BRDBoard::BoardType()
{
	return BRD;
}

