#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>

#include <bs_types.h>
#include <bs_mem.h>
#include <bs_wnd.h>

inline char* bs_getJsonToken(bs_Json* json, bs_JsonToken* tok) {
	char* p = json->token_data + tok->offset;
	return p;
}

// lexer
char* bs_lexJsonString(char* raw, char** out, int* out_len) {
	if (raw[0] == '\"') raw++;
	else return raw;

	const char* beginning = raw;

	while(raw[0] != '\0') {
		if (raw[0] == '\"') {
			*out = beginning;
			*out_len += (raw - beginning);

			return raw + 1;
		}

		raw++;
	}

	bs_callErrorf(BS_ERROR_JSON_EXPECTED_CHAR, 2, "Expected end-of-string quote char");
	return raw;
}

char* bs_lexJsonNumber(char* raw, char** out, int* out_len) {
	if ((raw[0] != '-') && (raw[0] < '0') && (raw[0] > '9')) {
		return raw;
	}

	const char* beginning = raw;

	while (raw[0] != '\0') {
		if (strchr("e-.0123456789", raw[0]) == NULL) {
			*out = beginning;
			*out_len += (raw - beginning);

			return raw;
		}

		raw++;
	}
}

char* bs_lexJsonWord(char* raw, char** out, int* out_len, const char* word, int word_len, int remaining) {
	if (remaining >= word_len && strncmp(raw, word, word_len) == 0) {
		*out = raw;
		*out_len += word_len;

		return raw + word_len;
	}

	return raw;
}

static bs_U32 bs_calculateNumTokens(bs_Json* json) {
	char* raw = json->raw;
	int len = 0;
	while (raw[0] != '\0') {
		int remaining = raw - json->raw;
		char* tok = NULL;
		char* prev = raw;

		if ((prev != (raw = bs_lexJsonString(raw, &tok, &len))) ||
			(prev != (raw = bs_lexJsonNumber(raw, &tok, &len))) ||
			(prev != (raw = bs_lexJsonWord(raw, &tok, &len, "true", 4, remaining))) ||
			(prev != (raw = bs_lexJsonWord(raw, &tok, &len, "false", 5, remaining))) ||
			(prev != (raw = bs_lexJsonWord(raw, &tok, &len, "null", 4, remaining)))
		) {
			json->num_tokens++;
		}
		else if (strchr("{[\":,]}", raw[0]) != NULL) {
			json->num_tokens++;
			len++;
			raw++;
		}
		else raw++;
	}
	return len;
}

void bs_lexJson(bs_Json* json) {
	json->token_len = bs_calculateNumTokens(json);
	json->token_data = bs_alloc(json->token_len + json->num_tokens);
	json->tokens = bs_alloc(json->num_tokens * sizeof(bs_JsonToken));
	bs_JsonToken* result = json->tokens;

	char* raw = json->raw;
	bs_U32 offset = 0;
	int tok_offset = 0;
	while (raw[0] != '\0') {
		int remaining = raw - json->raw;
		int tok_len = 0;
		char* tok = NULL;
		char* prev = raw;

		if (prev != (raw = bs_lexJsonString(raw, &tok, &tok_len))) {
			result->type = BS_JSON_STRING;
			memcpy(json->token_data + offset, tok, tok_len);
		}
		else if(prev != (raw = bs_lexJsonNumber(raw, &tok, &tok_len))) {
			result->type = BS_JSON_NUMBER;
			memcpy(json->token_data + offset, tok, tok_len);
		}
		else if (prev != (raw = bs_lexJsonWord(raw, &tok, &tok_len, "true", 4, remaining))) {
			result->type = BS_JSON_BOOL;
			memcpy(json->token_data + offset, tok, tok_len);
		}
		else if (prev != (raw = bs_lexJsonWord(raw, &tok, &tok_len, "false", 5, remaining))) {
			result->type = BS_JSON_BOOL;
			memcpy(json->token_data + offset, tok, tok_len);
		}
		else if (prev != (raw = bs_lexJsonWord(raw, &tok, &tok_len, "null", 4, remaining))) {
			result->type = BS_JSON_STRING;
			memcpy(json->token_data + offset, tok, tok_len);
		}
		else if (strchr("{[\":,]}", raw[0]) != NULL) {
			result->type = BS_JSON_SYNTAX;
			json->token_data[offset] = raw[0];
			tok_len = 1;
			raw++;
		}
		else {
			raw++;
			continue;
		}

		result->len = tok_len;
		result->offset = offset;
		result++;

		offset += tok_len;
		json->token_data[offset++] = '\0';
	}
}

