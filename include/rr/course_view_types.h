#ifndef RR_COURSE_VIEW_TYPES_H
#define RR_COURSE_VIEW_TYPES_H
#include "rr/layout.h"

/* Each populated record range continues the preceding record's range in the
 * corresponding primitive stream. The first four words are unused in PAL data. */
typedef struct {u16 count, first;} CoursePrimitiveRange;
/* Relocation transports serialized byte addresses, including unaligned ones.
 * The descriptor is a wire record; consumers of actual PAL geometry use its
 * aligned file instances. Keep its pointer transport valid for every byte. */
typedef struct {
    u32 reserved00[4];
    CoursePrimitiveRange ranges10[16];
} RR_PACKED CourseViewRecord;

/* On-disc visibility: a word count followed by halfword record indices. */
typedef struct {
    u32 count;
#if defined(__GNUC__) && __GNUC__ < 3
    u16 indices[0];
#else
    u16 indices[];
#endif
} CourseViewIndices;

/* PAL storage ends at the root-pointer symbol exactly0x400 bytes later. */
enum {COURSE_ORIGINAL_VISIBLE_CAPACITY=255};
#if defined(mips)
enum {COURSE_VISIBLE_CAPACITY=COURSE_ORIGINAL_VISIBLE_CAPACITY};
#else
/* Native visibility can include every u16-indexed course record. */
enum {COURSE_VISIBLE_CAPACITY=65536};
#endif
typedef union {u32 count;CourseViewRecord *record;} CourseViewListSlot;
/* The handwritten renderer consumes count followed by pointer-sized slots.
 * C also uses the same owner as a counted array of records. Both views retain
 * complete native pointers and the same physical count/pointer locations. */
typedef union {
    struct {u32 count;CourseViewRecord *records[COURSE_VISIBLE_CAPACITY];} objects;
    CourseViewListSlot stream[COURSE_VISIBLE_CAPACITY+1];
} VisibleCourseViews;
typedef struct {u8 count;u8 indices[31];} AdditionalCourseViews;

RR_ASSERT_SIZE(CoursePrimitiveRange,4);
RR_ASSERT_SIZE(CourseViewRecord,0x50);
RR_ASSERT_OFFSET(CourseViewRecord,ranges10,0x10);
RR_ASSERT_SIZE(CourseViewIndices,4);
RR_ASSERT_SIZE(AdditionalCourseViews,32);
RR_ASSERT_SIZE(CourseViewListSlot,sizeof(void *));
typedef char rr_visible_record_array_start[
    RR_FIELD_OFFSET(VisibleCourseViews,objects.records)==sizeof(CourseViewListSlot)?1:-1];
#if defined(mips)
RR_ASSERT_SIZE(VisibleCourseViews,0x400);
#endif
#endif
