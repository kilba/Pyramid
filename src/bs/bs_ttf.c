#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <bs_ttf.h>
#include <bs_wnd.h>
#include <bs_mem.h>
#include <bs_math.h>
#include <bs_core.h>
#include <bs_shaders.h>
#include <bs_textures.h>

#include <lodepng.h>
#include <cglm/cglm.h>

static void* bs_findTable(bs_Font* ttf, const char tag[4]) {
    const char* table = NULL;
    for (int i = 0; i < ttf->data_len - 4; i++) {
        const char* ptr = ttf->buf + i;
        if (memcmp(ptr, tag, 4) == 0) {
            table = ptr;
        }
    }
    if (table == NULL) {
        bs_callErrorf(BS_ERROR_TTF_TABLE_NOT_FOUND, 2, "TTF Table \"%s\" could not be found!", tag);
    }

    return ttf->buf + bs_memU32(table, 8);
}

static void bs_head(bs_Font* ttf) {
    ttf->head.buf = bs_findTable(ttf, "head");
    ttf->head.units_per_em = bs_memU16(ttf->head.buf, HEAD_UNITS_PER_EM);
    ttf->head.index_to_loc_format = bs_memU16(ttf->head.buf, HEAD_INDEX_TO_LOC_FORMAT);
}

static void bs_maxp(bs_Font* ttf) {
    ttf->maxp.buf = bs_findTable(ttf, "maxp");
    ttf->maxp.num_glyphs = bs_memU16(ttf->maxp.buf, MAXP_NUM_GLYPHS);
}

static void bs_hhea(bs_Font* ttf) {
    ttf->hhea.buf = bs_findTable(ttf, "hhea");
    ttf->hhea.num_of_long_hor_metrics = bs_memU16(ttf->hhea.buf, HHEA_NUM_OF_LONG_HOR_METRICS);
}

static void bs_hmtx(bs_Font* ttf, int id) {
    ttf->hmtx.buf = ttf->hmtx.buf == NULL ? bs_findTable(ttf, "hmtx") : ttf->hmtx.buf;

    bs_U32 offset = id * sizeof(bs_LongHorMetric);
    ttf->glyf.glyphs[id].long_hor_metric.advance_width = bs_memU16(ttf->hmtx.buf, offset);
    ttf->glyf.glyphs[id].long_hor_metric.left_side_bearing = (bs_I16)bs_memU16(ttf->hmtx.buf, offset + 2);
}

static bs_U32 bs_loca(bs_Font* ttf, int id) {
    ttf->loca.buf = ttf->loca.buf == NULL ? bs_findTable(ttf, "loca") : ttf->loca.buf;
    bs_U32 offset = 0;
    if (ttf->head.index_to_loc_format == 0) {
        offset = bs_memU16(ttf->loca.buf, id * sizeof(bs_U16)) * 2;
    }
    else {
        offset = bs_memU32(ttf->loca.buf, id * sizeof(bs_U32));
    }

    return offset;
}

static int bs_glyfCoords(bs_Glyf* glyf, bool y, bs_U8* flags, bs_GlyfPt* pts, int offset, int num_points) {
    bs_U16 coord_prev = 0;
    for(int i = 0; i < num_points; i++) {
        bs_U8 flag = flags[i];
        bs_U16 coord;

	    // If coord is 8-bit
	    if(BS_FLAGSET(flag, (GLYF_X_SHORT + y))) {
	        coord = bs_memU8(glyf->buf, offset);
	        offset += 1;

            if (!BS_FLAGSET(flag, (GLYF_X_SAME + y))) {
                coord = -coord;
            }

	        coord += coord_prev;
	    } else {
	        if(BS_FLAGSET(flag, (GLYF_X_SAME + y))) {
		        coord = coord_prev;
	        } else {
		        coord = bs_memU16(glyf->buf, offset);
		        coord += coord_prev;
		        offset += 2;
	        }
	    }

	    coord_prev = coord;
	    char *data = (char *)(pts + i);
	    bs_U16 *val = (bs_U16*)(data + y * sizeof(bs_U16));

	    *val = coord;
    }
    
    return offset;
}