// writer
inline bs_JsonField bs_createJsonField(
	const char* field, 
	int num_elements, 
	void* data,
	bool is_int,
	bool is_float,
	bool is_object
) {
	bs_JsonField json_value = { 0 };
	json_value.name = field;
	json_value.name_len = strlen(field);
	json_value.num_elements = num_elements;
	json_value.is_array = num_elements > 0;
	json_value.is_float = is_float;
	json_value.is_int = is_int;
	json_value.is_object = is_object;

	if (json_value.is_array) {
		json_value.value.as_array.as_objects = data;
	}
	else if(json_value.is_int){
		json_value.value.as_int = *(bs_I64*)data;
	}
	else if (json_value.is_float) {
		json_value.value.as_float = *(double*)data;
	}
	else if (json_value.is_object) {
		json_value.value.as_object = *(bs_Json*)data;
	}
	else {
		json_value.value.as_string = *(bs_JsonString*)data;
	}
	return json_value;
}

// int
bs_JsonField bs_createJsonInt(const char* field, bs_I64 value) {
	return bs_createJsonField(field, 0, &value, true, false, false);
}

bs_JsonField bs_createJsonIntArray(const char* field, bs_I64* value, int num_elements) {
	return bs_createJsonField(field, num_elements, value, true, false, false);
}

// float
bs_JsonField bs_createJsonFloat(const char* field, double value) {
	return bs_createJsonField(field, 0, &value, false, true, false);
}

bs_JsonField bs_createJsonFloatArray(const char* field, double* value, int num_elements) {
	return bs_createJsonField(field, num_elements, value, false, true, false);
}

// string
bs_JsonField bs_createJsonString(const char* field, bs_JsonString value) {
	return bs_createJsonField(field, 0, &value, false, false, false);
}

bs_JsonField bs_createJsonStringArray(const char* field, bs_JsonString* value, int num_elements) {
	return bs_createJsonField(field, num_elements, value, false, false, false);
}

// object
bs_JsonField bs_createJsonObject(const char* field, bs_Json* value) {
	return bs_createJsonField(field, 0, value, false, false, true);
}

bs_JsonField bs_createJsonObjectArray(const char* field, bs_Json* value, int num_elements) {
	return bs_createJsonField(field, num_elements, value, false, false, true);
}

int bs_addJsonObjectField(bs_Json* json, bs_JsonField* field, int offset, int index, int* indent) {
	json->raw[offset++] = '\n';
	for (int j = 0; j < (*indent); j++) {
		json->raw[offset++] = '\t';
	}

	bs_Json* obj = (field->num_elements == 0) ? &field->value.as_object : (field->value.as_array.as_objects + index);

	for (int i = 0; i < obj->raw_len; i++) {
		json->raw[offset++] = obj->raw[i];
		if (obj->raw[i] == '\n') {
			for (int j = 0; j < (*indent); j++) {
				json->raw[offset++] = '\t';
			}
		}
	}

	return offset;
}

bs_U32 bs_calculateJsonObjectFieldSize(bs_Json* json, bs_JsonField* field, int index, int* indent) {
	bs_U32 size = 1 + (*indent);
	bs_Json* ptr = (field->num_elements == 0) ? &field->value.as_object : field->value.as_array.as_objects;

	for (int i = 0; i < ptr[index].raw_len; i++) {
		size++;
		if (ptr[index].raw[i] == '\n') {
			for (int j = 0; j < (*indent); j++) {
				size++;
			}
		}
	}
	return size;
}

