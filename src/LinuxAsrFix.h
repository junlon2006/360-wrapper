#ifndef _LinuxAsrFix_
#define _LinuxAsrFix_

#ifdef FOR_C_APP
#define YZS_CUBE_EXPORT __attribute__((visibility("default")))
#else  // FOR_C_APP
#define YZS_CUBE_EXPORT
#endif  // FOR_C_APP
#ifdef __cplusplus
#ifdef FOR_C_APP
extern "C" {
#else   // FOR_C_APP
namespace UniASR {
#endif  // FOR_C_APP
#endif  // __cplusplus
/**
 * @function: create and initialize asr engine.
 * @paras:
 *         path_of_am: full directory where audio
 *                     models (am*.dat, tr*.dat) exists.
 *         path_of_grammar: file path(s) of grammar(s), for multiple grammars,
 *                          seperate each path with ';'
 *         port: decrepted. NO USE.
 * @return: status code
 */
int init(const char* path_of_am, const char* path_of_grammar, short port);

/**
 * @functions: set options of engine
 * @paras:
 *          id: option id
 *          value: option value
 * @return: status code
 * @note  : details will be provided in other docs.
 */
int setOptionInt(int id, int value);

/**
 * @functions: get options of engine
 * @paras:  id: option id
 * @return: value of id or -65535 for not support
 * @note  : details will be provided in other docs.
 */
int getOptionInt(int id);

/**
 * Deprecated
 */
int setOptionString(int id, const char* value);

/**
 * @function: set active grammar and turn the engine into start state
 * @para:
 *            grammar_tag: tag(s) for grammar(s), which will be activated.
 *                         For multiple grammars, seperated their domains with
 * ';'
 *            am_index: index of audio model (for multiple audio models).
 *                      For single audio model distribution, please set this
 * value to 0.
 * @return: status code
 */
int start(const char* grammar_tag, int am_index);

/**
 * @function: reload grammar data. If the grammar has been loaded before, the
 *            newly loaded one will replace the original data.
 * @para:
 *            grammar_path: path of the grammar data
 *            type: Decprecated. NO USE.
 *
 * @return:   status code
 */
int reset(const char* grammar_path, const char* type);

/**
 * @function: set the engine state to be stop. If the engine has not return
 *            any result before, this function will flush the audio and try to
 *            find result.
 * @return:   status code.
 */
int stop();

/**
 * Deprecated
 */
char* search(const char* keyword, const char* type);

/**
 * Deprecated
 */
int isactive(char* raw, int len);

/**
 * @function: decoding
 * @para:
 *         raw_audio: audio data
 *         len: length of the input data in Bytes
 * @return: status code. if status code == ASR_RECOGNIZER_PARTIAL_RESULT,
 *          you can skip left audio and try to getResult.
 */
int recognize(char* raw_audio, int len);

/**
 * @function: retrieve recognition result from the engine
 * @return:   result or NULL for No-result
 */
char* getResult();

/**
 * Deprecated
 */
int cancel();

/**
 * @function: delete the engine.
 * @return:   status code
 */
void release();

/**
 * @function: return whether the asr engine is in idle status or not
 * @return: 1 idle; 0 non-idle
 */
int isEngineIdle();

/**
 * @function: load fst from string
 * @para:
 *            gramamr_str: string of the fst
 * @return:   status code
 *
 */
int loadGrammarStr(const char* grammar_str);
/**
 * @function: unload grammar from the memroy(reclaim the memroy)
 * @para:
 *            gramamr_tag: tag of the to be unloaded grammar.
 * @return:   status code
 *
 */
int unloadGrammar(const char* grammar_tag);
/**
 * @function: initialize user data compiler
 * @para:
 *            modeldir: full directory where models of compiler exist.
 * @return:  handle of the compiler or NULL for failure.
 *
 */
long initUserDataCompiler(const char* modeldir);

/**
 * Deprecated
 */
char* getTagsInfo(long handle);

/**
 * @function: online compiling user data into grammar data
 * @para:
 *         handle: handle of the user data compiler
 *                 (obtained via initUserDataCompiler)
 *         jsgf: text of this grammar's jsgf
 *         user: text of this grammar's user data
 *         out_dir: path of the compiler grammar data
 *
 * @return: handle of the user data compiler
 */
int compileUserData(long handle, const char* jsgf, const char* user,
                    const char* out_dir);

/**
 * @function: online compiling user data slot by slot. this function will
 *            compile user data slot in param 'vocab' first, and then, if
 *            a compiled vocab data exist, this function will append slots
 *            that not included by 'vocab'. The final updated compiled vocab
 *            will be saved in 'out_partial_data_path'.
 * @para:
 *         handle: handle of the user data compiler
 *                 (obtained via initUserDataCompiler)
 *         in_partial_data_path: input path of compiled user data slot
 *         jsgf: text of this grammar's jsgf
 *         user: text of this grammar's user data
 *         out_dir: path of the compiler grammar data
 *         out_partial_data_path: output path of compiled user data slot
 *
 *         in_partial_data_path and out_partial_data_path can be SAME. But they
 * must be
 *         actual path with write permission.
 *
 * @return: handle of the user data compiler
 * @Note:  this is a trial API, which means it might be changed in a near
 * future.
 */
int partialCompileUserData(long handle, const char* in_partial_data_path,
                           const char* jsgf, const char* vocab,
                           const char* out_dir,
                           const char* out_partial_data_path);
/**
 * @function: delete user data compiler
 * @para:
 *         handle: handle of the user data compiler
 */
void destroyUserDataCompiler(long handle);

/**
 * @function: get version number of the library
 * @return: string of version number
 */
const char* version();

/**
 * Deprecated
 */
int crcCheck(const char* path);

/**
 * @function: load compiled jsgf_clg*.dat
 * @para:
 *         handle: handle of the compiler
 *         path  : path of the to be loaded jsgf_clg*.dat
 *
 * @return: status code
 */
int loadCompiledJsgf(long handle, const char* path);

/**
 * @function: compile dynamic slot user data.
 *            this function will load the compiled data.
 * @para:
 *         handle: handle of the compiler
 *         user_data: slot user data
 *         grammar_tag: tag of the gramamr
 *
 * @return: status code!
 */
int compileDynamicUserData(long handle, const char* user_data,
                           const char* grammar_tag);

/**
 * @function: set option with int value
 * @return: status, 0 for OK, negative value for failure..
 */
int grammarCompilerSetOptionInt(long handle, int opt_id, int opt_value);

/**
 * @function: Get value of specific option id.
 * @return: value of the option_id, or NULL if no such value exists.
 */
const char* grammarCompilerGetOptionString(long handle, int opt_id);

/**
 * @function: Set the active fst net for lpengine
 * @para:
 *         idx: the active netid to be setted
 *
 * @return: active net id!
 */
int setActiveNet(int idx);

/**
 * @function: This function is used to trace lp engine internal info
 * @para:
 *        info_type: the internal info type
 * @return: void
 */
void trackInfo(int info_type);

#ifdef ENCRYPTION
/**
 * * @function: 设置密码字符串用于引擎验证.
 * * @paras:
 * * env: 环境变量
 * */
int ual_std_set_env(void* env);
#endif
/*
 * check weather come to wav end
 */
int check_wav_end();
long long get_vad_state();
#ifdef __cplusplus
}
#endif  // __cplusplus
#endif
