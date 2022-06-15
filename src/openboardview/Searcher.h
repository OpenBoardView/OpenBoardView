#include "BRDBoard.h"

enum class SearchMode {
	Sub,
	Prefix,
	Whole,
};

class Searcher {
	SearchMode m_searchMode = SearchMode::Sub;
	bool m_search_details   = false;

	SharedVector<Net> m_nets;
	SharedVector<Component> m_parts;

	template<class T> std::vector<T> searchFor(const std::string& search, std::vector<T> v,  int limit);
	bool strstrModeSearch(const std::string &strhaystack, const std::string &strneedle);
public:
	void setNets(SharedVector<Net> nets);
	void setParts(SharedVector<Component> components);

	bool isMode(SearchMode sm);
	void setMode(SearchMode sm);
	SharedVector<Component> parts(const std::string& search, int limit);
	SharedVector<Component> parts(const std::string& search);
	SharedVector<Net> nets(const std::string& search, int limit);
	SharedVector<Net> nets(const std::string& search);

	bool &configSearchDetails() {
		return m_search_details;
	}

};