bs_U32 bs_calculateJsonObjectSize(bs_Json* json, bs_JsonField* fields, int num_fields) {
	bs_U32 size = sizeof("{}\n\n\t");

	int indent = 1;

	for (int i = 0; i < num_fields; i++) {
		bs_JsonField* field = fields + i;

		size += field->name_len + 4;
		if (field->is_array) size += 2; // []

		int num_elements = (field->is_array ? field->num_elements : 1);
		int indent = 1;
		for (int j = 0; j < num_elements; j++) {
			if (field->is_float) {
				double v = (field->num_elements == 0) ? field->value.as_float : field->value.as_array.as_floats[j];

				char buf[21];
				size += sprintf(buf, "%f", v);
			}
			else if (field->is_int) {
				bs_I64 v = (field->num_elements == 0) ? field->value.as_int : field->value.as_array.as_ints[j];

				char buf[21];
				size += sprintf(buf, "%lld", v);
			}
			else if (field->is_object) {
				size += bs_calculateJsonObjectFieldSize(json, field, j, &indent);
			}
			else {
				char* p = (field->num_elements == 0) ? field->value.as_string.value : field->value.as_array.as_strings[j].value;
				size += strlen(p) + 2;
			}
			
			if (j != (num_elements - 1)) {
				size += 2;
			}
		}

		//if (field->is_object) indent++;
		if (indent > 1) indent--;

		if (i != (num_fields - 1)) {
			size += 3;
		}
	}

	return size;
}

inline int bs_addJsonTokens(char* buf, const char* chars, int num_chars) {
	memcpy(buf, chars, num_chars);
	return num_chars;
}

char* bs_jsonFromFields(bs_Json* json, bs_JsonField* fields, int num_fields) {
	bs_U32 size = bs_calculateJsonObjectSize(json, fields, num_fields);

	json->raw_len = size - 1;
	json->raw = bs_alloc(size);
	memset(json->raw, 0, size);
	
	int offset = bs_addJsonTokens(json->raw, "{\n\t", 3);
	int indent = 1;

	for (int i = 0; i < num_fields; i++) {
		bs_JsonField* field = fields + i;

		offset += bs_addJsonTokens(json->raw + offset, "\"", 1);
		offset += bs_addJsonTokens(json->raw + offset, field->name, field->name_len);
		offset += bs_addJsonTokens(json->raw + offset, "\":", 2);
		offset += bs_addJsonTokens(json->raw + offset, " ", 1);

		if (field->is_array) {
			json->raw[offset++] = '[';
		}

		int num_elements = (field->is_array ? field->num_elements : 1);
		for (int j = 0; j < num_elements; j++) {

			if (field->is_int) {
				bs_I64 val = (field->num_elements == 0) ? field->value.as_int : field->value.as_array.as_ints[j];
				char buf[21];
				int strlen_buf = sprintf(buf, "%lld", val);

				offset += bs_addJsonTokens(json->raw + offset, buf, strlen_buf);
			} else if (field->is_float) {
				double val = (field->num_elements == 0) ? field->value.as_float : field->value.as_array.as_floats[j];
				char buf[21];
				int strlen_buf = sprintf(buf, "%f", val);

				offset += bs_addJsonTokens(json->raw + offset, buf, strlen_buf);
			}
			else if (field->is_object) {
				offset = bs_addJsonObjectField(json, field, offset, j, &indent);
			}
			else {
				char* ptr = (field->num_elements == 0) ? field->value.as_string.value : field->value.as_array.as_strings[j].value;
				offset += bs_addJsonTokens(json->raw + offset, "\"", 1);
				offset += bs_addJsonTokens(json->raw + offset, ptr, strlen(ptr));
				offset += bs_addJsonTokens(json->raw + offset, "\"", 1);
			}
			// todo null?

			if (j != (num_elements - 1)) {
				offset += bs_addJsonTokens(json->raw + offset, ", ", 2);
			}
		}

		//if (field->is_object) indent++;
		if (indent > 1) indent--;
		
		if (field->is_array) {
			offset += bs_addJsonTokens(json->raw + offset, "]", 1);
		}

		if (i != (num_fields - 1)) {
			offset += bs_addJsonTokens(json->raw + offset, ",\n\t", 3);
		}
	}
	offset += bs_addJsonTokens(json->raw + offset, "\n}\0", 3);

	if (json->raw_len != (offset - 1)) {
		bs_callErrorf(
			BS_ERROR_JSON_FAILED_CALCULATION, 2, 
			"JSON: Failed to calculate the correct size (expected %d, actual %d)", 
			json->raw_len, offset - 1
		);
	}

	return json->raw;
}

// parser
bs_JsonValue bs_parseJsonObject(bs_Json* json, bs_JsonToken* token) {
	bs_JsonValue val = { 0 };

	val.as_object.token_data = json->token_data;
	val.as_object.tokens = token;
	val.as_object.num_tokens = 0;
	val.found = true;
	char* tok = bs_getJsonToken(json, token);
	int num = 1;
	while (num != 0) {
		val.as_object.num_tokens++;
		tok = bs_getJsonToken(json, ++token);

		if (tok[0] == '{' || tok[0] == '[') num++;
		else if (tok[0] == '}' || tok[0] == ']') num--;
	}

	return val;
}

