#include "utility.h"

void utility::get_file_paths(LPCWSTR current, vector<string>& paths) {
	WIN32_FIND_DATA FindFileData;
	TCHAR tmp[MAX_PATH];
	StringCchCopy(tmp, MAX_PATH, current);
	StringCchCat(tmp, MAX_PATH, TEXT("\\*"));

	HANDLE hFind = FindFirstFile(tmp, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		return;
	}
	do {
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (wcscmp(FindFileData.cFileName, TEXT(".")) != 0 && wcscmp(FindFileData.cFileName, TEXT("..")) != 0) {
				TCHAR next[MAX_PATH];
				StringCchCopy(next, MAX_PATH, current);
				StringCchCat(next, MAX_PATH, TEXT("\\"));
				StringCchCat(next, MAX_PATH, FindFileData.cFileName);
				get_file_paths(next, paths);
			}
		}
		else {
			TCHAR path[MAX_PATH];
			StringCchCopy(path, MAX_PATH, current);
			StringCchCat(path, MAX_PATH, TEXT("\\"));
			StringCchCat(path, MAX_PATH, FindFileData.cFileName);
			wstring ws = path;
			string tmp = "";
			tmp.assign(ws.begin(), ws.end());
			paths.push_back(tmp);
		}
	} while (FindNextFile(hFind, &FindFileData));

	FindClose(hFind);
	return;
}

vector<string> utility::tokenizer(std::string s, char delim) {
	std::vector<std::string> tokens;
	std::stringstream ss(s);
	std::string token;
	while (getline(ss, token, delim)) {
		tokens.push_back(token);
	}
	return tokens;
}

string utility::format_digit(int num_digit, int num) { // 색인어ID,문서ID,TF를 원하는 자릿수로 표현
	string result = "";
	for (int i = 0; i < num_digit; ++i) {
		result = result + (to_string(num % 10));
		num /= 10;
	}
	reverse(result.begin(), result.end());
	return result;
}

string utility::format_weight(double weight) {
	string results = "";
	ostringstream os;
	os << weight;

	string str = os.str();

	if (weight == (int)weight) {
		str.append(".0");
	}

	if (str.size() > 7) {
		str = str.substr(0, 7);
	}
	else {
		while (str.size() < 7) {
			str.append("0");
		}
	}
	return str;
}

// trim from start
string& utility::ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(),
		std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

// trim from end
string& utility::rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(),
		std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

// trim from both ends
string& utility::trim(std::string &s) {
	return ltrim(rtrim(s));
}

bool utility::is_valid_char(char c) {
	if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '\'') return true;
	return false;
}

void utility::tokenize_only_alpha(string& s, vector<string>& tokens) {
	string tmp;
	bool is_counting = false;
	tokens.clear();
	for (int i = 0; i < s.size(); ++i) {
		if (!is_counting && is_valid_char(s.at(i))) {
			if ((i - 1 >= 0 && s.at(i - 1) != '\'' && s.at(i) == '\'' && i + 1 < s.size() && s.at(i + 1) != '\'') || s.at(i) != '\'') {
				is_counting = true;
				tmp = s.at(i);
			}
		}
		else if (is_counting && is_valid_char(s.at(i))) {
			if ((i - 1 >= 0 && s.at(i - 1) != '\'' && s.at(i) == '\'' && i + 1 < s.size() && s.at(i + 1) != '\'') || s.at(i) != '\'') {
				tmp += s.at(i);
			}
		}
		else if (is_counting && !is_valid_char(s.at(i))) {
			is_counting = false;
			tokens.push_back(tmp);
		}
	}
	if (is_counting) {
		tokens.push_back(tmp);
	}
}