#pragma once
#include <string>
#include <vector>
using namespace std;

struct document {
	string docno;
	vector<string> headline;
	vector<string> text;
};