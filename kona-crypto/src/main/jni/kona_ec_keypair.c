/*
 * Copyright (C) 2025, THL A29 Limited, a Tencent company. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>

#include <jni.h>

#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/objects.h>

#include "kona/kona_common.h"
#include "kona/kona_jni.h"

EVP_PKEY_CTX* ec_create_pkey_ctx(EVP_PKEY* pkey) {
    EVP_PKEY_CTX* ctx = NULL;

    if (pkey != NULL) {
        ctx = EVP_PKEY_CTX_new(pkey, NULL);
    } else {
        ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
    }

    if (ctx == NULL) {
        OPENSSL_print_err();

        return NULL;
    }

    return ctx;
}

JNIEXPORT jobjectArray JNICALL Java_com_tencent_kona_crypto_provider_nativeImpl_NativeCrypto_ecOneShotKeyPairGenGenKeyPair
  (JNIEnv* env, jclass classObj, jint curveNID) {
    EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
    if (pctx == NULL) {
        return NULL;
    }

    if (EVP_PKEY_keygen_init(pctx) <= 0) {
        EVP_PKEY_CTX_free(pctx);
        return NULL;
    }

    if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx, curveNID) <= 0) {
        EVP_PKEY_CTX_free(pctx);
        return NULL;
    }

    EVP_PKEY* pkey = NULL;
    if (EVP_PKEY_keygen(pctx, &pkey) <= 0) {
        EVP_PKEY_CTX_free(pctx);
        return NULL;
    }

    EVP_PKEY_CTX_free(pctx);

    const EC_KEY* ec_key = EVP_PKEY_get0_EC_KEY(pkey);
    const BIGNUM* priv_key_bn = EC_KEY_get0_private_key(ec_key);
    int priv_key_len = BN_num_bytes(priv_key_bn);
    unsigned char* priv_key_bytes = (unsigned char*)malloc(priv_key_len);
    if (priv_key_bytes == NULL) {
        EVP_PKEY_free(pkey);
        return NULL;
    }
    BN_bn2bin(priv_key_bn, priv_key_bytes);

    const EC_POINT* pub_key_point = EC_KEY_get0_public_key(ec_key);
    const EC_GROUP* group = EC_KEY_get0_group(ec_key);
    size_t pub_key_len = EC_POINT_point2oct(group, pub_key_point, POINT_CONVERSION_UNCOMPRESSED, NULL, 0, NULL);
    unsigned char* pub_key_bytes = (unsigned char*)malloc(pub_key_len);
    if (pub_key_bytes == NULL) {
        free(priv_key_bytes);
        EVP_PKEY_free(pkey);
        return NULL;
    }
    EC_POINT_point2oct(group, pub_key_point, POINT_CONVERSION_UNCOMPRESSED, pub_key_bytes, pub_key_len, NULL);

    jbyteArray privKeyArray = (*env)->NewByteArray(env, priv_key_len);
    if (privKeyArray == NULL) {
        free(priv_key_bytes);
        free(pub_key_bytes);
        EVP_PKEY_free(pkey);
        return NULL;
    }
    (*env)->SetByteArrayRegion(env, privKeyArray, 0, priv_key_len, (jbyte*)priv_key_bytes);

    jbyteArray pubKeyArray = (*env)->NewByteArray(env, (jsize)pub_key_len);
    if (pubKeyArray == NULL) {
        free(priv_key_bytes);
        free(pub_key_bytes);
        EVP_PKEY_free(pkey);
        return NULL;
    }
    (*env)->SetByteArrayRegion(env, pubKeyArray, 0, (jsize)pub_key_len, (jbyte*)pub_key_bytes);

    free(priv_key_bytes);
    free(pub_key_bytes);
    EVP_PKEY_free(pkey);

    jobjectArray result = (*env)->NewObjectArray(env, 2, (*env)->FindClass(env, "[B"), NULL);
    if (result == NULL) {
        return NULL;
    }
    (*env)->SetObjectArrayElement(env, result, 0, privKeyArray);
    (*env)->SetObjectArrayElement(env, result, 1, pubKeyArray);

    return result;
}

JNIEXPORT jlong JNICALL Java_com_tencent_kona_crypto_provider_nativeImpl_NativeCrypto_ecKeyPairGenCreateCtx
  (JNIEnv* env, jclass classObj, jint curveNID) {
    EVP_PKEY_CTX* pctx = ec_create_pkey_ctx(NULL);
    if (pctx == NULL) {
        return OPENSSL_FAILURE;
    }

    if (!EVP_PKEY_keygen_init(pctx)) {
        OPENSSL_print_err();
        EVP_PKEY_CTX_free(pctx);

        return OPENSSL_FAILURE;
    }

    if (!EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx, curveNID)) {
        OPENSSL_print_err();
        EVP_PKEY_CTX_free(pctx);

        return OPENSSL_FAILURE;
    }

    return (jlong)pctx;
}

JNIEXPORT void JNICALL Java_com_tencent_kona_crypto_provider_nativeImpl_NativeCrypto_ecKeyPairGenFreeCtx
  (JNIEnv* env, jclass classObj, jlong pointer) {
    EVP_PKEY_CTX* pctx = (EVP_PKEY_CTX*)pointer;
    if (pctx != NULL) {
        EVP_PKEY_CTX_free(pctx);
    }
}

JNIEXPORT jobjectArray JNICALL Java_com_tencent_kona_crypto_provider_nativeImpl_NativeCrypto_ecKeyPairGenGenKeyPair
  (JNIEnv* env, jclass classObj, jlong pointer) {
    EVP_PKEY_CTX* pctx = (EVP_PKEY_CTX*)pointer;
    if (pctx == NULL) {
        return NULL;
    }

    EVP_PKEY* pkey = NULL;
    if (!EVP_PKEY_keygen(pctx, &pkey)) {
        return NULL;
    }

    const EC_KEY* ec_key = EVP_PKEY_get0_EC_KEY(pkey);
    const BIGNUM* priv_key_bn = EC_KEY_get0_private_key(ec_key);
    int priv_key_len = BN_num_bytes(priv_key_bn);
    unsigned char* priv_key_bytes = (unsigned char*)malloc(priv_key_len);
    if (priv_key_bytes == NULL) {
        EVP_PKEY_free(pkey);

        return NULL;
    }
    BN_bn2bin(priv_key_bn, priv_key_bytes);

    const EC_POINT* pub_key_point = EC_KEY_get0_public_key(ec_key);
    const EC_GROUP* group = EC_KEY_get0_group(ec_key);
    size_t pub_key_len = EC_POINT_point2oct(group, pub_key_point, POINT_CONVERSION_UNCOMPRESSED, NULL, 0, NULL);
    unsigned char* pub_key_bytes = (unsigned char*)malloc(pub_key_len);
    if (pub_key_bytes == NULL) {
        free(priv_key_bytes);
        EVP_PKEY_free(pkey);

        return NULL;
    }
    EC_POINT_point2oct(group, pub_key_point, POINT_CONVERSION_UNCOMPRESSED, pub_key_bytes, pub_key_len, NULL);

    jbyteArray privKeyArray = (*env)->NewByteArray(env, priv_key_len);
    if (privKeyArray == NULL) {
        free(priv_key_bytes);
        free(pub_key_bytes);
        EVP_PKEY_free(pkey);

        return NULL;
    }
    (*env)->SetByteArrayRegion(env, privKeyArray, 0, priv_key_len, (jbyte*)priv_key_bytes);

    jbyteArray pubKeyArray = (*env)->NewByteArray(env, (jsize)pub_key_len);
    if (pubKeyArray == NULL) {
        free(priv_key_bytes);
        free(pub_key_bytes);
        EVP_PKEY_free(pkey);

        return NULL;
    }
    (*env)->SetByteArrayRegion(env, pubKeyArray, 0, (jsize)pub_key_len, (jbyte*)pub_key_bytes);

    free(priv_key_bytes);
    free(pub_key_bytes);
    EVP_PKEY_free(pkey);

    jobjectArray result = (*env)->NewObjectArray(env, 2, (*env)->FindClass(env, "[B"), NULL);
    if (result == NULL) {
        return NULL;
    }
    (*env)->SetObjectArrayElement(env, result, 0, privKeyArray);
    (*env)->SetObjectArrayElement(env, result, 1, pubKeyArray);

    return result;
}
