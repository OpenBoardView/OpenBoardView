#include "BRDBoard.h"

enum class SearchMode {
	Sub,
	Prefix,
	Whole,
};

class Searcher {
	SearchMode m_searchMode = SearchMode::Sub;

	SharedVector<Net> m_nets;
	SharedVector<Component> m_parts;

	template<class T> SharedVector<T> searchFor(const std::string& search, SharedVector<T> v, int limit, std::vector<std::string T::*> fields_to_look);
	bool strstrModeSearch(const std::string &strhaystack, const std::string &strneedle);
public:
	//fields to use while searching Component. Logically its a set, but set<pointer-to-member> doesn't compile.
	std::vector<std::string Component::*> part_fields_to_look = {&Component::name};
	void setNets(SharedVector<Net> nets);
	void setParts(SharedVector<Component> components);

	bool isMode(SearchMode sm);
	void setMode(SearchMode sm);
	SharedVector<Component> parts(const std::string& search, int limit);
	SharedVector<Component> parts(const std::string& search);
	SharedVector<Net> nets(const std::string& search, int limit);
	SharedVector<Net> nets(const std::string& search);
};
