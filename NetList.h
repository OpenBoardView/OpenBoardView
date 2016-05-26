
#include <vector>
#include <string>

using namespace std;

class NetList
{

public:
	NetList();
	~NetList();

	void Draw(const char* title, bool* p_open);

private:
	vector<string> m_strings;

};