static void bs_glyf(bs_Font* ttf, int id) {
    ttf->glyf.buf = bs_findTable(ttf, "glyf");

    // get location of glyph in ttf buffer
    bs_U32 loc = bs_loca(ttf, id);
    ttf->glyf.buf += loc;
    bs_Glyph* glyph = ttf->glyf.glyphs + id;

    // get bounding boxes
    glyph->x_min = bs_memU16(ttf->glyf.buf, GLYF_XMIN);
    glyph->y_min = bs_memU16(ttf->glyf.buf, GLYF_YMIN);
    glyph->x_max = bs_memU16(ttf->glyf.buf, GLYF_XMAX);
    glyph->y_max = bs_memU16(ttf->glyf.buf, GLYF_YMAX);

    // parse contours
    int num_contours = bs_memU16(ttf->glyf.buf, GLYF_NUMBER_OF_CONTOURS);
    int* end_pts = bs_alloc(num_contours * sizeof(int));
    for (int i = 0; i < num_contours; i++) {
        end_pts[i] = bs_memU16(ttf->glyf.buf, GLYF_END_PTS_OF_CONTOURS + i * 2);
    }

    // num points is equal to end of last contour
    int num_points = end_pts[num_contours - 1] + 1;

    int instruction_offset = GLYF_INSTRUCTION_LENGTH(num_contours);
    int num_instructions = bs_memU16(ttf->glyf.buf, instruction_offset);

    int flag_offset = GLYF_FLAGS(instruction_offset, num_instructions);
    int flag_offset_original = flag_offset;

    int coord_offset = GLYF_XCOORDS(flag_offset, num_points);

    glyph->coords = bs_alloc(num_points * sizeof(bs_GlyfPt));
    glyph->contours = bs_alloc(num_contours * sizeof(uint16_t));
    glyph->num_contours = num_contours;
    glyph->num_points = num_points;

    for (int i = 0; i < num_contours; i++) {
        ttf->glyf.glyphs[id].contours[i] = end_pts[i];
    }

    int num_repeats_total = 0;
    bs_U8* flags = bs_alloc(num_points);

    // Parse flags
    for(int i = 0; i < num_points; i++) {
	    flags[i] = bs_memU8(ttf->glyf.buf, flag_offset++);

	    // Add repeated flags
	    if(BS_FLAGSET(flags[i], GLYF_REPEAT)) {
	        int num_repeats = bs_memU8(ttf->glyf.buf, flag_offset++);
	        int flag_repeated = flags[i];

	        num_repeats_total += num_repeats - 1;

	        for(int j = 0; j < num_repeats; j++)
		    flags[++i] = flag_repeated;
	    }
    }
    
    coord_offset -= num_repeats_total;

    for (int i = 0; i < num_points; i++) {
        glyph->coords[i].on_curve = BS_FLAGSET(flags[i], GLYF_ON_CURVE);
    }

    // Get X and Y coords
    coord_offset = bs_glyfCoords(&ttf->glyf, false, flags, glyph->coords, coord_offset, num_points);
    coord_offset = bs_glyfCoords(&ttf->glyf, true , flags, glyph->coords, coord_offset, num_points);

    free(end_pts);
    free(flags);
}

static int bs_glyphIndex(bs_Font* ttf, bs_U16 code) {
    int index = -1;
    bs_U16* ptr = NULL;

    for (int i = 0; i < ttf->cmap.seg_count_x2 / 2; i++) {
        if (ttf->cmap.end_code[i] > code) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        bs_callErrorf(BS_ERROR_TTF_GLYPH_NOT_FOUND, 2, "TTF glyph with code \"%d\" could not be found", code);
        return 0;
    }

    if (ttf->cmap.start_code[index] < code) {
        if (ttf->cmap.id_range_offset[index] == 0) {
            return code + ttf->cmap.id_delta[index];
        }

        ptr = ttf->cmap.id_range_offset + index + ttf->cmap.id_range_offset[index] / 2;
        ptr += code - ttf->cmap.start_code[index];
        if (*ptr == 0) return 0;
        return *ptr + ttf->cmap.id_delta[index];
    }

    return 0;
}

bs_Glyph* bs_glyph(bs_Font* font, char c) {
    return font->glyf.glyphs + bs_glyphIndex(font, c);
}

