#include "LLMInference.h"
#include "llama.h"
#include "gguf.h"
#include <android/log.h>
#include <cstring>
#include <iostream>

#define TAG "[SmolLMAndroid-Cpp]"
#define LOGi(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGe(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

std::vector<llama_token> common_tokenize(const struct llama_vocab *vocab, const std::string &text, bool add_special, bool parse_special = false);
std::string common_token_to_piece(const struct llama_context *ctx, llama_token token, bool special = true);

void LLMInference::loadModel(const char *model_path, float minP, float temperature, bool storeChats, long contextSize, const char *chatTemplate, int nThreads, bool useMmap, bool useMlock) {
    LOGi("Loading model: %s", model_path);

    llama_model_params model_params = llama_model_default_params();
    model_params.use_mmap = useMmap;
    model_params.use_mlock = useMlock;

    if (FILE *f = fopen(model_path, "rb")) fclose(f);
    else {
        LOGe("Model file not found: %s", model_path);
        throw std::runtime_error("Model file not found");
    }

    _model = llama_model_load_from_file(model_path, model_params);
    if (!_model) throw std::runtime_error("Model load failed");

    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = contextSize;
    ctx_params.n_threads = nThreads;
    ctx_params.no_perf = false;

    _ctx = llama_init_from_model(_model, ctx_params);
    if (!_ctx) throw std::runtime_error("Context init failed");

    llama_sampler_chain_params sampler_params = llama_sampler_chain_default_params();
    sampler_params.no_perf = false;
    _sampler = llama_sampler_chain_init(sampler_params);
    llama_sampler_chain_add(_sampler, llama_sampler_init_min_p(minP, 1));
    llama_sampler_chain_add(_sampler, llama_sampler_init_temp(temperature));
    llama_sampler_chain_add(_sampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

    _formattedMessages.resize(llama_n_ctx(_ctx));
    _chatTemplate = strdup(chatTemplate);
    _storeChats = storeChats;
    _messages.clear();
    LOGi("Model loaded successfully and memory initialized.");
}

void LLMInference::addChatMessage(const char *message, const char *role) {
    _messages.push_back({ strdup(role), strdup(message) });
}

float LLMInference::getResponseGenerationTime() const {
    return _responseGenerationTime > 0 ? (float)_responseNumTokens / (_responseGenerationTime / 1e6f) : 0.0f;
}

int LLMInference::getContextSizeUsed() const {
    return _nCtxUsed;
}

void LLMInference::startCompletion(const char *query) {
    _stopRequested.store(false);
    _response.clear();
    _cacheResponseTokens.clear();

    if (!_storeChats) {
        for (auto &msg : _messages) {
            free(const_cast<char *>(msg.role));
            free(const_cast<char *>(msg.content));
        }
        _messages.clear();
        _prevLen = 0;
        _formattedMessages.resize(llama_n_ctx(_ctx));
        LOGi("Cleared memory for fresh chat context.");
    }

    _responseGenerationTime = 0;
    _responseNumTokens = 0;

    addChatMessage(query, "user");

    int newLen = llama_chat_apply_template(_chatTemplate, _messages.data(), _messages.size(), true, _formattedMessages.data(), _formattedMessages.size());
    if (newLen > (int)_formattedMessages.size()) {
        _formattedMessages.resize(newLen);
        newLen = llama_chat_apply_template(_chatTemplate, _messages.data(), _messages.size(), true, _formattedMessages.data(), _formattedMessages.size());
    }
    if (newLen < 0) throw std::runtime_error("Chat template apply failed");

    _promptTokens = common_tokenize(llama_model_get_vocab(_model), std::string(_formattedMessages.begin() + _prevLen, _formattedMessages.begin() + newLen), true, true);

    _batch.token = _promptTokens.data();
    _batch.n_tokens = _promptTokens.size();
    LOGi("Prompt tokens prepared, ready to generate.");
}

bool LLMInference::_isValidUtf8(const char *response) {
    if (!response) return true;
    const unsigned char *bytes = reinterpret_cast<const unsigned char *>(response);
    while (*bytes) {
        int num = (*bytes & 0x80) == 0x00 ? 1 : (*bytes & 0xE0) == 0xC0 ? 2 : (*bytes & 0xF0) == 0xE0 ? 3 : (*bytes & 0xF8) == 0xF0 ? 4 : 0;
        if (!num) return false;
        bytes++;
        for (int i = 1; i < num; ++i) if ((*bytes++ & 0xC0) != 0x80) return false;
    }
    return true;
}

std::string LLMInference::completionLoop() {
    if (_stopRequested.load()) return "[EOG]";

    _nCtxUsed = llama_kv_self_used_cells(_ctx);
    if (_nCtxUsed + _batch.n_tokens > llama_n_ctx(_ctx)) throw std::runtime_error("Context size exceeded");

    auto start = ggml_time_us();
    if (llama_decode(_ctx, _batch) < 0) throw std::runtime_error("Decode failed");

    _currToken = llama_sampler_sample(_sampler, _ctx, -1);
    if (llama_vocab_is_eog(llama_model_get_vocab(_model), _currToken)) {
        if (_storeChats) addChatMessage(_response.c_str(), "assistant");
        _response.clear();
        return "[EOG]";
    }

    _cacheResponseTokens += common_token_to_piece(_ctx, _currToken, true);
    _responseGenerationTime += (ggml_time_us() - start);
    _responseNumTokens++;

    _batch.token = &_currToken;
    _batch.n_tokens = 1;

    if (_isValidUtf8(_cacheResponseTokens.c_str())) {
        _response += _cacheResponseTokens;
        std::string valid_piece = _cacheResponseTokens;
        _cacheResponseTokens.clear();
        LOGi("Generated partial output: %s", valid_piece.c_str());
        return valid_piece;
    }
    return "";
}

void LLMInference::stopCompletion() {
    _stopRequested.store(true);
    if (!_storeChats) llama_kv_self_clear(_ctx);

    if (_storeChats) addChatMessage(_response.c_str(), "assistant");
    else {
        for (auto &msg : _messages) {
            free(const_cast<char *>(msg.role));
            free(const_cast<char *>(msg.content));
        }
        _messages.clear();
    }

    _response.clear();
    _cacheResponseTokens.clear();

    const char *tmpl = llama_model_chat_template(_model, nullptr);
    _prevLen = llama_chat_apply_template(tmpl, _messages.data(), _messages.size(), false, nullptr, 0);
    if (_prevLen < 0) throw std::runtime_error("Chat template reset failed");
    LOGi("Stopped generation and reset chat context.");
}

void LLMInference::stopGenerationImmediately() {
    _stopRequested.store(true);
    _response.clear();
    _cacheResponseTokens.clear();
    LOGi("Forced immediate stop of generation.");
}

LLMInference::~LLMInference() {
    LOGi("Deallocating LLMInference");
    for (auto &msg : _messages) {
        free(const_cast<char *>(msg.role));
        free(const_cast<char *>(msg.content));
    }
    free(const_cast<char *>(_chatTemplate));
    llama_model_free(_model);
    llama_free(_ctx);
}

void LLMInference::clearChatMemory() {
    _messages.clear();
    _formattedMessages.clear();
    _promptTokens.clear();
    _response.clear();
    _cacheResponseTokens.clear();
    _nCtxUsed = 0;
    LOGi("Cleared chat memory and internal buffers.");
}
