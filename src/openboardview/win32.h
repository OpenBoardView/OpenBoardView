#ifndef _WIN32_H_
#define _WIN32_H_

#include <string>

const std::string utf16_to_utf8(const std::wstring &text);

const std::wstring utf8_to_utf16(const std::string &text);

#endif//_WIN32_H_
