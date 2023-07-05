#include <queue>
#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <cassert>
#include <atomic>

#include "nand_monitor.h"
#include "msgq_common.h"

struct rm_monitor_async_t
{
    rm_monitor_t mon;
    std::queue<msg_t> msg_queue;
    std::atomic_uint64_t tx, rx;
    std::queue<size_t> await_stop;
    pthread_mutex_t lock;
    pthread_cond_t all_processed;
    ms_t * machine_state;
    void (*parse_callback)(token_t token);
    void (*reset_callback)(void);
};

static struct rm_monitor_async_t am =
{
    false,
};

static void waitfor_all_processed()
{
    if (!am.mon.ready)
    {
        return;
    }

    pthread_mutex_lock(&am.lock);
    if (std::atomic_load(&am.tx) == std::atomic_load(&am.rx))
    {
        pthread_mutex_unlock(&am.lock);
    }
    else
    {
        assert(am.tx > am.rx);
        size_t tx = std::atomic_load(&am.tx);
        am.await_stop.push(tx);
        pthread_cond_wait(&am.all_processed, &am.lock);
        pthread_mutex_unlock(&am.lock);
    }
}

static void rm_monitor_async_callback(result_t * result)
{
    msg_t& msg = am.msg_queue.front();
    *am.machine_state = result->machine_state;

    if (msg.type == MSG_TOKEN)
    {
        if (result->succ == ERR_OK)
        {
            am.parse_callback(msg.token.token);
        }
        else if (result->succ == ERR_RESET_OK)
        {
            am.reset_callback();
            am.parse_callback(msg.token.token);
        }
    }
    am.msg_queue.pop();

    pthread_mutex_lock(&am.lock);
    atomic_fetch_add(&am.rx, 1);

    if (am.await_stop.empty())
    {
        pthread_cond_signal(&am.all_processed);
    }
    else
    {
        while ((!am.await_stop.empty()) && (am.await_stop.front() <= am.rx))
        {
            pthread_cond_broadcast(&am.all_processed);
            am.await_stop.pop();
        }
    }
    pthread_mutex_unlock(&am.lock);
}

extern "C"
{
    __driver void rm_monitor_async_setup(ms_t* machine_state,
        void (*parse_callback)(token_t token), void (*reset_callback)(void))
    {
        pthread_mutex_init(&am.lock, nullptr);
        pthread_cond_init(&am.all_processed, nullptr);
        std::atomic_store(&am.tx, 0);
        std::atomic_store(&am.rx, 0);

        am.machine_state = machine_state;
        am.parse_callback = parse_callback;
        am.reset_callback = reset_callback;
        msgq_driver_open(&am.mon);
        msgq_setup_rx_result_async(&am.mon, rm_monitor_async_callback);
        am.mon.ready = true;
    }

    __driver void rm_monitor_async_close()
    {
        if (!am.mon.ready)
        {
            return;
        }
        msgq_driver_close(&am.mon);
        am.mon.ready = false;
    }

    __driver void rm_monitor_async(token_t* token, ms_t* machine_state)
    {
        if (!am.mon.ready)
        {
            return;
        }
        msg_t msg               = { MSG_TOKEN };
        msg.token.token         = *token;
        msg.token.machine_state = *machine_state;

        am.msg_queue.push(msg);
        msgq_tx_msg(&am.mon, &msg);

        pthread_mutex_lock(&am.lock);
        atomic_fetch_add(&am.tx, 1);
        pthread_mutex_unlock(&am.lock);
    }

    __driver void await_rm_monitor_async()
    {
        if (!am.mon.ready)
        {
            return;
        }
        waitfor_all_processed();
    }

    __driver void rm_monitor_reset()
    {
        if (!am.mon.ready)
        {
            return;
        }
        msg_t msg = { MSG_RESET };
        am.msg_queue.push(msg);
        msgq_tx_msg(&am.mon, &msg);

        pthread_mutex_lock(&am.lock);
        atomic_fetch_add(&am.tx, 1);
        pthread_mutex_unlock(&am.lock);
    }
}