static bs_U16 bs_format4(bs_Font* ttf) {
    ttf->cmap.length = bs_memU16(ttf->cmap.subtable, CMAP_FORMAT4_LENGTH);
    ttf->cmap.seg_count_x2 = bs_memU16(ttf->cmap.subtable, CMAP_FORMAT4_NUM_SEGMENTS_X2);

    bs_U16 seg_count = ttf->cmap.seg_count_x2 / 2;

    ttf->cmap.format_data = calloc(1, ttf->cmap.length);
    ttf->cmap.end_code        = ttf->cmap.format_data + seg_count * 0;
    ttf->cmap.start_code      = ttf->cmap.format_data + seg_count * 1;
    ttf->cmap.id_delta        = ttf->cmap.format_data + seg_count * 2;
    ttf->cmap.id_range_offset = ttf->cmap.format_data + seg_count * 3;

    for (int i = 0; i < seg_count; i++) {
        bs_U16 end_code        = bs_memU16(ttf->cmap.subtable, CMAP_FORMAT4_END_CODE + i * sizeof(bs_U16));
        bs_U16 start_code      = bs_memU16(ttf->cmap.subtable, CMAP_FORMAT4_START_CODE(seg_count) + i * sizeof(bs_U16));
        bs_I16 id_delta        = bs_memU16(ttf->cmap.subtable, CMAP_FORMAT4_ID_DELTA(seg_count) + i * sizeof(bs_U16));
        bs_U16 id_range_offset = bs_memU16(ttf->cmap.subtable, CMAP_FORMAT4_ID_RANGE_OFFSET(seg_count) + i * sizeof(bs_U16));

        ttf->cmap.end_code       [i] = end_code;
        ttf->cmap.start_code     [i] = start_code;
        ttf->cmap.id_delta       [i] = id_delta;
        ttf->cmap.id_range_offset[i] = id_range_offset;
    }

    return 0;
}

static void bs_cmap(bs_Font* ttf) {
    ttf->cmap.buf = bs_findTable(ttf, "cmap");
    ttf->cmap.num_subtables = bs_memU16(ttf->cmap.buf, CMAP_NUMBER_SUBTABLES);

    int subtable_offset = -1;
    for (int i = 0; i < ttf->cmap.num_subtables; i++) {
        bs_U32 platform_id          = bs_memU16(ttf->cmap.buf, CMAP_PLATFORM_ID + CMAP_TABLE_SIZE * i);
        bs_U32 platform_specific_id = bs_memU16(ttf->cmap.buf, CMAP_PLATFORM_SPECIFIC_ID + CMAP_TABLE_SIZE * i);
        subtable_offset             = bs_memU32(ttf->cmap.buf, CMAP_OFFSET + CMAP_TABLE_SIZE * i);
        if (platform_id == 0) break;
    }

    if (subtable_offset == -1) {
        bs_callErrorf(BS_ERROR_TTF_UNSUPPORTED_ENCODING, 2, "TTF Only supports Unicode");
    }

    ttf->cmap.subtable = ttf->cmap.buf + subtable_offset;
    ttf->cmap.format = bs_memU16(ttf->cmap.subtable, CMAP_SUBTABLE_FORMAT);

    switch (ttf->cmap.format) {
    case 4: bs_format4(ttf); break;
    default: {
        bs_callErrorf(BS_ERROR_TTF_UNSUPPORTED_FORMAT, 2, "Format %d not supported", ttf->cmap.format);
    } break;
    }
}

static int bs_cmpFloats(const void *a, const void *b) {
    float f_a = *(float *)a;
    float f_b = *(float *)b;

    if(f_a < f_b) return -1;
    else if(f_a > f_b) return 1;

    return 0;
}
int bs_glyphWindingOrder(bs_Glyph* glyph, int contourIndex, bs_vec2 point, bs_vec2* pts, int num_pts) {
    int windingNumber = 0;

    for (int i = 0; i < num_pts; ++i) {
        bs_vec2 pt1 = pts[i];
        bs_vec2 pt2 = pts[(i + 1) % num_pts];

        if (pt1.y <= point.y && pt2.y > point.y) {
            float intersectX = pt1.x + (point.y - pt1.y) * (pt2.x - pt1.x) / (pt2.y - pt1.y);
            if (intersectX > point.x) {
                windingNumber++;
            }
        }
        else if (pt1.y > point.y && pt2.y <= point.y) {
            float intersectX = pt1.x + (point.y - pt1.y) * (pt2.x - pt1.x) / (pt2.y - pt1.y);
            if (intersectX > point.x) {
                windingNumber--;
            }
        }
    }

    return windingNumber;
}

