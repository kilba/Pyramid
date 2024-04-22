#ifndef BS_JSON_H
#define BS_JSON_H

#include <bs_types.h>

bs_JsonField bs_createJsonInt(const char* field, bs_I64 value);
bs_JsonField bs_createJsonIntArray(const char* field, bs_I64* value, int num_elements);
bs_JsonField bs_createJsonFloat(const char* field, double value);
bs_JsonField bs_createJsonFloatArray(const char* field, double* value, int num_elements);
bs_JsonField bs_createJsonString(const char* field, bs_JsonString value);
bs_JsonField bs_createJsonStringArray(const char* field, bs_JsonString* value, int num_elements);
bs_JsonField bs_createJsonObject(const char* field, bs_Json* value);
bs_JsonField bs_createJsonObjectArray(const char* field, bs_Json* value, int num_elements);

bs_Json bs_json(const char* raw);
bs_Json bs_jsonFile(const char* path);
void bs_freeJson(bs_Json* json);
bs_JsonValue bs_jsonField(bs_Json* json, const char* field);
char* bs_jsonFromFields(bs_Json* json, bs_JsonField* fields, int num_fields);

void* bs_jsonFieldObjectArray(
	bs_Json* json,
	const char* field,
	int* num_elements,
	bs_U32 size,
	void (*declaration)(bs_Json* json, void* destination, void* param),
	void* param
);

bs_vec2 bs_jsonFieldV2(bs_Json* json, const char* field, bs_vec2 fallback);
bs_vec3 bs_jsonFieldV3(bs_Json* json, const char* field, bs_vec3 fallback);
bs_vec4 bs_jsonFieldV4(bs_Json* json, const char* field, bs_vec4 fallback);

#endif // BS_JSON_H