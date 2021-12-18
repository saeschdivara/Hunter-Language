#pragma once

#include <string>

void replaceAll(std::string& str, const std::string& from, const std::string& to);

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);

// trim from start (in place)
void ltrim(std::string &s);

// trim from end (in place)
void rtrim(std::string &s);

// trim from both ends (in place)
void trim(std::string &s);