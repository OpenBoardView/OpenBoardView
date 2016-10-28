#include <string>
#include <vector>
#include <algorithm>

class SpellCorrector {
	unsigned int threshold = 3;
	std::vector<std::string> dictionary;

	unsigned int levenshtein_distance(const std::string& s1, const std::string& s2, size_t limit);
	unsigned int fuzzy_match(const std::string& s1, const std::string& s2);

public:
	void setDictionary(const std::vector<std::string>& dictionnary);

	std::vector<std::string> suggest(const std::string& word);
};
