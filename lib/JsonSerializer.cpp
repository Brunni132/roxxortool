#include "Precompiled.h"
#include "JsonSerializer.h"
using std::to_string;

static string float_to_string(float value) {
	string transformed = to_string(value);
	bool foundComma = false;
	int lastNonZeroChar = 0;
	for (unsigned i = 0; i < transformed.size(); i++) {
		if (transformed[i] == '.')
			foundComma = true;
		else if (!foundComma || transformed[i] != '0')
			lastNonZeroChar = i;
	}
	return transformed.substr(0, lastNonZeroChar + 1);
}

JsonWriterNode& JsonWriterNode::put(string key, JsonWriterNode& node) {
	dico[key] = node;
	return *this;
}

JsonWriterNode& JsonWriterNode::put(string key, string value) {
	dico[key] = JsonWriterNode('"' + value + '"');
	return *this;
}

JsonWriterNode& JsonWriterNode::put(string key, const char *value) {
	dico[key] = JsonWriterNode('"' + string(value) + '"');
	return *this;
}

JsonWriterNode& JsonWriterNode::put(string key, unsigned value) {
	dico[key] = JsonWriterNode(to_string(value));
	return *this;
}

JsonWriterNode& JsonWriterNode::put(string key, int value) {
	dico[key] = JsonWriterNode(to_string(value));
	return *this;
}

JsonWriterNode& JsonWriterNode::put(string key, float value) {
	dico[key] = JsonWriterNode(float_to_string(value));
	return *this;
}

JsonWriterNode& JsonWriterNode::put(string key, bool value) {
	dico[key] = JsonWriterNode(value ? "true" : "false");
	return *this;
}

JsonWriterNode & JsonWriterNode::put(string key, const float *values, unsigned count) {
	JsonWriterNode results;
	for (unsigned i = 0; i < count; i++) {
		JsonWriterNode node(float_to_string(values[i]));
		results.put(node);
	}
	dico[key] = results;
	return *this;
}

JsonWriterNode & JsonWriterNode::put(string key, const unsigned short *values, unsigned count) {
	JsonWriterNode results;
	for (unsigned i = 0; i < count; i++) {
		JsonWriterNode node(to_string(values[i]));
		results.put(node);
	}
	dico[key] = results;
	return *this;
}

JsonWriterNode& JsonWriterNode::put(JsonWriterNode& node) {
	isArray = true;
	array.push_back(node);
	return *this;
}

static void print_r(JsonWriterNode &node, int indent, FILE *f) {
	if (node.isEmpty())
		return;
	if (node.isValue) {
		fputs(node.contents.c_str(), f);
	}
	else if (node.isArray) {
		bool writeInline = node.array[0].isValue;
		fputs(writeInline ? "[" : "[\n", f);
		indent++;
		for (auto it = node.array.begin(); it != node.array.end(); ) {
			if (it->isEmpty()) {
				++it;
				continue;
			}
			if (!writeInline) {
				for (int i = 0; i < indent; i++)
					fputc('	', f);
			}
			print_r(*it, indent, f);
			if (++it != node.array.end()) {
				fputc(',', f);
				if (writeInline)
					fputc(' ', f);
			}
			if (!writeInline)
				fputc('\n', f);
		}
		indent--;
		if (!writeInline) {
			for (int i = 0; i < indent; i++)
				fputc('	', f);
		}
		fputc(']', f);
	}
	else {
		fputs("{\n", f);
		indent++;
		for (auto it = node.dico.begin(); it != node.dico.end(); ) {
			if (it->second.isEmpty()) {
				++it;
				continue;
			}
			for (int i = 0; i < indent; i++)
				fputc('	', f);
			fprintf(f, "\"%s\": ", it->first.c_str());
			print_r(it->second, indent, f);
			if (++it != node.dico.end())
				fputc(',', f);
			fputc('\n', f);
		}
		indent--;
		for (int i = 0; i < indent; i++)
			fputc('	', f);
		fputc('}', f);
	}
}

void writeJson(JsonWriterNode &node, FILE *f) {
	print_r(node, 0, f);
}
