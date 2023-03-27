/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#ifndef qed_common_h
#define qed_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DEBUG_PRINT_CODE
//#define DEBUG_TRACE_EXECUTION

#define UINT8_COUNT (UINT8_MAX + 1)

// put above ATTRIBUTE_COLOR all size-related attributes
#define UI_ATTRIBUTES_DEF \
    UI_ATTRIBUTE_DEF( ATTRIBUTE_OUT, "out", NULL ), \
    UI_ATTRIBUTE_DEF( ATTRIBUTE_SIZE, "size", convertToPoint ), \
    UI_ATTRIBUTE_DEF( ATTRIBUTE_AREA_HERITABLE, "", NULL ), \
    UI_ATTRIBUTE_DEF( ATTRIBUTE_ZOOM, "zoom", convertToFloat ), \
    UI_ATTRIBUTE_DEF( ATTRIBUTE_FONTSIZE, "fontSize", convertToInt ), \
    UI_ATTRIBUTE_DEF( ATTRIBUTE_AREA_END, "", NULL ), \
    UI_ATTRIBUTE_DEF( ATTRIBUTE_ALIGN, "align", convertToFloatPoint ), \
    UI_ATTRIBUTE_DEF( ATTRIBUTE_EXPAND, "expand", convertToFloatPoint ), \
    UI_ATTRIBUTE_DEF( ATTRIBUTE_POS, "pos", convertToPoint ), \
    UI_ATTRIBUTE_DEF( ATTRIBUTE_HERITABLE, "", NULL ), \
    UI_ATTRIBUTE_DEF( ATTRIBUTE_COLOR, "color", convertToInt ), \
    UI_ATTRIBUTE_DEF( ATTRIBUTE_OPACITY, "opacity", convertToFloat ), \
    UI_ATTRIBUTE_DEF( ATTRIBUTE_END, NULL, NULL ), \

#define UI_ATTRIBUTE_DEF( identifier, text, conversionFunction )  identifier
typedef enum { UI_ATTRIBUTES_DEF } Attribute;
#undef UI_ATTRIBUTE_DEF

#define UI_EVENTS_DEF \
    UI_EVENT_DEF( EVENT_PRESS,"onPress" ), \
    UI_EVENT_DEF( EVENT_RELEASE, "onRelease" ), \
    UI_EVENT_DEF( EVENT_END, NULL ), \

#define UI_EVENT_DEF( identifier, text )  identifier
typedef enum { UI_EVENTS_DEF } Event;
#undef UI_EVENT_DEF

#endif