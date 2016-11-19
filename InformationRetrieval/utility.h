#pragma once
#include <Windows.h>
#include <fstream>
#include <tchar.h> 
#include <stdio.h>
#include <strsafe.h>
#include <vector>
#include <string>
#include <sstream>
#include <functional> 
#include <cctype>
#include <locale>
#include <algorithm>
#include <utility>
#include <map>
#pragma comment(lib, "User32.lib")

namespace utility {
	void get_file_paths(LPCWSTR path, std::vector<std::string>& path_returned);
	std::string format_digit(int, int);
	std::string format_weight(double);
	std::vector<std::string> tokenizer(std::string, char);
	std::string& ltrim(std::string &s);
	std::string& rtrim(std::string &s);
	std::string& trim(std::string &s);
	bool is_valid_char(char);
	void tokenize_only_alpha(std::string&, std::vector<std::string>&);
	bool if_exists(std::string);
	int get_doc_length(std::ifstream&, int);
	std::string get_doc_name(std::ifstream&, int);
	void print_result(std::ifstream&, std::vector<std::pair<double, int>>&, std::map<std::string, int>&, int, std::ofstream&);
	std::string merge(int first, int last);
	void external_merge_sort(std::ifstream& pre_file);
}