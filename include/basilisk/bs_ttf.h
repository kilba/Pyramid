#ifndef BS_TTF_H
#define BS_TTF_H

#include <bs_types.h>

#define BS_TTF_DETAIL 4
#define BS_TTF_MAX_PTS 1024
#define BS_TTF_DIM 64

bs_Font bs_loadFont(const char* path, const char* alphabet, const char* output_name);
bs_Glyph* bs_glyph(bs_Font* font, char c);

// HEAD table offsets
#define HEAD_VERSION                    0   // 4 | fixed
#define HEAD_FONT_REVISION              4   // 4 | fixed
#define HEAD_CHECK_SUM_ADJUSTMENT       8   // 4 | uint32
#define HEAD_MAGIC_NUMBER               12  // 4 | uint32
#define HEAD_FLAGS                      16  // 2 | uint16
#define HEAD_UNITS_PER_EM               18  // 2 | uint16
#define HEAD_CREATED                    20  // 8 | longDateTime
#define HEAD_MODIFIED                   28  // 8 | longDateTime
#define HEAD_X_MIN                      36  // 2 | fWord
#define HEAD_Y_MIN                      38  // 2 | fWord
#define HEAD_X_MAX                      40  // 2 | fWord
#define HEAD_Y_MAX                      42  // 2 | fWord
#define HEAD_MAC_STYLE                  44  // 2 | uint16
#define HEAD_LOWEST_REC_PPEM            46  // 2 | uint16
#define HEAD_FONT_DIRECTION_HINT        48  // 2 | int16
#define HEAD_INDEX_TO_LOC_FORMAT        50  // 2 | int16
#define HEAD_GLYPH_DATA_FORMAT          52  // 2 | int16

// MAXP table offsets
#define MAXP_VERSION                    0   // 4 | fixed
#define MAXP_NUM_GLYPHS                 4   // 2 | uint16
#define MAXP_MAX_POINTS                 6   // 2 | uint16
#define MAXP_MAX_CONTOURS               8   // 2 | uint16
#define MAXP_MAX_COMPONENT_POINTS       10  // 2 | uint16
#define MAXP_MAX_COMPONENT_CONTOURS     12  // 2 | uint16
#define MAXP_MAX_ZONES                  14  // 2 | uint16
#define MAXP_MAX_TWILIGHT_POINTS        16  // 2 | uint16
#define MAXP_MAX_STORAGE                18  // 2 | uint16
#define MAXP_MAX_FUNCTION_DEFS          20  // 2 | uint16
#define MAXP_MAX_INSTRUCTION_DEFS       22  // 2 | uint16
#define MAXP_MAX_STACK_ELEMENTS         24  // 2 | uint16
#define MAXP_MAX_SIZE_OF_INSTRUCTIONS   26  // 2 | uint16
#define MAXP_MAX_COMPONENT_ELEMENTS     28  // 2 | uint16
#define MAXP_MAX_COMPONENT_DEPTH        30  // 2 | uint16

// HHEA table offsets
#define HHEA_VERSION                    0   // 4 | fixed
#define HHEA_ASCENT                     4   // 2 | fWord
#define HHEA_DESCENT                    6   // 2 | fWord
#define HHEA_LINE_GAP                   8   // 2 | fWord
#define HHEA_ADVANCE_WIDTH_MAX          10  // 2 | ufWord
#define HHEA_MIN_LEFT_SIDE_BEARING      12  // 2 | fWord
#define HHEA_MIN_RIGHT_SIDE_BEARING     14  // 2 | fWord
#define HHEA_X_MAX_EXTENT               16  // 2 | fWord
#define HHEA_CARET_SLOPE_RISE           18  // 2 | int16
#define HHEA_CARET_SLOPE_RUN            20  // 2 | int16
#define HHEA_CARET_OFFSET               22  // 2 | fWord
// 8 bytes reserved here
#define HHEA_METRIC_DATA_FORMAT         32  // 2 | int16
#define HHEA_NUM_OF_LONG_HOR_METRICS    34  // 2 | uint16

