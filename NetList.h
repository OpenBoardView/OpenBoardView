
#include "Board.h"

#include <vector>
#include <string>
#include <functional>

typedef std::function<void(char*)> TcharStringCallback;

class NetList
{

public:
	NetList(TcharStringCallback cbNetSelected);
	~NetList();

	void Draw(const char* title, bool* p_open, Board *board);

private:
	TcharStringCallback m_cbNetSelected;
};
