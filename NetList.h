
#include <vector>
#include <string>
#include <functional>
#include "Board.h"

using namespace std;

typedef std::function<void(char*)> TcharStringCallback;

class NetList
{

public:
	NetList(TcharStringCallback cbNetSelected);
	~NetList();

	void Draw(const char* title, bool* p_open, Board *board);

private:
	vector<string> m_strings;
	TcharStringCallback m_cbNetSelected;
};