// HMTX table offsets
#define HMTX_LONG_HOR_METRICS			0   // 4 | uin16, int16 ARRAY
// #define HMTX_LEFT_SIDE_BEARING			0   // 2 | fWord ARRAY

// GLYF table offsets
#define GLYF_NUMBER_OF_CONTOURS             0   // 2 | int16
#define GLYF_XMIN                           2   // 2 | fWord
#define GLYF_YMIN                           4   // 2 | fWord
#define GLYF_XMAX                           6   // 2 | fWord
#define GLYF_YMAX                           8   // 2 | fWord
#define GLYF_END_PTS_OF_CONTOURS            10  // 2 | uint16 ARRAY
#define GLYF_INSTRUCTION_LENGTH(contours)   GLYF_END_PTS_OF_CONTOURS + contours * 2 // 2 | uint16
#define GLYF_INSTRUCTIONS                   10                                      // 1 | uint8  ARRAY
#define GLYF_FLAGS(offset, instructions)    offset + instructions + 2               // 1 | uint8  ARRAY

// GLYF flags
#define GLYF_ON_CURVE   0
#define GLYF_X_SHORT    1
#define GLYF_Y_SHORT    2
#define GLYF_REPEAT     3
#define GLYF_X_SAME     4
#define GLYF_Y_SAME     5
#define GLYF_OVERLAP    6

#define GLYF_XCOORDS(offset, num_flags) offset + num_flags                  // 1 | uint8  ARRAY
#define GLYF_YCOORDS(offset, num_flags, coord) offset + num_flags + coord   // 1 | uint8  ARRAY

// CMAP table offsets
#define CMAP_VERSION                    0   // 2 | uint16
#define CMAP_NUMBER_SUBTABLES           2   // 2 | uint16

// field variables
#define CMAP_PLATFORM_ID                4   // 2 | uint16
#define CMAP_PLATFORM_SPECIFIC_ID       6   // 2 | uint16
#define CMAP_OFFSET                     8   // 4 | uint32
#define CMAP_TABLE_SIZE                 (2 + 2 + 4)

// formats
#define CMAP_SUBTABLE_FORMAT            0   // 2 | uint16

// #define CMAP_FORMAT0_LENGTH             2   // 2 | uint16
// #define CMAP_FORMAT0_LANGUAGE           4   // 2 | uint16
// #define CMAP_FORMAT0_GLYPH_ARRAY        6   // 256 | uint8
// #define CMAP_FORMAT0_GLYPH_ARRAY_SIZE   6   // 256 | uint8

#define CMAP_FORMAT4_LENGTH             2
#define CMAP_FORMAT4_LANGUAGE           4
#define CMAP_FORMAT4_NUM_SEGMENTS_X2   6
#define CMAP_FORMAT4_SEARCH_RANGE       8
#define CMAP_FORMAT4_ENTRY_SELECTOR     10
#define CMAP_FORMAT4_RANGE_SHIFT        12
#define CMAP_FORMAT4_END_CODE           14
// 2 bytes reserved here
#define CMAP_FORMAT4_START_CODE(num_segments) (14 + 2 + num_segments * sizeof(bs_U16))
#define CMAP_FORMAT4_ID_DELTA(num_segments) (CMAP_FORMAT4_START_CODE(num_segments) + num_segments * sizeof(bs_U16))
#define CMAP_FORMAT4_ID_RANGE_OFFSET(num_segments) (CMAP_FORMAT4_ID_DELTA(num_segments) + num_segments * sizeof(bs_U16))
#define CMAP_FORMAT4_GLYPH_ARRAY(num_segments) (CMAP_FORMAT4_ID_RANGE_OFFSET(num_segments) + num_segments * sizeof(bs_U16))

#endif // BS_TTF_H