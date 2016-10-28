#pragma once
#include <Windows.h>
#include <fstream>
#include <tchar.h> 
#include <stdio.h>
#include <strsafe.h>
#include <vector>
#include <string>
#include <sstream>
#pragma comment(lib, "User32.lib")

using namespace std;

namespace utility {
	void get_file_paths(LPCWSTR current, vector<string>& paths);
	string format_digit(int num_digit, int num);
	string format_weight(double weight);
	vector<string> tokenizer(std::string s, char delim);
}