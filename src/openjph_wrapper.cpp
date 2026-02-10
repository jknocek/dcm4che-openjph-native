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

#include "openjph_wrapper.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <stdexcept>

#include "ojph_codestream.h"
#include "ojph_file.h"
#include "ojph_params.h"
#include "ojph_mem.h"

/**
 * A simple in-memory output target for OpenJPH's codestream.
 */
class jni_mem_outfile : public ojph::outfile_base {
public:
    jni_mem_outfile() : buf_(nullptr), size_(0), capacity_(0) {}

    ~jni_mem_outfile() override {
        if (buf_) {
            free(buf_);
            buf_ = nullptr;
        }
    }

    void open() {
        size_ = 0;
        capacity_ = 65536;
        buf_ = (uint8_t*)malloc(capacity_);
        if (!buf_)
            throw std::runtime_error("Failed to allocate output buffer");
    }

    size_t write(const void *ptr, size_t size) override {
        if (size_ + size > capacity_) {
            while (size_ + size > capacity_)
                capacity_ *= 2;
            uint8_t *tmp = (uint8_t*)realloc(buf_, capacity_);
            if (!tmp)
                throw std::runtime_error("Failed to grow output buffer");
            buf_ = tmp;
        }
        memcpy(buf_ + size_, ptr, size);
        size_ += size;
        return size;
    }

    ojph::si64 tell() override {
        return (ojph::si64)size_;
    }

    void close() override {}

    uint8_t *release(size_t *out_size) {
        uint8_t *p = buf_;
        *out_size = size_;
        buf_ = nullptr;
        size_ = 0;
        capacity_ = 0;
        return p;
    }

private:
    uint8_t *buf_;
    size_t size_;
    size_t capacity_;
};

static char *strdup_safe(const char *s) {
    if (!s) return nullptr;
    size_t len = strlen(s) + 1;
    char *d = (char*)malloc(len);
    if (d) memcpy(d, s, len);
    return d;
}

ojph_encode_result ojph_encode(const uint8_t *raw_data, size_t raw_size,
                               const ojph_encode_params *params) {
    ojph_encode_result result;
    memset(&result, 0, sizeof(result));

    try {
        ojph::ui32 width = (ojph::ui32)params->width;
        ojph::ui32 height = (ojph::ui32)params->height;
        ojph::ui32 comps = (ojph::ui32)params->components;
        ojph::ui32 bps = (ojph::ui32)params->bits_per_sample;
        bool is_signed = params->is_signed != 0;
        bool reversible = params->reversible != 0;
        int bytes_per_sample = (bps <= 8) ? 1 : 2;

        size_t expected_size = (size_t)width * height * comps * bytes_per_sample;
        if (raw_size < expected_size) {
            result.error = strdup_safe("Raw data size is smaller than expected");
            return result;
        }

        ojph::codestream codestream;

        // SIZ parameters
        ojph::param_siz siz = codestream.access_siz();
        siz.set_image_extent(ojph::point(width, height));
        siz.set_num_components(comps);
        for (ojph::ui32 c = 0; c < comps; c++) {
            siz.set_component(c, ojph::point(1, 1), bps, is_signed);
        }
        siz.set_image_offset(ojph::point(0, 0));
        siz.set_tile_size(ojph::size(width, height));
        siz.set_tile_offset(ojph::point(0, 0));

        // COD parameters
        ojph::param_cod cod = codestream.access_cod();
        cod.set_num_decomposition((ojph::ui32)params->decompositions);
        cod.set_reversible(reversible);

        switch (params->progression_order) {
            case OJPH_PROG_LRCP: cod.set_progression_order("LRCP"); break;
            case OJPH_PROG_RLCP: cod.set_progression_order("RLCP"); break;
            case OJPH_PROG_RPCL: cod.set_progression_order("RPCL"); break;
            case OJPH_PROG_PCRL: cod.set_progression_order("PCRL"); break;
            case OJPH_PROG_CPRL: cod.set_progression_order("CPRL"); break;
            default:             cod.set_progression_order("LRCP"); break;
        }

        // Enable color transform for multi-component images (RCT for lossless,
        // ICT for lossy). dcm4che's Transcoder always delivers RGB pixel data to
        // the encoder for 3-component images, handling any YBR->RGB conversion
        // during decompression. After encoding, Transcoder.adjustDataset() updates
        // PhotometricInterpretation to YBR_RCT or YBR_ICT accordingly.
        cod.set_color_transform(comps >= 3);

        // For RPCL, set precincts to enable resolution-level random access
        if (params->progression_order == OJPH_PROG_RPCL) {
            int num_precincts = params->decompositions + 1;
            ojph::size precincts[34]; // max decompositions is 33
            for (int i = 0; i < num_precincts; i++) {
                precincts[i] = ojph::size(256, 256);
            }
            cod.set_precinct_size(num_precincts, precincts);
        }

        cod.set_block_dims(64, 64);

        // Lossy quality settings
        if (!reversible && params->compression_ratio > 0.0f) {
            float rate = 1.0f / params->compression_ratio;
            float bpp = (float)(bps * comps) * rate;
            codestream.access_qcd().set_irrev_quant(bpp);
        }

        // Write codestream
        jni_mem_outfile output;
        output.open();

        codestream.write_headers(&output);

        // Push lines using exchange() protocol:
        // 1. exchange(NULL, next_comp) to get first buffer
        // 2. Fill buffer with data for component next_comp
        // 3. exchange(filled_buf, next_comp) to return it and get next
        ojph::ui32 next_comp;
        ojph::line_buf *cur_line = codestream.exchange(NULL, next_comp);

        for (ojph::ui32 y = 0; y < height; y++) {
            for (ojph::ui32 c = 0; c < comps; c++) {
                if (bytes_per_sample == 1) {
                    ojph::si32 *dp = cur_line->i32;
                    const uint8_t *src_row = raw_data + (size_t)y * width * comps;
                    for (ojph::ui32 x = 0; x < width; x++) {
                        int val = src_row[x * comps + next_comp];
                        if (is_signed) val = (int)(int8_t)val;
                        dp[x] = val;
                    }
                } else {
                    ojph::si32 *dp = cur_line->i32;
                    const uint8_t *src_row = raw_data + (size_t)y * width * comps * 2;
                    for (ojph::ui32 x = 0; x < width; x++) {
                        int offset = (x * comps + next_comp) * 2;
                        int val = (int)src_row[offset] | ((int)src_row[offset + 1] << 8);
                        if (is_signed) val = (int)(int16_t)(uint16_t)val;
                        dp[x] = val;
                    }
                }
                cur_line = codestream.exchange(cur_line, next_comp);
            }
        }

        codestream.flush();
        codestream.close();

        result.data = output.release(&result.size);
    } catch (const std::exception &e) {
        result.error = strdup_safe(e.what());
    } catch (...) {
        result.error = strdup_safe("Unknown error during HTJ2K encoding");
    }

    return result;
}

void ojph_free_result(ojph_encode_result *result) {
    if (result) {
        if (result->data) {
            free(result->data);
            result->data = nullptr;
        }
        if (result->error) {
            free(result->error);
            result->error = nullptr;
        }
        result->size = 0;
    }
}
