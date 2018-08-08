#ifndef UNILINAER_MIC_ARRAY_H_
#define UNILINAER_MIC_ARRAY_H_

#if defined _WIN32 || defined __CYGWIN__
    #ifdef BUILDING_DLL
        #ifdef __GNUC__
            #define DLL_PUBLIC __attribute__((dllexport))
        #else
            #define DLL_PUBLIC __declspec(dllexport)
        #endif
    #else
        #ifdef __GNUC__
            #define DLL_PUBLIC __attribute__((dllimport))
        #else
            #define DLL_PUBLIC __declspec(dllimport)
        #endif
    #endif
#else
    #ifdef __GNUC__
        #if __GNUC__ >= 4 || defined(__arm__)
            #define DLL_PUBLIC __attribute__((visibility("default")))
            #define DLL_LOCAL  __attribute__((visibility("hidden")))
        #else
            #define DLL_PUBLIC
            #define DLL_LOCAL
        #endif
    #endif
#endif

#define UNI_LINEAR_MICARRAY_VERBOSE                   0
#define UNI_LINEAR_MICARRAY_AEC                       10
#define UNI_LINEAR_MICARRAY_MCLP                      11
#define UNI_LINEAR_MICARRAY_AGC                       12
#define UNI_LINEAR_MICARRAY_MICROPHONE_NUM            21
#define UNI_LINEAR_MICARRAY_ECHO_NUM                  22
#define UNI_LINEAR_MICARRAY_WAKE_UP_TIME_LEN          101
#define UNI_LINEAR_MICARRAY_WAKE_UP_TIME_DELAY        102
#define UNI_LINEAR_MICARRAY_ROBOT_FACE_DEGREE         103
#define UNI_LINEAR_MICARRAY_DOA_START                 104
#define UNI_LINEAR_MICARRAY_DOA_RESULT                105
#define UNI_LINEAR_MICARRAY_AEC_NSHIFT                201
#define UNI_LINEAR_MICARRAY_AEC_FILTER_NUM            202

#ifdef __cplusplus
extern "C" {
#endif

DLL_PUBLIC void* Unisound_LinearMicArray_Init(const char* cfg_path);
DLL_PUBLIC int Unisound_LinearMicArray_Process(void* handle, const short* in,
                                               int in_len, short* echo_ref,
                                               int is_waked, short** out_asr,
                                               short** out_vad, int* out_len);
DLL_PUBLIC int Unisound_LinearMicArray_Compute_DOA(void* handle, float time_len,
                                                   float time_delay);
DLL_PUBLIC void Unisound_LinearMicArray_Get(void* handle, int type, void* value);
DLL_PUBLIC void Unisound_LinearMicArray_Set(void* handle, int type, void* value);
DLL_PUBLIC void Unisound_LinearMicArray_Reset(void* handle);
DLL_PUBLIC void Unisound_LinearMicArray_Release(void* handle);
DLL_PUBLIC const char* Unisound_LinearMicArray_Version();

#ifdef __cplusplus
}
#endif
#endif
