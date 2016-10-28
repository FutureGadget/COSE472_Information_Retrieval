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