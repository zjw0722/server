/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
// vim: ft=cpp:expandtab:ts=8:sw=4:softtabstop=4:
#ident "$Id$"
#ident "Copyright (c) 2007-2012 Tokutek Inc.  All rights reserved."
#ident "The technology is licensed by the Massachusetts Institute of Technology, Rutgers State University of New Jersey, and the Research Foundation of State University of New York at Stony Brook under United States of America Serial No. 11/760379 and to the patents and/or patent applications resulting from it."

#ifndef RANGE_BUFFER_H
#define RANGE_BUFFER_H

#include <toku_stdint.h>

#include <ft/ybt.h>

namespace toku {

// a key range buffer represents a set of key ranges that can
// be stored, iterated over, and then destroyed all at once.

class range_buffer {
private:

    // the key range buffer is a bunch of records in a row.
    // each record has the following header, followed by the
    // left key and right key data payload, if applicable.

    struct record_header {
        bool left_neg_inf;
        bool left_pos_inf;
        bool right_pos_inf;
        bool right_neg_inf;
        uint32_t left_key_size;
        uint32_t right_key_size;

        bool left_is_infinite(void) const;

        bool right_is_infinite(void) const;

        void init(const DBT *left_key, const DBT *right_key);
    };
    static_assert(sizeof(record_header) == 12, "record header format is off");
    
public:

    // the iterator abstracts reading over a buffer of variable length
    // records one by one until there are no more left.

    class iterator {
    public:

        // a record represents the user-view of a serialized key range.
        // it handles positive and negative infinity and the optimized
        // point range case, where left and right points share memory.

        class record {
        public:
            // get a read-only pointer to the left key of this record's range
            const DBT *get_left_key(void) const;

            // get a read-only pointer to the right key of this record's range
            const DBT *get_right_key(void) const;

            // how big is this record? this tells us where the next record is
            size_t size(void) const;

            // populate a record header and point our DBT's
            // buffers into ours if they are not infinite.
            void deserialize(const char *buf);

        private:
            record_header m_header;
            DBT m_left_key;
            DBT m_right_key;
        };

        void create(const range_buffer *buffer);

        // populate the given record object with the current
        // the memory referred to by record is valid for only
        // as long as the record exists.
        bool current(record *rec);

        // move the iterator to the next record in the buffer
        void next(void);

    private:
        // the key range buffer we are iterating over, the current
        // offset in that buffer, and the size of the current record.
        const range_buffer *m_buffer;
        size_t m_current_offset;
        size_t m_current_size;
    };

    // allocate buffer space lazily instead of on creation. this way,
    // no malloc/free is done if the transaction ends up taking no locks.
    void create(void);

    // append a left/right key range to the buffer.
    // if the keys are equal, then only one copy is stored.
    void append(const DBT *left_key, const DBT *right_key);

    void destroy(void);

private:
    char *m_buf;
    size_t m_buf_size;
    size_t m_buf_current;

    void append_range(const DBT *left_key, const DBT *right_key);

    // append a point to the buffer. this is the space/time saving
    // optimization for key ranges where left == right.
    void append_point(const DBT *key);

    void maybe_grow(size_t size);

    // the initial size of the buffer is the next power of 2
    // greater than the first entry we insert into the buffer.
    size_t get_initial_size(size_t n) const;
};

} /* namespace toku */

#endif /* RANGE_BUFFER_H */