bs_U32 bs_calculateJsonObjectArraySize(bs_Json* json, bs_JsonToken* token, bs_U32 object_size) {
	bs_U32 size = 0;

	while (bs_getJsonToken(json, token)[0] != ']') {
		if (bs_getJsonToken(json, token)[0] == '{') {
			int num = 1;
			while (num != 0) {
				char tok = bs_getJsonToken(json, ++token)[0];

				if (tok == '{' || tok == '[') num++;
				else if (tok == '}' || tok == ']') num--;
			}

			size += object_size;
		}
		else {
			token++;
			continue;
		}

		token++;
	}
	return size;
}

bs_U32 bs_calculateJsonArraySize(bs_Json* json, bs_JsonToken* token) {
	bs_U32 size = 0;

	while (bs_getJsonToken(json, token)[0] != ']') {
		if (bs_getJsonToken(json, token)[0] == '{') {
			int num = 1;
			while (num != 0) {
				char tok = bs_getJsonToken(json, ++token)[0];

				if (tok == '{' || tok == '[') num++;
				else if (tok == '}' || tok == ']') num--;
			}

			size += sizeof(bs_Json);
		}
		else if (token->type == BS_JSON_NUMBER) {
			size += sizeof(bs_I64);
		}
		else if (token->type == BS_JSON_STRING) {
			size += sizeof(bs_JsonString);
		}
		else {
			token++;
			continue;
		}

		token++;
	}
	return size;
}

bs_JsonValue bs_parseJsonArray(bs_Json* json, bs_JsonToken* token) {
	bs_JsonValue val = { 0 };
	bs_U32 expected_size = bs_calculateJsonArraySize(json, token);
	bs_U32 actual_size = 0;
	val.found = true;
	val.as_array.as_strings = malloc(expected_size);
	int offset = 0;
	while (bs_getJsonToken(json, token)[0] != ']') {
		char* tok = bs_getJsonToken(json, token);

		if (tok[0] == '{') {
			val.as_array.as_objects[offset] = bs_parseJsonObject(json, token).as_object;
			int num_tokens = val.as_array.as_objects[offset].num_tokens;
			token += num_tokens;

			actual_size += sizeof(bs_Json);
		}
		else if (token->type == BS_JSON_NUMBER) {
			bool is_float = (strchr(tok, '.') != NULL) || (strchr(tok, 'e') != NULL);

			if (is_float) {
				double d = bs_toDouble(tok);
				val.as_array.as_floats[offset] = d;
			}
			else {
				bs_I64 i = bs_toLong(tok);
				val.as_array.as_ints[offset] = i;
			}

			actual_size += sizeof(bs_I64);
		}
		else if (token->type == BS_JSON_STRING) {
			val.as_array.as_strings[offset].value = tok;
			actual_size += sizeof(bs_JsonString);
		}
		else {
			token++;
			continue;
		}

		val.as_array.size++;
		offset++;
		token++;
	}

	if (expected_size != actual_size) {
		bs_callErrorf(
			BS_ERROR_JSON_FAILED_CALCULATION, 2,
			"JSON: Failed to calculate the correct size (expected %d, actual %d)",
			expected_size, actual_size
		);
	}

	return val;
}

bs_JsonToken* bs_findToken(bs_Json* json, const char* field) {
	int t = 0;
	for (int i = 1; i < json->num_tokens - 2; i++) {
		bs_JsonToken* token = json->tokens + i;

		char* x = bs_getJsonToken(json, token);
		char y = bs_getJsonToken(json, token + 1)[0];

		if (x[0] == '{' || x[0] == '[') t++;
		if (x[0] == '}' || x[0] == ']') t--;

		if (strcmp(x, field) == 0 && y == ':' && t == 0) {
			return token + 2;
		}
	}
	return NULL;
}

