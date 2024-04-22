#include <tchar.h>
#include <windows.h>
#include <mmreg.h>
#include <xaudio2.h>
#include <basilisk.h>

#ifdef _XBOX //Big-Endian
#define fourccRIFF 'RIFF'
#define fourccDATA 'data'
#define fourccFMT 'fmt '
#define fourccWAVE 'WAVE'
#define fourccXWMA 'XWMA'
#define fourccDPDS 'dpds'
#endif

#ifndef _XBOX //Little-Endian
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'
#endif

bool audio_initialized = false;
const float max_volume = 4.0;

IXAudio2* pXAudio2 = NULL;
wchar_t executable_path[MAX_PATH] = { 0 };
bs_U32 executable_path_len = 0;

void bs_playSound(bs_Sound* sound, float volume) {
    IXAudio2SourceVoice* voice = sound->xaudio;
    if (voice == NULL) return;

    if (volume > max_volume) volume = max_volume;
    if (volume < -max_volume) volume = -max_volume;

    XAUDIO2_BUFFER buffer = { 0 };
    buffer.AudioBytes = sound->size;
    buffer.pAudioData = sound->data;
    buffer.Flags = XAUDIO2_END_OF_STREAM;

    voice->lpVtbl->Stop(voice, 0, 0);
    voice->lpVtbl->FlushSourceBuffers(voice);
    voice->lpVtbl->SubmitSourceBuffer(voice, &buffer, NULL);
    voice->lpVtbl->SetVolume(voice, volume, 0);
    voice->lpVtbl->Start(voice, 0, 0);
}

static void bs_findAudioChunk(HANDLE hFile, DWORD fourcc, DWORD* dwChunkSize, DWORD* dwChunkDataPosition);
static void bs_readAudioChunk(HANDLE hFile, void* buffer, DWORD buffersize, DWORD bufferoffset);
bs_Sound bs_sound(const char* name) {
    bs_Sound sound = { 0 };

    if (!audio_initialized) {
        bs_callErrorf(BS_ERROR_AUDIO_VOICE, 1, "Attempted to load audio file \"%s\", audio not initialized", name);
        return sound;
    }

    wchar_t wide[256];
    wcsncpy(wide, executable_path, executable_path_len);
    int result = MultiByteToWideChar(CP_UTF8, 0, name, -1, wide + executable_path_len, 256);
    if (result == 0) {
        bs_callErrorf(1, 2, "failed converting char array to wide char array");
        return sound;
    }

    IXAudio2SourceVoice* src_voice = NULL;
    WAVEFORMATEXTENSIBLE wfx = { 0 };
    DWORD chunk_size;
    DWORD chunk_offset;
    DWORD filetype;
    HANDLE file_handle = CreateFile(wide, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    BYTE* data;

    if (file_handle == INVALID_HANDLE_VALUE ||
        SetFilePointer(file_handle, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
        char path[256];
        wcstombs(path, wide, 256);
        bs_callErrorf(1, 1, "could not find the file \"%s\"", path);
        return sound;
    }

    //check the file type, should be fourccWAVE or 'XWMA'
    bs_findAudioChunk(file_handle, fourccRIFF, &chunk_size, &chunk_offset);
    bs_readAudioChunk(file_handle, &filetype, sizeof(DWORD), chunk_offset);
    if (filetype != fourccWAVE) {
        bs_callErrorf(1, 1, "incorrect filetype \"%s\"", filetype);
        return sound;
    }

    bs_findAudioChunk(file_handle, fourccFMT, &chunk_size, &chunk_offset);
    bs_readAudioChunk(file_handle, &wfx, chunk_size, chunk_offset);

    bs_findAudioChunk(file_handle, fourccDATA, &chunk_size, &chunk_offset);
    data = bs_alloc(chunk_size);
    bs_readAudioChunk(file_handle, data, chunk_size, chunk_offset);

    if (result = FAILED(pXAudio2->lpVtbl->CreateSourceVoice(pXAudio2, &src_voice, (WAVEFORMATEX*)&wfx, 0, XAUDIO2_MAX_FREQ_RATIO, NULL, NULL, NULL))) {
        bs_callErrorf(result, 1, "bs_sound() failed");
        return sound;
    }

    sound.xaudio = src_voice;
    sound.data = data;
    sound.size = chunk_size;
    return sound;
}

void bs_initAudio() {
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    IXAudio2MasteringVoice* pMasterVoice = NULL;

    if (FAILED(hr)) {
        bs_callErrorf(BS_ERROR_AUDIO_INITIALIZE, 2, "Failed to initialize xaudio2");
        return;
    }

    if (FAILED(hr = XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
        bs_callErrorf(BS_ERROR_AUDIO_CREATE, 2, "Failed to create xaudio2");
        return;
    }

    if (FAILED(hr = pXAudio2->lpVtbl->CreateMasteringVoice(pXAudio2, &pMasterVoice, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE, 0, NULL, NULL, 0))) {
        bs_callErrorf(BS_ERROR_AUDIO_VOICE, 1, "Failed to create xaudio2 mastering voice");
        return;
    }

    audio_initialized = true;
    executable_path_len = GetModuleFileName(NULL, executable_path, MAX_PATH);
    int i = 0;
    for (i = executable_path_len - 1; i >= 0; i--) {
        char c = executable_path[i] == '\\' ? '/' : executable_path[i];
        if (c == '/') {
            if(i == executable_path_len - 1) break;
            else executable_path[i + 1] = '\0';

            break;
        }
    }
    executable_path_len -= (executable_path_len - i) - 1;
}

static void bs_findAudioChunk(HANDLE hFile, DWORD fourcc, DWORD* dwChunkSize, DWORD* dwChunkDataPosition) {
    HRESULT hr = S_OK;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN)) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    DWORD dwChunkType;
    DWORD dwChunkDataSize;
    DWORD dwRIFFDataSize = 0;
    DWORD dwFileType;
    DWORD bytesRead = 0;
    DWORD dwOffset = 0;

    while (hr == S_OK) {
        DWORD dwRead;
        if (0 == ReadFile(hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }

        if (0 == ReadFile(hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }

        switch (dwChunkType) {
        case fourccRIFF:
            dwRIFFDataSize = dwChunkDataSize;
            dwChunkDataSize = 4;
            if (0 == ReadFile(hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL)) {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
            break;

        default:
            if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, dwChunkDataSize, NULL, FILE_CURRENT)) {
                bs_callErrorf(GetLastError(), 1, "failed to find audio data");
                return;
            }
        }

        dwOffset += sizeof(DWORD) * 2;

        if (dwChunkType == fourcc)
        {
            *dwChunkSize = dwChunkDataSize;
            *dwChunkDataPosition = dwOffset;
            return;
        }

        dwOffset += dwChunkDataSize;

        if (bytesRead >= dwRIFFDataSize) {
            bs_callErrorf(GetLastError(), 2, "invalid audio data");
            return;
        }
    }
}

static void bs_readAudioChunk(HANDLE hFile, void* buffer, DWORD buffersize, DWORD bufferoffset) {
    HRESULT hr = S_OK;
    DWORD dwRead;

    if (SetFilePointer(hFile, bufferoffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
        bs_callErrorf(GetLastError(), 1, "failed to read audio data");
        return;
    }

    if (ReadFile(hFile, buffer, buffersize, &dwRead, NULL) == 0) {
        bs_callErrorf(GetLastError(), 1, "failed to read audio data");
    }
}