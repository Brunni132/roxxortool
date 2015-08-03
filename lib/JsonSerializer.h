#pragma once
#include <vector>
#include <map>
#include <string>
#include "Ref.h"
using std::string;
using std::map;
using std::vector;

struct JsonWriterNode: public RefClass {
	bool isValue, isArray;
	string contents;
	map<string, JsonWriterNode> dico;
	vector<JsonWriterNode> array;

	JsonWriterNode& operator = (const JsonWriterNode &node) {
		isValue = node.isValue;
		isArray = node.isArray;
		contents = node.contents;
		dico = node.dico;
		array = node.array;
		return *this;
	}
	JsonWriterNode(string withValue) : contents(withValue), isValue(true), isArray(false) {}
	JsonWriterNode() : isValue(false), isArray(false) {}
	JsonWriterNode(const JsonWriterNode& copy) {
		operator = (copy);
	}
	bool isEmpty() { return dico.empty() && array.empty() && contents.empty(); }
	JsonWriterNode &put(string key, JsonWriterNode& node);
	JsonWriterNode &put(string key, string value);
	JsonWriterNode &put(string key, const char *value);
	JsonWriterNode &put(string key, unsigned value);
	JsonWriterNode &put(string key, int value);
	JsonWriterNode &put(string key, float value);
	JsonWriterNode &put(string key, const float *values, unsigned count);
	JsonWriterNode &put(string key, const unsigned short *values, unsigned count);
	JsonWriterNode &put(string key, bool value);
	JsonWriterNode &put(JsonWriterNode &node);
};

JsonWriterNode& node();
void writeJson(JsonWriterNode& node, FILE *f);
