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

//using namespace json11;
using namespace std;

BRDBoard::BRDBoard(const BRDFile* boardFile) 
	: m_file(boardFile)
{
	m_parts.assign(m_file->parts, m_file->parts + m_file->num_parts);
	m_pins.assign(m_file->pins, m_file->pins + m_file->num_pins);
	m_nails.assign(m_file->nails, m_file->nails + m_file->num_nails);
    m_points.assign(m_file->format, m_file->format + m_file->num_format);

	// sort NET names alphabetically and make vec unique
	m_netUnique = m_nails;
	std::sort(m_netUnique.begin(), m_netUnique.end(), &cmpAlphabetically);
	m_netUnique.erase(
		std::unique(m_netUnique.begin(), m_netUnique.end(), &equalNames),
		m_netUnique.end());

    if (FetchPartAnnotations()) {
        
    }
}

BRDBoard::~BRDBoard()
{
}

bool BRDBoard::FetchPartAnnotations() {
    
    std::string error;

    size_t buffer_size;
    char *buffer = file_as_buffer(&buffer_size, "annotations.json");
    if (buffer) {
        buffer[buffer_size] = '\0';

        json11::Json json = json11::Json::parse((const char*)buffer, error);

        std::map<std::string, std::string> annotations;
        for (auto part : json.object_items().at("part_annotations").array_items()) {
            string pname = part.object_items().at("part").string_value();
            string annot = part.object_items().at("name").string_value();

            annotations.insert(std::pair<std::string,std::string>(pname, annot));
        }

        //TODO: error handling

        #pragma warning(disable : 4996)
        for (auto& part : m_parts) {
            string part_name(part.name);
            if (annotations.count(part_name)) {
                // FIXME: C++ify
                char *buf = new char[MAX_COMP_NAME_LENGTH];
                strcpy(buf, annotations.at(part_name).c_str());
                part.annotation = buf;
            }
            // FIXME: free bufs
        }
    }

    return false;
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

