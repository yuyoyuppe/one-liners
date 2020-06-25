 @@ -0,0 +1,70 @@
#pragma once

#include <Windows.h>
#include "type_traits.h"

template<typename F>
class delayed_executor
{
    static_assert(is_zero_arity_callable<F>::value, "F should be a callable accepting 0 arguments!");
    F _callable;
    HANDLE _timer = nullptr;
    unsigned _delay_milliseconds = 0;

    static void CALLBACK timer_callback(void* param, BOOLEAN timeout)
    {
        auto self = static_cast<delayed_executor*>(param);
        if (!timeout)
        {
            return;
        }
        self->_callable();
        self->delete_timer();
    }

    inline void create_timer()
    {
        ULONG flags = WT_EXECUTEDEFAULT | WT_EXECUTEONLYONCE;
        if (_delay_milliseconds > 1000)
        {
            flags |= WT_EXECUTELONGFUNCTION;
        }
        CreateTimerQueueTimer(&_timer, nullptr, timer_callback, this, _delay_milliseconds, 0, flags);
    }

    inline void delete_timer()
    {
        if (_timer)
        {
            DeleteTimerQueueTimer(nullptr, _timer, nullptr);
            _timer = nullptr;
        }
    }

public:
    delayed_executor(F callable, const unsigned delay_milliseconds, const bool start = true) :
        _callable{ callable },
        _delay_milliseconds{ delay_milliseconds }
    {
        if (start)
        {
            create_timer();
        }
    }

    inline void restart()
    {
        delete_timer();
        create_timer();
    }

    inline void stop()
    {
        delete_timer();
    }

    ~delayed_executor()
    {
        delete_timer();
    }
};
