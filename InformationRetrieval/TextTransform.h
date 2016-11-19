#pragma once
#include <fstream>
#include <map>
#include <string>
#include <Windows.h>
#include "Parameters.h"

void text_transform_indexing(LPCWSTR datafilePath);
void create_word_data_file(std::ofstream&);
void text_transform(std::string& , std::map<std::string, int>& , int , int &);
void fast_parse(std::ifstream& , std::ofstream& , std::ofstream& );
void updateWordInfo(std::string& , std::map<std::string, int>& , int );
void preprocessing();