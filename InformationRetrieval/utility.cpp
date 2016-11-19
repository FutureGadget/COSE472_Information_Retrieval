#include "utility.h"
#include "Entities.h"

using namespace std;

bool utility::if_exists(string name) {
	ifstream f(name.c_str());
	return f.good();
}

int utility::get_doc_length(ifstream& doc_file, int doc_id) {
	string line;
	doc_file.seekg((unsigned long long)(doc_id - 1)*(6 + 16 + 4 + 2));
	getline(doc_file, line);

	return stoi(line.substr(22, 4));
}

void utility::print_result(ifstream& docu_dat_file, vector<pair<double, int>>& result,
	map<string, int>& query, int queryNum, ofstream& outfile) {
	outfile << queryNum << endl;
	for (auto &res : result) {
		outfile << get_doc_name(docu_dat_file, res.second) << "\t";
	}
	outfile << endl;
}

string utility::get_doc_name(ifstream& doc_file, int doc_id) {
	string line;
	doc_file.seekg((unsigned long long)(doc_id - 1)*(6 + 16 + 4 + 2));
	getline(doc_file, line);

	return line.substr(6, 16);
}

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

//void utility::tokenize_only_alpha(string& s, vector<string>& tokens) {
//	string tmp;
//	bool is_counting = false;
//	tokens.clear();
//	for (int i = 0; i < s.size(); ++i) {
//		if (!is_counting && is_valid_char(s.at(i))) {
//			if ((i - 1 >= 0 && s.at(i - 1) != '\'' && s.at(i) == '\'' && i + 1 < s.size() && s.at(i + 1) != '\'') || s.at(i) != '\'') {
//				is_counting = true;
//				tmp = s.at(i);
//			}
//		}
//		else if (is_counting && is_valid_char(s.at(i))) {
//			if ((i - 1 >= 0 && s.at(i - 1) != '\'' && s.at(i) == '\'' && i + 1 < s.size() && s.at(i + 1) != '\'') || s.at(i) != '\'') {
//				tmp += s.at(i);
//			}
//		}
//		else if (is_counting && !is_valid_char(s.at(i))) {
//			is_counting = false;
//			tokens.push_back(tmp);
//		}
//	}
//	if (is_counting) {
//		tokens.push_back(tmp);
//	}
//}

void utility::tokenize_only_alpha(string& s, vector<string>& tokens) {
	stringstream ss(s);
	string tok;
	bool check = false;
	tokens.clear();
	while (getline(ss, tok, ' ')) {
		stringstream ss2(tok);
		while (getline(ss2, tok)) {
			string tmp = "";
			for (int i = 0; i < tok.size(); ++i) {
				if (!utility::is_valid_char(tok[i])) {
					if (tmp != "") {
						tokens.push_back(tmp);
					}
					tmp = "";
					check = false;
				}
				else if (tok[i] == '\'') {
					if (!check) check = true;
					else {
						if (tmp != "") {
							tokens.push_back(tmp);
						}
						tmp = "";
						check = false;
					}
				}
				else {
					tmp += tok[i];
					check = false;
				}
			}
			if (tmp != "") {
				tokens.push_back(tmp);
			}
		}
	}
}
string utility::merge(int first, int last) {
	ifstream file1, file2;
	ofstream res_file;
	string f1, f2, f3, line1, line2;
	int a, b;
	double w1, w2;
	TCHAR t1[MAX_PATH], t2[MAX_PATH];
	if (first < last) {
		int mid = (first + last) / 2;
		f1 = merge(first, mid);
		f2 = merge(mid + 1, last);
		file1.open(f1);
		file2.open(f2);
		if (f1.size() + f2.size() >= 50) {
			f3 = f1.substr(0, f1.size() / 2) + "_" + f2.substr(0, f2.size() / 2);
		}
		else {
			f3 = f1 + "_" + f2;
		}
		res_file.open(f3);

		getline(file1, line1);
		getline(file2, line2);
		while (!file1.eof() || !file2.eof()) {
			if (file1.eof()) {
				res_file << line2 << endl;
				getline(file2, line2);
			}
			else if (file2.eof()) {
				res_file << line1 << endl;
				getline(file1, line1);
			}
			else {
				a = stoi(line1.substr(0, 6));
				w1 = stod(line1.substr(15, 7));
				b = stoi(line2.substr(0, 6));
				w2 = stod(line2.substr(15, 7));
				if (a < b) {
					res_file << line1 << endl;
					getline(file1, line1);
				}
				else if (a > b) {
					res_file << line2 << endl;
					getline(file2, line2);
				}
				else if (a == b) {
					if (w1 > w2) {
						res_file << line1 << endl;
						getline(file1, line1);
					}
					else {
						res_file << line2 << endl;
						getline(file2, line2);
					}
				}
			}
		}
		wstring tmp1(f1.begin(), f1.end());
		wstring tmp2(f2.begin(), f2.end());
		GetFullPathName(tmp1.c_str(), MAX_PATH, t1, NULL);
		GetFullPathName(tmp2.c_str(), MAX_PATH, t2, NULL);
		file1.close();
		file2.close();

		DeleteFile(t1);
		DeleteFile(t2);
		res_file.close();
		return f3;
	}
	else {
		return to_string(first);
	}
}

void utility::external_merge_sort(ifstream& pre_file) {
	string line;
	vector<inverted_index> local;
	vector<string> tokens;
	TCHAR temp[MAX_PATH];
	int file_cnt = 0;
	ofstream out;
	int cnt = 0;

	// PASS 1
	while (getline(pre_file, line)) {
		if (cnt < 1500000) {
			local.push_back(inverted_index(stoi(line.substr(0, 6)), stod(line.substr(15, 7)), line.substr(6, 9)));
			++cnt;
		}
		else {
			sort(local.begin(), local.end());
			out.open(to_string(file_cnt));
			for (int i = 0; i < local.size(); ++i) {
				out << utility::format_digit(6, local[i].word_id) << local[i].data << utility::format_weight(local[i].weight) << endl;
			}
			out.close();
			local.clear();
			++file_cnt;
			cnt = 1;
			local.push_back(inverted_index(stoi(line.substr(0, 6)), stod(line.substr(15, 7)), line.substr(6, 9)));
		}
	}
	out.open(to_string(file_cnt));
	sort(local.begin(), local.end());
	for (int i = 0; i < local.size(); ++i) {
		out << utility::format_digit(6, local[i].word_id) << local[i].data << utility::format_weight(local[i].weight) << endl;
	}
	++file_cnt;
	out.close();

	// PASS 2
	string final_file = merge(0, file_cnt - 1);
	ifstream from(final_file);
	out.open("Index.dat");
	while (getline(from, line)) {
		out << line << endl;
	}
	out.close();
	from.close();
	GetFullPathName(wstring(final_file.begin(), final_file.end()).c_str(), MAX_PATH, temp, NULL);
	DeleteFile(temp);
}