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

std::vector<BRDPart> BRDBoard::getParts()
{	
	return m_parts;
}

std::vector<BRDPin> BRDBoard::getPins()
{
	return m_pins;
}

std::vector<BRDNail> BRDBoard::getNails()
{
	return m_nails;
}

std::vector<BRDNail> BRDBoard::getUniqueNetList()
{
	return m_netUnique;
}

BoardType BRDBoard::getBoardType()
{
	return BRD;
}

