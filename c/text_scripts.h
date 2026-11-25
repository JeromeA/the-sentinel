#ifndef SENTINEL_TEXT_SCRIPTS_H
#define SENTINEL_TEXT_SCRIPTS_H
#include <stdint.h>

/* Keep IDs full-width: unknown and sign-extended nested tokens are rejected
 * by the interpreter, not truncated to a valid byte-sized script ID. */
typedef uint16_t TextScriptId;
_Static_assert(sizeof(TextScriptId) == 2, "text script ID width");

enum TextScript {
    TEXT_SCRIPT_TITLE_PROMPT = 0,
    TEXT_SCRIPT_LANDSCAPE_NUMBER_PROMPT = 1,
    TEXT_SCRIPT_SECRET_CODE_PROMPT = 2,
    TEXT_SCRIPT_WRONG_SECRET_CODE = 3,
    TEXT_SCRIPT_PREVIEW_CAPTION = 4,
    TEXT_SCRIPT_COMPLETION_CAPTION = 5,
    TEXT_SCRIPT_COMPLETION_PROMPT = 6,
    TEXT_SCRIPT_POSITION_TOP_LEFT = 7,
    TEXT_SCRIPT_POSITION_TOP_INSET = 8,
    TEXT_SCRIPT_POSITION_BOTTOM_INSET = 9,
    TEXT_SCRIPT_PROMPT_STYLE = 10,
    TEXT_SCRIPT_CAPTION_STYLE = 11,
    TEXT_SCRIPT_POSITION_BOTTOM_LEFT = 12,
    TEXT_SCRIPT_LANDSCAPE_LABEL = 13,
    TEXT_SCRIPT_SECRET_ENTRY_CODE_LABEL = 14,
    TEXT_SCRIPT_SKIP_FIVE_GLYPHS = 15,
    TEXT_SCRIPT_SKIP_THREE_GLYPHS = 16,
    TEXT_SCRIPT_PRESS_ANY_KEY = 17,
    TEXT_SCRIPT_COUNT = 18
};

/* Embedded calls in the readable byte bank. Each literal is one byte at
 * TEXT_SCRIPT_CALL_BASE (200) + its catalog ID. Keep string tokens separate
 * from following text so hexadecimal escapes cannot consume its letters. */
#define TEXT_CALL_TITLE_PROMPT "\xc8"
#define TEXT_CALL_POSITION_TOP_LEFT "\xcf"
#define TEXT_CALL_POSITION_TOP_INSET "\xd0"
#define TEXT_CALL_POSITION_BOTTOM_INSET "\xd1"
#define TEXT_CALL_PROMPT_STYLE "\xd2"
#define TEXT_CALL_CAPTION_STYLE "\xd3"
#define TEXT_CALL_POSITION_BOTTOM_LEFT "\xd4"
#define TEXT_CALL_LANDSCAPE_LABEL "\xd5"
#define TEXT_CALL_SECRET_ENTRY_CODE_LABEL "\xd6"
#define TEXT_CALL_SKIP_FIVE_GLYPHS "\xd7"
#define TEXT_CALL_SKIP_THREE_GLYPHS "\xd8"
#define TEXT_CALL_PRESS_ANY_KEY "\xd9"

/* Positions are (column,row) before the interpreter doubles the column byte:
 * top-left (1,1), top-inset (3,3), bottom-inset (3,23), bottom-left (1,22).
 * Prompt/caption style scripts intentionally have identical bytes but distinct IDs. */
#endif
