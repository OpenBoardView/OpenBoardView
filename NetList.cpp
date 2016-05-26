#include "NetList.h"
#include "imgui\imgui.h"

NetList::NetList()
{

}


NetList::~NetList()
{
}


void NetList::Draw(const char* title, bool* p_open) {
	// TODO: implement
	ImGui::SetNextWindowSize(ImVec2(400, 640));

	ImGui::Begin("Net List");
	ImGui::Text("And Here are some contents.");
	ImGui::End();
}