static void bs_calculateContourCurves(bs_Glyf* glyf, int idx, int contour, bs_vec2* out, bs_U32* out_num_pts) {
    bs_Glyph* glyph = glyf->glyphs + idx;

#define BS_TTF_DETAIL 4

    uint16_t first = (contour == 0) ? 0 : glyph->contours[contour - 1] + 1;
    uint16_t last = glyph->contours[contour] + 1;

    for (int j = first; j < last; j++) {
        bs_GlyfPt curr = glyph->coords[j];
        bs_GlyfPt curr_off, next_off;

        int idx;
        if ((j + 1) >= last) idx = (j + 1) - last + first;
        else                 idx = (j + 1);

        curr_off = glyph->coords[idx];
        if (!curr.on_curve) continue;

        bs_vec2 curr_off_v, next_off_v, curr_v = bs_v2(curr.x, curr.y);
        if (curr_off.on_curve) {
            out[(*out_num_pts)++] = curr_v;
            continue;
        }

        while (!(curr_off = glyph->coords[idx]).on_curve) {
            next_off = glyph->coords[((j + 2) >= last) ? ((j + 2) - last + first) : (j + 2)];
            if (next_off.on_curve) break;

            curr_off_v = bs_v2(curr_off.x, curr_off.y);
            next_off_v = bs_v2(next_off.x, next_off.y);

            bs_vec2 mid = bs_v2mid(curr_off_v, next_off_v);

            bs_vec2 elems[BS_TTF_DETAIL + 1];
            bs_v2QuadBez(curr_v, curr_off_v, mid, elems, BS_TTF_DETAIL);
            elems[BS_TTF_DETAIL] = mid;
            for (int i = 0; i < BS_TTF_DETAIL; i++) {
                out[(*out_num_pts)++] = elems[i];
            }

            curr_v = mid;
            j++;
            if ((j + 1) >= last) idx = (j + 1) - last + first;
            else                 idx = (j + 1);
        }

        curr_off = glyph->coords[((j + 1) >= last) ? ((j + 1) - last + first) : (j + 1)];
        next_off = glyph->coords[((j + 2) >= last) ? ((j + 2) - last + first) : (j + 2)];
        curr_off_v = bs_v2(curr_off.x, curr_off.y);
        next_off_v = bs_v2(next_off.x, next_off.y);
        double t = 0.0;
        double incr;

        incr = 1.0 / (double)BS_TTF_DETAIL;

        for (int k = 0; k < BS_TTF_DETAIL; k++, t += incr) {
            bs_vec2 v;
            v.x = (1 - t) * (1 - t) * curr_v.x + 2 * (1 - t) * t * curr_off_v.x + t * t * next_off_v.x;
            v.y = (1 - t) * (1 - t) * curr_v.y + 2 * (1 - t) * t * curr_off_v.y + t * t * next_off_v.y;
            out[(*out_num_pts)++] = v;
        }
    }
    out[(*out_num_pts)++] = bs_v2(glyph->coords[first].x, glyph->coords[first].y);
}

