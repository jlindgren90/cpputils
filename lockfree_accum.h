#include <assert.h>
#include <atomic>

/*
 * Lock-free double-buffer implementation which allows accumulation of
 * data into a buffer from one thread and reporting from a second thread
 * even if the buffer itself does not support atomic operations.
 *
 * The buffer class (Buf) must implement four functions:
 *
 *   operator=(const Buf &b);
 *   void accum(const Val &v);
 *   const Val &report();
 *   void reset();
 *
 * where:
 *
 *   - accum() adds a value to the buffer (for some sense of "add")
 *   - report() returns the accumulated result (sum)
 *   - reset() resets the buffer to its initial state
 *
 * The design is probably of limited utility except in very specific
 * situations, but it was a fun exercise to implement.
 */
template<class Buf, class Val>
class lockfree_accum
{
private:
    /* buffer state pairs */
    enum {
        EMPTY_EMPTY = 0,
        ACCUM_EMPTY = 1,
        VALID_EMPTY = 2,
        VALID_ACCUM = 3,
        REPORT_EMPTY = 4,
        REPORT_ACCUM = 5,
        REPORT_VALID = 6,
        /* reverse pairs are offset by 8 */
        EMPTY_EMPTY_ALT = 8, /* same as EMPTY_EMPTY */
        EMPTY_ACCUM = 9,
        EMPTY_VALID = 10,
        ACCUM_VALID = 11,
        EMPTY_REPORT = 12,
        ACCUM_REPORT = 13,
        VALID_REPORT = 14,
    };

    Buf m_bufs[2]{};

    std::atomic_uint8_t m_state = EMPTY_EMPTY;

public:
    void accum(const Val &val)
    {
        uint8_t state = m_state.load();

        /* Must not already be accumulating */
        assert(state != ACCUM_EMPTY && state != VALID_ACCUM &&
               state != REPORT_ACCUM && state != EMPTY_ACCUM &&
               state != ACCUM_VALID && state != ACCUM_REPORT);

        /*
         * One buffer valid, the other reporting. Accumulate into the
         * valid buffer.
         *
         * Transition table:
         *   REPORT_VALID(6) -> REPORT_ACCUM(5)
         *   VALID_REPORT(14) -> ACCUM_REPORT(13)
         *
         * The CMPXCHG can fail due to a simultaneous reset(). In that
         * case, "state" has been updated with the current value of
         * m_state, which will be handled in other cases.
         */
        if ((state == REPORT_VALID || state == VALID_REPORT) &&
            m_state.compare_exchange_strong(state, state - 1)) {

            uint8_t valid_idx = (state >> 3) ^ 1;
            m_bufs[valid_idx].accum(val);

            /*
             * Possible simultaneous transitions due to reset():
             *   REPORT_ACCUM(5)  -> EMPTY_ACCUM(9)
             *   ACCUM_REPORT(13) -> ACCUM_EMPTY(1)
             *
             * Transition table:
             *   REPORT_ACCUM(5)  -> REPORT_VALID(6)
             *   EMPTY_ACCUM(9)   -> EMPTY_VALID(10)
             *   ACCUM_REPORT(13) -> VALID_REPORT(14)
             *   ACCUM_EMPTY(1)   -> VALID_EMPTY(2)
             */
            state = ++m_state;
            assert(state == VALID_EMPTY || state == REPORT_VALID ||
                   state == EMPTY_VALID || state == VALID_REPORT);
            return;
        }

        /*
         * One buffer valid, the other empty. No reporting in progress.
         * Copy the valid buffer to the empty one (to leave the valid
         * one available for reporting), then accumulate into the copy.
         *
         * Transition table:
         *   VALID_EMPTY(2) -> VALID_ACCUM(3)
         *   EMPTY_VALID(10) -> ACCUM_VALID(11)
         *
         * The CMPXCHG can fail due to a simultaneous report(). In that
         * case, "state" has been updated with the current value of
         * m_state, which will be handled in other cases.
         */
        if ((state == VALID_EMPTY || state == EMPTY_VALID) &&
            m_state.compare_exchange_strong(state, state + 1)) {

            uint8_t valid_idx = (state >> 3);
            uint8_t empty_idx = valid_idx ^ 1;

            m_bufs[empty_idx].reset();
            m_bufs[empty_idx] = m_bufs[valid_idx];
            m_bufs[empty_idx].accum(val);

            /*
             * Now mark the updated copy as valid and the original
             * buffer as empty. If the CMPXCHG succeeds, we are done.
             *
             * Transition table:
             *   VALID_ACCUM(3) -> EMPTY_VALID(10)
             *   ACCUM_VALID(11) -> VALID_EMPTY(2)
             */
            state++;
            if (m_state.compare_exchange_strong(state, (state - 1) ^ 8)) {
                return;
            }

            /*
             * If the second CMPXCHG fails, the updated copy is
             * discarded, the buffer is marked empty again, and we
             * proceed to other cases.
             *
             * Transition table (failure cases):
             *   REPORT_ACCUM(5)  -> REPORT_EMPTY(4)
             *   EMPTY_ACCUM(9)   -> EMPTY_EMPTY_ALT(8)
             *   ACCUM_REPORT(13) -> EMPTY_REPORT(12)
             *   ACCUM_EMPTY(1)   -> EMPTY_EMPTY(0)
             */
            state = --m_state;
            assert(state == EMPTY_EMPTY || state == REPORT_EMPTY ||
                   state == EMPTY_EMPTY_ALT || state == EMPTY_REPORT);
        }

        /*
         * One buffer empty, the other either empty or being reported.
         * Reset and accumulate into (one of) the empty buffer(s).
         *
         * Possible simultaneous transitions due to reset():
         *   REPORT_EMPTY(4)  -> EMPTY_EMPTY_ALT(8)
         *   EMPTY_REPORT(12) -> EMPTY_EMPTY(0)
         *
         * Transition table:
         *   REPORT_EMPTY(4)    -> REPORT_ACCUM(5)
         *   EMPTY_EMPTY_ALT(8) -> EMPTY_ACCUM(9)
         *   EMPTY_REPORT(12)   -> ACCUM_REPORT(13)
         *   EMPTY_EMPTY(0)     -> ACCUM_EMPTY(1)
         */
        if (state == EMPTY_EMPTY || state == REPORT_EMPTY ||
            state == EMPTY_EMPTY_ALT || state == EMPTY_REPORT) {

            uint8_t empty_idx = (state >> 3) ^ 1;

            state = ++m_state;
            assert(state == ACCUM_EMPTY || state == REPORT_ACCUM ||
                   state == EMPTY_ACCUM || state == ACCUM_REPORT);

            m_bufs[empty_idx].reset();
            m_bufs[empty_idx].accum(val);

            /*
             * Transition table:
             *   REPORT_ACCUM(5)  -> REPORT_VALID(6)
             *   EMPTY_ACCUM(9)   -> EMPTY_VALID(10)
             *   ACCUM_REPORT(13) -> VALID_REPORT(14)
             *   ACCUM_EMPTY(1)   -> VALID_EMPTY(2)
             */
            state = ++m_state;
            assert(state == VALID_EMPTY || state == REPORT_VALID ||
                   state == EMPTY_VALID || state == VALID_REPORT);
        } else {
            assert(false);
        }
    }