void* bs_jsonFieldObjectArray(
	bs_Json* json, 
	const char* field, 
	int* num_elements,
	bs_U32 size,
	void (*declaration)(bs_Json* json, void* destination, void* param),
	void* param) 
{
	bs_JsonToken* token = bs_findToken(json, field);

	if (token == NULL) return;
	if (bs_getJsonToken(json, token)[0] != '[') return;

	bs_U32 expected_size = bs_calculateJsonObjectArraySize(json, token, size);
	void* buffer = bs_alloc(expected_size);
	bs_U8* destination = buffer;

	int offset = 0;
	while (bs_getJsonToken(json, token)[0] != ']') {
		char* tok = bs_getJsonToken(json, token);

		if (tok[0] != '{') {
			token++;
			continue;
		}

		bs_Json obj = bs_parseJsonObject(json, token).as_object;
		declaration(&obj, destination + offset, param);

		token += obj.num_tokens;
		offset += size;
		token++;
	}

	*num_elements = expected_size / size;
	return buffer;
}

void bs_jsonFieldVector(bs_Json* json, const char* field, float* out, bs_U32 num_elements) {
	bs_JsonToken* token = bs_findToken(json, field);

	if (token == NULL) return;
	if (bs_getJsonToken(json, token)[0] != '[') return;

	int offset = 0;
	while (bs_getJsonToken(json, token)[0] != ']' && offset < num_elements) {
		char* tok = bs_getJsonToken(json, token);

		if (token->type == BS_JSON_NUMBER) {
			bool is_float = (strchr(tok, '.') != NULL) || (strchr(tok, 'e') != NULL);

			if (is_float) {
				double d = bs_toDouble(tok);
				out[offset] = d;
			}
			else {
				bs_I64 i = bs_toLong(tok);
				out[offset] = i;
			}
		}
		else {
			token++;
			continue;
		}

		offset++;
		token++;
	}

	// this should be fast and probably dont care if it fails so no error call
}

bs_JsonValue bs_jsonField(bs_Json* json, const char* field) {
	bs_JsonValue val = { 0 };
	bs_JsonToken* token = bs_findToken(json, field);
	if (token == NULL) return val;

	val.found = true;

	char* tok = bs_getJsonToken(json, token);

	if (tok[0] == '{') {
		return bs_parseJsonObject(json, token);
	}
	else if (tok[0] == '[') {
		return bs_parseJsonArray(json, token);
	}
	else if (token->type == BS_JSON_NUMBER) {
		bool is_float = (strchr(tok, '.') != NULL) || (strchr(tok, 'e') != NULL);

		if (is_float) {
			double d = bs_toDouble(tok);
			val.as_float = d;
		}
		else {
			bs_I64 i = bs_toLong(tok);
			val.as_int = i;
		}
		return val;
	}
	else if(token->type == BS_JSON_STRING) {
		val.as_string.value = tok;
		return val;
	}

	// bs_callErrorf(BS_ERROR_JSON_FIELD_NOT_FOUND, 1, "JSON: Could not find field \"%s\"", field);
	return val;
}

// loader
void bs_dumpTokens(bs_Json* json) {
	for (int i = 0; i < json->num_tokens; i++) {
		printf("%s\n", bs_getJsonToken(json, json->tokens + i));
	}
}

bs_Json bs_json(const char* raw) {
	bs_Json json = { 0 };
	json.raw = raw;
	json.raw_len = strlen(raw);

	bs_lexJson(&json);
	//bs_dumpTokens(&json);

	return json;
}

bs_Json bs_jsonFile(const char* path) {
	bs_Json json = { 0 };
	json.raw = bs_loadFile(path, &json.raw_len);

	bs_lexJson(&json);
	//bs_dumpTokens(&json);

	return json;
}

void bs_freeJson(bs_Json* json) {
	//json->raw = bs_free(json->raw);
	//json->tokens = bs_free(json->tokens);
	//json->token_data = bs_free(json->token_data);
}

bs_vec2 bs_jsonFieldV2(bs_Json* json, const char* field, bs_vec2 fallback) {
	bs_vec2 result = fallback;
	bs_jsonFieldVector(json, field, result.a, 2);
	return result;
}

bs_vec3 bs_jsonFieldV3(bs_Json* json, const char* field, bs_vec3 fallback) {
	bs_vec3 result = fallback;
	bs_jsonFieldVector(json, field, result.a, 3);
	return result;
}

bs_vec4 bs_jsonFieldV4(bs_Json* json, const char* field, bs_vec4 fallback) {
	bs_vec4 result = fallback;
	bs_jsonFieldVector(json, field, result.a, 4);
	return result;
}