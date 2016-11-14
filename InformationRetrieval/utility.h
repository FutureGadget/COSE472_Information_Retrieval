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
#pragma comment(lib, "User32.lib")

using namespace std;

namespace utility {
	void get_file_paths(LPCWSTR current, vector<string>& paths);
	string format_digit(int num_digit, int num);
	string format_weight(double weight);
	vector<string> tokenizer(std::string s, char delim);
	string& ltrim(string &s);
	string& rtrim(string &s);
	string& trim(string &s);
	bool is_valid_char(char c);
	void tokenize_only_alpha(string&, vector<string>&);
}