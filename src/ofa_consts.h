/**
 * Copyright 2017, unisound.com. All rights reserved.
 */
#ifndef YZS_OFA_CONSTS_H__
#define YZS_OFA_CONSTS_H__
// List all consts, including:
// - return value definition
// - option id definition
//

// option ID of grammar compiler
enum {
  OPT_ID_GC_SET_MAX_PRONS_LIMIT = 1000,               // value: limits
  OPT_ID_GC_SET_ACTION_WHEN_EXCEEDS_MAX_PRONS_LIMIT,  // value: 0: discard
                                                      //        1: insert first
                                                      //           limits prons
  OPT_ID_GC_GET_PROBLEM_WORDS,
};

// option ID for configuring LP engine net
enum {
  ASR_CONF_NET0_THRES = 200,
  ASR_CONF_NET1_THRES = 201,
  ASR_CONF_NET2_THRES = 202,
};
// option ID for configuring multi threads
enum {
  ASR_SET_THREAD_NUMBER = 500,
  ASR_GET_THREAD_NUMBER = 501,
  ASR_SET_THREAD_INIT = 502,
  ASR_SET_THREAD_TOCPU0 = 510,
  ASR_SET_THREAD_TOCPU1 = 511,
  ASR_SET_THREAD_TOCPU2 = 512,
  ASR_SET_THREAD_TOCPU3 = 513,
  ASR_SET_THREAD_TOCPU4 = 514,
  ASR_SET_THREAD_TOCPU5 = 515,
  ASR_SET_THREAD_TOCPU6 = 516,
  ASR_SET_THREAD_TOCPU7 = 517,
  ASR_SET_THREAD_TOCPU8 = 518,
  ASR_SET_THREAD_TOCPU9 = 519,
  ASR_GET_THREAD0_TID = 520,
  ASR_GET_THREAD1_TID = 521,
  ASR_GET_THREAD2_TID = 522,
  ASR_GET_THREAD3_TID = 523,
  ASR_GET_THREAD4_TID = 524,
  ASR_GET_THREAD5_TID = 525,
  ASR_GET_THREAD6_TID = 526,
  ASR_GET_THREAD7_TID = 527,
  ASR_GET_THREAD8_TID = 528,
  ASR_GET_THREAD9_TID = 529,
};

// option ID for engine
enum {
  ASR_ENGINE_AUDIO_MODEL_COUNT_ID = 100,
  ASR_ENGINE_UTTERANCE_START_TIME,           // 101
  ASR_ENGINE_UTTERANCE_STOP_TIME,            // 102
  ASR_ENGINE_AUTHORIZED_STATUS,              // 103
  ASR_ENGINE_AUTHORIZED_EXPIRY_TIME,         // 104
  ASR_ENGINE_AUTHORIZED_PACKAGE_NAME,        // 105
  ASR_ENGINE_COUNT_ID,                       // 106
  ASR_ENGINE_ACTIVE_ENGINE_ID,               // 107
  ASR_ENGINE_SET_TYPE_ID,                    // 108
  ASR_ENGINE_SET_RESET_FRONTEND_ID,          // 109
  ASR_ENGINE_SET_AM_CALCULATION_FRAME_STEP,  // 110
  ASR_ENGINE_SET_FAST_RETURN,
  ASR_ENGINE_SET_LP_ENGINE_THRESHOLD = 120,
  ASR_ENGINE_SET_LP_SKIP_FRAME = 121,
  ASR_CONF_OUT_INELIGIBLE = 122,
  ASR_ENGINE_GET_FRAMEOFFSET_ID = 123,
  ASR_RECORD_WAV_ID = 124,
  ASR_ENGINE_ALLOWHEADSPEECH_ID = 125,
  ASR_ENGINE_ALLOWTAILSPEECH_ID = 126,
};

// engine legacy ID
enum {
  ASR_FRAMESKIP_ID = 11,
  ASR_LOG_ID = 12,                        // Log level setting ID.
                                          // 0 silence
                                          // 1 verbose
                                          // 2 debug
                                          // 3 info
                                          // 4 warn
                                          // 5 error
                                          // fatal
  ASR_CALLSTACK_ID = 13,                  // CallStack setting ID
                                          // 0 disable
                                          // 1 enable
  ASR_SILENCE_HANGOUT_THRESH = 15,        // for setting silence hangout
  ASR_GET_AM_MODEL_ID = 16,               // get id the ith am model
  ASR_SET_ENGINE_ID = 17,                 // set engine type
  ASR_SET_MAX_HANGOUT_THRESHOLD_ID = 18,  // set fast return threshold
  ASR_SET_USE_LAST_RESULT = 19,           // set clear last time nn cache
  ASR_SET_AUX_INFO_ID = 20,               // read word penalty
  ASR_SET_BEAM_ID = 21,                   // set max token
  ASR_VAD_WAV_CHECK_MAX_LATENCY = 22,     // set max latency in  millisecond,
  // within max latency,check if there is new speech
  // after recognization
};
// engine return value
enum {
  ASR_NOT_IMPLEMENTED_OPTION_ID = -11,
  ASR_PATIAL_FILE_DAMAGED = -10,
  ASR_RECOGNIZER_WRONG_OPS = -9,
  ASR_AUTHORIZE_ERROR = -8,
  ASR_VAD_TIMEOUT = -7,
  ASR_MAX_SPEECH_TIMEOUT = -6,
  ASR_SERVICE_NOT_RUNNING = -5,
  ASR_MEM_ALLOCATION_ERROR = -4,
  ASR_COMPRESS_PCM_ERROR = -3,
  ASR_COMMUNICATION_ERROR = -2,
  ASR_FATAL_ERROR = -1,
  ASR_RECOGNIZER_OK = 0,
  ASR_RECOGNIZER_NO_RESULT = 1,
  ASR_RECOGNIZER_PARTIAL_RESULT = 2,
};

enum {
  ASR_VAD_NOWAV = 3,      // no new wav
  ASR_VAD_WAV_START = 4,  // new wav
};
// grammar compiler return value
enum {
  GC_OPTION_NOT_SUPPORT = -101,             // the option is not supported
  GC_HAS_WORDS_WITH_TOO_MUCH_PRONS = -102,  // some words in the vocab has
                                            // prons exceeds the maximum limits
  GC_FATAL_ERROR = -1,
  GC_OK = 0,
};

// Front End Sample Rate kHZ ID
enum {
  ASR_SAMPLE_RATE_KHZ = 600,
  ASR_FRONTEND_EXTRACT_ENERGY = 601,
};

// SET VAD para
enum {
  ASR_SET_VAD_MIN_SPEECH_LEN = 700,   // min speech len
  ASR_SET_VAD_MIN_SILENCE_LEN = 701,  // min silence len
  ASR_ENGINE_ENABLE_VAD_ID = 702,     // 开启VAD功能
};
#endif