    const Val *report()
    {
        uint8_t state = m_state.load();

        /* Must not already be reporting. */
        assert(state != REPORT_EMPTY && state != REPORT_ACCUM &&
               state != REPORT_VALID && state != EMPTY_REPORT &&
               state != ACCUM_REPORT && state != VALID_REPORT);

        /*
         * Must have a valid buffer.
         *
         * Transition table:
         *   VALID_EMPTY(2)  -> REPORT_EMPTY(4)
         *   VALID_ACCUM(3)  -> REPORT_ACCUM(5)
         *   EMPTY_VALID(10) -> EMPTY_REPORT(12)
         *   ACCUM_VALID(11) -> ACCUM_REPORT(13)
         *
         * If the CMPXCHG fails due to a simultaneous accum(), try again.
         */
        while (state == VALID_EMPTY || state == VALID_ACCUM ||
               state == EMPTY_VALID || state == ACCUM_VALID) {

            if (!m_state.compare_exchange_strong(state, state + 2)) {
                continue;
            }

            uint8_t valid_idx = (state >> 3);
            return &m_bufs[valid_idx].report();
        }

        /* No valid buffer - return null */
        return nullptr;
    }

    void reset()
    {
        uint8_t state = m_state.load();

        /* Must be reporting. */
        assert(state == REPORT_EMPTY || state == REPORT_ACCUM ||
               state == REPORT_VALID || state == EMPTY_REPORT ||
               state == ACCUM_REPORT || state == VALID_REPORT);

        /*
         * Just reset the state, accum() will reset the buffer later.
         *
         * Transition table:
         *   REPORT_EMPTY(4)  -> EMPTY_EMPTY_ALT(8)
         *   REPORT_ACCUM(5)  -> EMPTY_ACCUM(9)
         *   REPORT_VALID(6)  -> EMPTY_VALID(10)
         *   EMPTY_REPORT(12) -> EMPTY_EMPTY(0)
         *   ACCUM_REPORT(13) -> ACCUM_EMPTY(1)
         *   VALID_REPORT(14) -> VALID_EMPTY(2)
         */
        m_state ^= 12;
    }
};
