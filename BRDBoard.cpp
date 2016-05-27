#include "BRDFile.h"

#include "BRDBoard.h"
#include <vector>
#include <algorithm>


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
}

BRDBoard::~BRDBoard()
{
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

