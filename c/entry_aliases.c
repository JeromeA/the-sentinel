/* Scheduler unwind operations retain their stack state and game side effects. */
struct StackReturnEnd __far c_sentry_abort(void)
{
    struct StackReturnEnd end = {sentry_scheduler_saved_sp, (uint16_t)abi_c_sentry_schedule_finish};
    return end;
}

struct SentryRetryEnd __far c_sentry_retry_after_redraw(uint16_t source_slot)
{
    struct SentryRetryEnd end;
    end.previous = sentry_scan_cursor[source_slot];
    ++sentry_scan_cursor[source_slot];
    end.resume = c_sentry_abort();
    return end;
}

struct StackReturnEnd __far c_sentry_player_absorbed(void)
{
    game_end_flags = 128;
    return c_sentry_abort();
}
