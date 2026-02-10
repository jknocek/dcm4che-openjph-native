/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is part of dcm4che, an implementation of DICOM(TM) in
 * Java(TM), hosted at https://github.com/dcm4che.
 *
 * The Initial Developer of the Original Code is
 * J4Care.
 * Portions created by the Initial Developer are Copyright (C) 2025
 * the Initial Developer. All Rights Reserved.
 *
 * ***** END LICENSE BLOCK ***** */

#include <jni.h>
#include <cstring>
#include "openjph_wrapper.h"

static void throwOpenJPHException(JNIEnv *env, const char *msg) {
    jclass exClass = env->FindClass("org/dcm4che3/openjph/OpenJPHException");
    if (exClass != nullptr) {
        env->ThrowNew(exClass, msg);
    }
}

extern "C" {

/*
 * Class:     org_dcm4che3_openjph_OpenJPH
 * Method:    encode
 * Signature: ([BIIIIZZFII)[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_dcm4che3_openjph_OpenJPH_encode(
    JNIEnv *env, jclass cls,
    jbyteArray rawPixelData,
    jint width, jint height, jint components,
    jint bitsPerSample, jboolean isSigned,
    jboolean reversible, jfloat compressionRatio,
    jint progressionOrder, jint decompositions)
{
    if (rawPixelData == nullptr) {
        throwOpenJPHException(env, "rawPixelData must not be null");
        return nullptr;
    }

    jsize rawSize = env->GetArrayLength(rawPixelData);

    // Use GetPrimitiveArrayCritical for zero-copy access to avoid copying
    // potentially large pixel buffers. This suspends GC for the duration of
    // ojph_encode(), but encode times are short (sub-second for typical DICOM
    // frames) so GC impact is negligible. Prefer this over GetByteArrayElements
    // which would copy the entire buffer.
    jbyte *rawBytes = (jbyte*)env->GetPrimitiveArrayCritical(rawPixelData, nullptr);
    if (rawBytes == nullptr) {
        throwOpenJPHException(env, "Failed to access raw pixel data");
        return nullptr;
    }

    ojph_encode_params params;
    memset(&params, 0, sizeof(params));
    params.width = (int)width;
    params.height = (int)height;
    params.components = (int)components;
    params.bits_per_sample = (int)bitsPerSample;
    params.is_signed = isSigned ? 1 : 0;
    params.reversible = reversible ? 1 : 0;
    params.compression_ratio = (float)compressionRatio;
    params.progression_order = (int)progressionOrder;
    params.decompositions = (int)decompositions;

    ojph_encode_result result = ojph_encode((const uint8_t*)rawBytes, (size_t)rawSize, &params);

    // Release the critical array - must happen before any JNI calls
    env->ReleasePrimitiveArrayCritical(rawPixelData, rawBytes, JNI_ABORT);

    if (result.error != nullptr) {
        char errorBuf[512];
        snprintf(errorBuf, sizeof(errorBuf), "HTJ2K encoding failed: %s", result.error);
        ojph_free_result(&result);
        throwOpenJPHException(env, errorBuf);
        return nullptr;
    }

    // Copy result to Java byte array
    jbyteArray jresult = env->NewByteArray((jsize)result.size);
    if (jresult == nullptr) {
        ojph_free_result(&result);
        throwOpenJPHException(env, "Failed to allocate result byte array");
        return nullptr;
    }

    env->SetByteArrayRegion(jresult, 0, (jsize)result.size, (const jbyte*)result.data);
    ojph_free_result(&result);

    return jresult;
}

} /* extern "C" */
