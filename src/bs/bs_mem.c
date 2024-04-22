#include <glad/glad.h>

#include <bs_core.h>
#include <bs_math.h>
#include <bs_wnd.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

bs_JsonString bs_string(const char* str) {
    bs_JsonString string = { 0 };
    string.value = str;
    string.len = strlen(str);
    return string;
}

void* bs_free(void* p) {
    free(p);
    return NULL;
}

void* bs_alloc(bs_U32 size) {
    void* p = malloc(size);
    if (p == NULL) {
        bs_callErrorf(BS_ERROR_MEM_ALLOCATION, 2, "malloc returned null");
    }

    return p;
}

void* bs_realloc(void* p, bs_U32 size) {
    p = realloc(p, size);
    if (p == NULL) {
        bs_callErrorf(BS_ERROR_MEM_ALLOCATION, 2, "realloc returned null");
    }

    return p;
}

void* bs_bufferData(bs_Buffer* buf, bs_U32 offset) {
    return ((uint8_t*)buf->data) + offset * buf->unit_size;
}

void bs_bufferResizeCheck(bs_Buffer* buf, bs_U32 num_units) {
    if ((buf->size + num_units) < buf->capacity) {
        return;
    }

    if (buf->max_units != 0 && num_units >= buf->max_units) {
        return; // todo log, or not?
    }

    bs_U32 prev_capacity = buf->capacity;
    buf->capacity += bs_max(num_units, buf->increment);

    if (buf->realloc_ram) {
        buf->data = realloc(buf->data, buf->capacity * buf->unit_size);

        memset(bs_bufferData(buf, prev_capacity), 0, (buf->capacity - prev_capacity) * buf->unit_size);
        //printf("REALLOCED : %p, %d (%d units)\n", buf, buf->num_units * buf->unit_size, buf->size);
    }

    if (buf->realloc_vram) {
        glBufferData(buf->type, buf->capacity * buf->unit_size, buf->data, GL_DYNAMIC_DRAW);
    }
}

void* bs_bufferAppend(bs_Buffer* buf, void* data) {
    bs_bufferResizeCheck(buf, 1); // TODO: is 1 here correct?

    uint8_t* dest = bs_bufferData(buf, buf->size);

    if (data != NULL) {
        memcpy(dest, data, buf->unit_size);
    }

    buf->size++;
    return dest;
}

void bs_bufferAppendRange(bs_Buffer* buf, void* data, size_t num_units) {
    bs_bufferResizeCheck(buf, num_units);

    uint8_t* dest = bs_bufferData(buf, buf->size);
    memcpy(dest, data, buf->unit_size * num_units);

    buf->size += num_units;
}

bs_Buffer bs_buffer(bs_U32 type, bs_U32 unit_size, bs_U32 increment, bs_U32 pre_alloc, bs_U32 max_units) {
    bs_Buffer buf = { 0 };

    buf.unit_size = unit_size;
    buf.increment = increment;
    buf.realloc_ram = true;
    buf.realloc_vram = type != 0;
    buf.type = type;
    buf.max_units = max_units;

    if (pre_alloc != 0) {
        bs_bufferResizeCheck(&buf, pre_alloc);
    }

    return buf;
}

bs_Buffer bs_singleUnitBuffer(void* data, bs_U32 unit_size) {
    bs_Buffer buf = { 0 };

    buf.data = data;
    buf.capacity = 1;
    buf.size = 1;
    buf.unit_size = unit_size;
    buf.realloc_ram = true;

    return buf;
}

bs_U8 bs_memU8(void *data, bs_U32 offset) {
    return *((bs_U8*)data + offset);
}

bs_U16 bs_memU16(void *data, bs_U32 offset) {
    const bs_U8* base = (bs_U8*)data + offset;
    return (bs_U16) (base[0] << 8 | base[1]);
}

bs_U32 bs_memU32(void *data, bs_U32 offset) {
    const bs_U8* base = (bs_U8*)data + offset;
    return (bs_U32) (base[0] << 24 | base[1] << 16 | base[2] << 8 | base[3]);
}

int bs_memcmpU32(const void *a, const void *b) {
    return memcmp(a, b, sizeof(bs_U32));
}

char* bs_replaceFirstSubstring(const char* str, const char* old_str, const char* new_str) {
    if(str == NULL || old_str == NULL || new_str == NULL) return NULL;
    int new_len = strlen(new_str);
    int old_len = strlen(old_str);
    int str_len = strlen(str);

    // Counting the number of times old word
    // occur in the string
    char *start = strstr(str, old_str);

    if(start == NULL) 
	return NULL;

    int replace_offset = start - str;
    int last_size = str_len - replace_offset - old_len;
    
    int new_size = replace_offset + new_len + last_size + 1;

    // Making new string of enough length
    char *result = bs_alloc(new_size);
    char *offset = result;

    memcpy(offset, str, replace_offset); offset += replace_offset;
    memcpy(offset, new_str, new_len); offset += new_len;
    memcpy(offset, str + replace_offset + old_len, last_size);
    result[new_size-1] = '\0';

    return result;
}

char* bs_loadFile(const char* path, int* content_len) {
    if (path == NULL) {
        bs_callErrorf(BS_ERROR_MEM_PATH_NOT_FOUND, 2, "File \"%s\" not found");
        return NULL;
    }

    long length = 0;
    char* buffer = 0;
    FILE* f = fopen (path, "rb");

    if (f) {
        fseek (f, 0, SEEK_END);
        length = ftell (f) + 1;
        fseek (f, 0, SEEK_SET);
        buffer = bs_alloc(length);

        if (buffer) {
            fread(buffer, 1, length, f);
        }

        fclose (f);
    } else {
        bs_callErrorf(BS_ERROR_MEM_FAILED_TO_READ, 2, "Failed to read file \"%s\"", path);
        return NULL;
    }

    *content_len = length;
    buffer[length - 1] = '\0';
    return buffer;
}

void bs_appendToFile(const char *filepath, const char *data) {
    FILE *fp = fopen(filepath, "ab");
    if (fp != NULL) {
        fputs(data, fp);
        fclose(fp);
    }
}

void bs_writeToFile(const char *filepath, const char *data) {
    FILE *fp = fopen(filepath, "w");
    if (fp != NULL) {
        fputs(data, fp);
        fclose(fp);
    }  
}

void bs_writeBuffer(const char* file_path, void* buffer, bs_U64 size) {
    FILE* file = fopen(file_path, "wb");
    if (file == NULL) {
        bs_callErrorf(BS_ERROR_MEM_PATH_NOT_FOUND, 2, "Failed to open file \"%s\"", file_path);
        return 1;
    }

    size_t written = fwrite(buffer, sizeof(char), size, file);
    if (written < size) {
        bs_callErrorf(BS_ERROR_MEM, 2, "Failed to write the full buffer \"%s\", (%d, %d)", file_path, written, size);
        fclose(file);
        return 1;
    }

    fclose(file);
}

bs_U32 bs_numIntDigits(bs_I64 n) {
    if (n < 0) return bs_numIntDigits((n == INT_MIN) ? INT_MAX : -n);
    if (n < 10) return 1;
    return 1 + bs_numIntDigits(n / 10);
}

// todo make this better
bs_U32 bs_numFloatDigits(double n) {
    char buf[32];
    return sprintf(buf, "%.4f", n);
}

bs_I64 bs_toLong(const char* str) {
    char* o = NULL;
    return strtol(str, &o, 10);
}

double bs_toDouble(const char* str) {
    char* o = NULL;
    return strtod(str, &o);
}