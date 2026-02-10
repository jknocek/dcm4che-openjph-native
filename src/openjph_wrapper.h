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

#ifndef OPENJPH_WRAPPER_H
#define OPENJPH_WRAPPER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Progression orders matching OpenJPH.java constants */
#define OJPH_PROG_LRCP 0
#define OJPH_PROG_RLCP 1
#define OJPH_PROG_RPCL 2
#define OJPH_PROG_PCRL 3
#define OJPH_PROG_CPRL 4

/**
 * Encode parameters for HTJ2K encoding.
 */
typedef struct {
    int width;
    int height;
    int components;
    int bits_per_sample;
    int is_signed;
    int reversible;          /* 1 for lossless (5/3), 0 for lossy (9/7) */
    float compression_ratio; /* e.g. 10.0 for 10:1; 0 for lossless */
    int progression_order;   /* OJPH_PROG_* constant */
    int decompositions;      /* DWT levels, typically 5 */
} ojph_encode_params;

/**
 * Result of HTJ2K encoding.
 */
typedef struct {
    uint8_t *data;    /* Encoded codestream bytes (caller must free with ojph_free_result) */
    size_t size;      /* Size in bytes */
    char *error;      /* Error message if encoding failed (NULL on success) */
} ojph_encode_result;

/**
 * Encode raw pixel data to an HTJ2K codestream.
 *
 * @param raw_data   pixel data in pixel-interleaved layout; for 16-bit data
 *                   stored as little-endian byte pairs (the Java caller is
 *                   responsible for ensuring LE order regardless of the
 *                   original DICOM transfer syntax)
 * @param raw_size   size of raw_data in bytes
 * @param params     encoding parameters
 * @return           result struct; caller must call ojph_free_result()
 */
ojph_encode_result ojph_encode(const uint8_t *raw_data, size_t raw_size,
                               const ojph_encode_params *params);

/**
 * Free memory allocated by ojph_encode.
 */
void ojph_free_result(ojph_encode_result *result);

#ifdef __cplusplus
}
#endif

#endif /* OPENJPH_WRAPPER_H */
