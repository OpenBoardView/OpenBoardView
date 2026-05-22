#include "SpellCorrector.h"

void SpellCorrector::setDictionary(const std::vector<std::string>& dictionary) {
	this->dictionary = dictionary;
}

/**
 * From https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance#C.2B.2B
 * Added a limit parameter to limit the machting to n characters of the second string.
 */
unsigned int SpellCorrector::levenshtein_distance(const std::string& s1, const std::string& s2, size_t limit) {
	const std::size_t len1 = s1.size(), len2 = s2.size();
	limit = std::min(limit, len2);
	std::vector<unsigned int> col(limit+1), prevCol(limit+1);

	for (unsigned int i = 0; i < prevCol.size(); i++)
		prevCol[i] = i;
	for (unsigned int i = 0; i < len1; i++) {
		col[0] = i+1;
		for (unsigned int j = 0; j < limit; j++)
			col[j+1] = std::min({ prevCol[1 + j] + 1, col[j] + 1, prevCol[j] + (s1[i]==s2[j] ? 0 : 1) });
		col.swap(prevCol);
	}
	return prevCol[limit];
}

/**
 * Compute the levenshtein distance with a limit to s1 length + 1 character.
 * Returns 0 (meaning not matched) when the distance is above the threshold.
 */
unsigned int SpellCorrector::fuzzy_match(const std::string& s1, const std::string& s2) {
	unsigned int score = levenshtein_distance(s1, s2, s1.size() + 1);
	if (score > this->threshold) return 0;
	return threshold - score;
}

std::vector<std::string> SpellCorrector::suggest(const std::string& word) {
	std::vector<std::string> suggestions; // What will actually be returned, a vector of matched words with levenshtein below the threshold
	std::vector<std::pair<std::string, unsigned int>> scores; // Vector of pairs of matched words and their associated score
	std::string wordLower = word;
	std::transform(wordLower.begin(), wordLower.end(), wordLower.begin(), ::tolower); // Lowercase the word to search for

	for (auto &s : dictionary) {
		std::string sLower = s;
		std::transform(sLower.begin(), sLower.end(), sLower.begin(), ::tolower); // Lowercase the word from the dictionary

		unsigned int score = fuzzy_match(wordLower, sLower);
		if (score) scores.push_back({s, score});
	}

	// Sort by score, descending
	std::sort(scores.begin(), scores.end(), [] (const std::pair<std::string, unsigned int>& v1, const std::pair<std::string, unsigned int>& v2) {
		return v1.second > v2.second;
	});

	for (auto &p : scores) suggestions.push_back(p.first);

	return suggestions;
}
