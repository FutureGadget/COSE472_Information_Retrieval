#pragma once
#include <string>

struct word_info {
	int id;
	int cf;
	int df;
	int start;
	word_info() {
		this->cf = 0;
		this->df = 0;
	}
};

struct inverted_index {
	int word_id;
	double weight;
	std::string data;
	bool operator<(inverted_index& t) {
		if (this->word_id == t.word_id) {
			return this->weight > t.weight;
		}
		else {
			return this->word_id < t.word_id;
		}
	}
	inverted_index() {}
	inverted_index(int word_id, double weight, std::string data) {
		this->word_id = word_id;
		this->weight = weight;
		this->data = data;
	}
};