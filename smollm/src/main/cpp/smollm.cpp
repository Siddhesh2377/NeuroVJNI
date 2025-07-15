#include "LLMInference.h"
#include <jni.h>

extern "C" JNIEXPORT jlong JNICALL
Java_io_shubham0204_smollm_SmolLM_loadModel(JNIEnv *env, jobject, jstring modelPath, jfloat minP, jfloat temperature, jboolean storeChats, jlong contextSize, jstring chatTemplate, jint nThreads, jboolean useMmap, jboolean useMlock) {
    const char *modelPathCstr = env->GetStringUTFChars(modelPath, nullptr);
    const char *chatTemplateCstr = env->GetStringUTFChars(chatTemplate, nullptr);

    auto *llmInference = new LLMInference();
    try {
        llmInference->loadModel(modelPathCstr, minP, temperature, storeChats, contextSize, chatTemplateCstr, nThreads, useMmap, useMlock);
    } catch (const std::runtime_error &error) {
        env->ReleaseStringUTFChars(modelPath, modelPathCstr);
        env->ReleaseStringUTFChars(chatTemplate, chatTemplateCstr);
        delete llmInference;
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"), error.what());
        return 0;
    }

    env->ReleaseStringUTFChars(modelPath, modelPathCstr);
    env->ReleaseStringUTFChars(chatTemplate, chatTemplateCstr);
    return reinterpret_cast<jlong>(llmInference);
}

extern "C" JNIEXPORT void JNICALL
Java_io_shubham0204_smollm_SmolLM_addChatMessage(JNIEnv *env, jobject, jlong modelPtr, jstring message, jstring role) {
    if (!modelPtr) return;
    auto *llmInference = reinterpret_cast<LLMInference *>(modelPtr);

    const char *messageCstr = env->GetStringUTFChars(message, nullptr);
    const char *roleCstr = env->GetStringUTFChars(role, nullptr);

    llmInference->addChatMessage(messageCstr, roleCstr);

    env->ReleaseStringUTFChars(message, messageCstr);
    env->ReleaseStringUTFChars(role, roleCstr);
}

extern "C" JNIEXPORT jfloat JNICALL
Java_io_shubham0204_smollm_SmolLM_getResponseGenerationSpeed(JNIEnv *, jobject, jlong modelPtr) {
    if (!modelPtr) return 0.0f;
    return reinterpret_cast<LLMInference *>(modelPtr)->getResponseGenerationTime();
}

extern "C" JNIEXPORT jint JNICALL
Java_io_shubham0204_smollm_SmolLM_getContextSizeUsed(JNIEnv *, jobject, jlong modelPtr) {
    if (!modelPtr) return 0;
    return reinterpret_cast<LLMInference *>(modelPtr)->getContextSizeUsed();
}

extern "C" JNIEXPORT void JNICALL
Java_io_shubham0204_smollm_SmolLM_close(JNIEnv *, jobject, jlong modelPtr) {
    if (!modelPtr) return;
    delete reinterpret_cast<LLMInference *>(modelPtr);
}

extern "C" JNIEXPORT void JNICALL
Java_io_shubham0204_smollm_SmolLM_startCompletion(JNIEnv *env, jobject, jlong modelPtr, jstring prompt) {
    if (!modelPtr) return;

    const char *promptCstr = env->GetStringUTFChars(prompt, nullptr);
    reinterpret_cast<LLMInference *>(modelPtr)->startCompletion(promptCstr);
    env->ReleaseStringUTFChars(prompt, promptCstr);
}

extern "C" JNIEXPORT jstring JNICALL
Java_io_shubham0204_smollm_SmolLM_completionLoop(JNIEnv *env, jobject, jlong modelPtr) {
    if (!modelPtr) return env->NewStringUTF("");

    try {
        std::string response = reinterpret_cast<LLMInference *>(modelPtr)->completionLoop();
        return env->NewStringUTF(response.c_str());
    } catch (const std::runtime_error &error) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"), error.what());
        return nullptr;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_io_shubham0204_smollm_SmolLM_stopCompletion(JNIEnv *, jobject, jlong modelPtr) {
    if (!modelPtr) return;
    reinterpret_cast<LLMInference *>(modelPtr)->stopCompletion();
}

extern "C" JNIEXPORT void JNICALL
Java_io_shubham0204_smollm_SmolLM_clearChatMemory(JNIEnv *, jobject, jlong modelPtr) {
    if (!modelPtr) return;
    reinterpret_cast<LLMInference *>(modelPtr)->clearChatMemory();
}

extern "C"
JNIEXPORT void JNICALL
Java_io_shubham0204_smollm_SmolLM_stopGenerationImmediately(JNIEnv *env, jobject thiz, jlong modelPtr) {
    if (modelPtr == 0) return;
    auto *llmInference = reinterpret_cast<LLMInference *>(modelPtr);
    llmInference->stopGenerationImmediately();
}

