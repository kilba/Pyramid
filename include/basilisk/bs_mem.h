#ifndef BS_MEM_H
#define BS_MEM_H

#include <stdint.h>
#include <bs_core.h>

#define BS_FLAGSET(flag, cmp) ((flag >> cmp) & 0x01)

typedef int64_t bs_I64, bs_long;
typedef int32_t bs_I32, bs_int;
typedef int16_t bs_I16, bs_short;
typedef int8_t  bs_I8, bs_byte;

typedef uint64_t bs_U64, bs_ulong;
typedef uint32_t bs_U32, bs_uint;
typedef uint16_t bs_U16, bs_ushort;
typedef uint8_t  bs_U8, bs_ubyte;

bs_JsonString bs_string(const char* str);

void* bs_free(void* p);
void* bs_alloc(bs_U32 size);
void* bs_realloc(void* p, bs_U32 size);

void* bs_bufferAppend(bs_Buffer* buf, void* data);
void* bs_bufferData(bs_Buffer* buf, bs_U32 offset);
void bs_bufferAppendRange(bs_Buffer* buf, void* data, size_t num_units);
void bs_bufferResizeCheck(bs_Buffer* buf, bs_U32 num_units);
bs_Buffer bs_buffer(bs_U32 type, bs_U32 unit_size, bs_U32 increment, bs_U32 pre_malloc, bs_U32 max_units);
bs_Buffer bs_singleUnitBuffer(void* data, bs_U32 unit_size);

bs_U8 bs_memU8 (void *data, bs_U32 offset);
bs_U16 bs_memU16(void *data, bs_U32 offset);
bs_U32 bs_memU32(void *data, bs_U32 offset);
int bs_memcmpU32(const void *a, const void *b);

char* bs_replaceFirstSubstring(const char *str, const char *old_str, const char *new_str);
char* bs_loadFile(const char *path, int *content_len);
void bs_appendToFile(const char *filepath, const char *data);
void bs_writeToFile(const char *filepath, const char *data);
void bs_writeBuffer(const char* file_path, void* buffer, bs_U64 size);

bs_U32 bs_numFloatDigits(double n);
bs_U32 bs_numIntDigits(bs_I64 n);

bs_I64 bs_toLong(const char* str);
double bs_toDouble(const char* str);

#endif // BS_MEM_H