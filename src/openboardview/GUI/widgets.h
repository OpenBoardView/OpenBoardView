#ifndef _WIDGETS_H_
#define _WIDGETS_H_

#include <string>

void RightAlignedText(const char *t, int w);
bool MenuItemWithCheckbox(const std::string &label, const std::string &shortcut, bool &selected, bool enabled = true);

#endif//_WIDGETS_H_