static void bs_rasterizeGlyph(bs_Glyf* glyf, int idx, char* out_bmp, int dim, int offset, float scale) {
    int num_pts = glyf->glyphs[idx].num_points;
    bs_Glyph* glyph = glyf->glyphs + idx;

    bs_vec2 out[BS_TTF_MAX_PTS];
    bs_U32 out_num_pts = 0;

    unsigned char bmp[BS_TTF_DIM * BS_TTF_DIM] = { 0 };

    for (int i = 0; i < glyph->num_contours; i++) {
        bs_U32 num_pts = 0;
        bs_vec2* pts = out + out_num_pts;
        bs_calculateContourCurves(glyf, idx, i, pts, &num_pts);
        out_num_pts += num_pts;

        for (int j = 0; j < num_pts; j++) {
            pts[j].x = (pts[j].x - glyph->x_min) * scale;
            pts[j].y = (pts[j].y - glyph->y_min) * scale;
        }

        for (int y = 0; y < BS_TTF_DIM; y++) {
            float intersections[256];
            int num_intersections = 0;

            for (int j = 0; j < num_pts; j++) {
                bs_vec2 pt0 = pts[j];
                bs_vec2 pt1 = pts[(j + 1) % num_pts];

                if ((y < fmin(pt0.y, pt1.y)) || (y >= fmax(pt0.y, pt1.y))) continue;

                float dy = pt1.y - pt0.y;
                if (dy != 0) {
                    float dx = pt1.x - pt0.x;
                    float x_intersect = pt0.x + (y - pt0.y) * dx / dy;
                    intersections[num_intersections++] = x_intersect;
                }
            }

            qsort(intersections, num_intersections, sizeof(float), bs_cmpFloats);

            for (int j = 0; j < num_intersections - 1; j += 2) {
                int start = ceil(intersections[j]);
                int end = floor(intersections[j + 1]);

                for (int x = start; x <= end; x++) {
                    if (x >= 0 && x < BS_TTF_DIM) {
                        int z = bs_glyphWindingOrder(glyph, i, bs_v2(x, y), pts, num_pts);

                        out_bmp[y * dim + x + offset] = z == -1 ? 255 : 0;
                    }
                }
            }
        }
    }
}

bs_Font bs_loadFont(const char* path, const char* alphabet, const char* output_name) {
    bs_Font ttf = { 0 };
    const char* buf = bs_loadFile(path, &ttf.data_len);

    ttf.buf = buf;
    ttf.glyf.buf = NULL;

    // metadata gathering
    ttf.table_count = bs_memU16(ttf.buf, 4);

    // table gathering
    bs_head(&ttf);
    bs_maxp(&ttf);
    bs_hhea(&ttf);
    bs_cmap(&ttf);

    int len = strlen(alphabet);
    int dim = ceil(bs_sqrt(len * BS_TTF_DIM * BS_TTF_DIM));
    const char* atlas = calloc(1, dim * dim);
    int y_offset = 0;

    ttf.scale = BS_TTF_DIM;
    for (int i = 0; i < len; i++) {
        char code = alphabet[i];
        int idx = bs_glyphIndex(&ttf, code);

        bs_glyf(&ttf, idx);
        bs_Glyph* glyph = ttf.glyf.glyphs + idx;

        float scale = bs_min(
            (float)BS_TTF_DIM / (float)(glyph->x_max - glyph->x_min),
            (float)BS_TTF_DIM / (float)(glyph->y_max - glyph->y_min)
        );
        ttf.scale = bs_min(ttf.scale, scale);
    }

    for (int i = 0, x_offset = 0; i < len; i++) {
        char code = alphabet[i];
        int idx = bs_glyphIndex(&ttf, code);

        bs_glyf(&ttf, idx);
        bs_Glyph* glyph = ttf.glyf.glyphs + idx;
        bs_hmtx(&ttf, idx);

        glyph->width = (float)(glyph->x_max * ttf.scale - glyph->x_min * ttf.scale);

        if (x_offset + ceil(glyph->width) >= dim) {
            x_offset = 0;
            y_offset++;
        }

        glyph->tex_offset.x = (float)x_offset / (float)dim;
        glyph->tex_offset.y = (float)y_offset * BS_TTF_DIM / (float)dim;

        float x = ceil(glyph->width) / (float)dim;
        glyph->tex_coord.x = glyph->tex_offset.x + x;
        glyph->tex_coord.y = glyph->tex_offset.y + BS_TTF_DIM / (float)dim;

        bs_rasterizeGlyph(&ttf.glyf, idx, atlas, dim, x_offset + y_offset * dim * BS_TTF_DIM + dim, ttf.scale);
        x_offset += ceil(glyph->width);
    }

    if (output_name != NULL) {
        lodepng_encode_file(output_name, atlas, dim, dim, LCT_GREY, 8);
    }

    bs_textureR(&ttf.texture, bs_v2s(dim), atlas);

    free(buf);
    return ttf;
}