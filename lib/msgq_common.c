#include <fcntl.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

#include "msgq_common.h"
#include "nand_monitor.h"

#define QUEUE_INCOMING "/nand_monitor_msg"
#define QUEUE_OUTGOING "/nand_monitor_result"

void
msgq_monitor_close(rm_monitor_t* m)
{
    mq_close(m->monitor);
    mq_unlink(QUEUE_INCOMING);
    mq_close(m->driver);
    mq_unlink(QUEUE_OUTGOING);
    printf("monitor closed\n");
}

void
msgq_driver_close(rm_monitor_t* m)
{
    mq_close(m->monitor);
    mq_close(m->driver);
    printf("driver closed\n");
}

void
msgq_driver_open(rm_monitor_t* m)
{
    while ((m->driver = mq_open(QUEUE_OUTGOING, O_RDONLY)) == -1)
    {
        if (errno != ENOENT)
        {
            perror("cannot open message queue for nand monitor");
            msgq_driver_close(m);
            exit(1);
        }
        printf("waiting for monitor start ...\n");
        sleep(1);
    }

    m->monitor  = mq_open(QUEUE_INCOMING, O_WRONLY);
    if (m->monitor == -1 || m->driver == -1)
    {
        perror("cannot open message queues for nand monitor");
        msgq_driver_close(m);
        exit(1);
    }
    printf("monitor connected: result=0x%x, msg=0x%x\n", m->driver, m->monitor);
}

void
msgq_monitor_open(rm_monitor_t* m)
{
    struct mq_attr attr;
    attr.mq_flags   = 0;
    attr.mq_maxmsg  = MSGQ_QUEUE_SIZE;
    attr.mq_msgsize = MSG_SIZE;
    attr.mq_curmsgs = 0;

    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

    mq_unlink(QUEUE_INCOMING);
    m->monitor = mq_open(QUEUE_INCOMING, O_CREAT | O_RDONLY, mode, &attr);

    attr.mq_msgsize = RESULT_SIZE;
    mq_unlink(QUEUE_OUTGOING);
    m->driver = mq_open(QUEUE_OUTGOING, O_CREAT | O_WRONLY, mode, &attr);

    if (m->monitor == -1 || m->driver == -1)
    {
        perror("cannot open message queues for nand monitor");
        msgq_monitor_close(m);
        exit(1);
    }
    m->ready = true;
    printf("monitor opened: result=0x%x, msg=0x%x\n", m->driver, m->monitor);
}

void
msgq_rx_msg(rm_monitor_t* m, msg_t* msg)
{
    if (mq_receive(m->monitor, (char*)msg, MSG_SIZE, 0) == -1)
    {
        perror("cannot receive message from `msg` queue");
        msgq_monitor_close(m);
        exit(1);
    }
}

void
msgq_tx_msg(rm_monitor_t* m, msg_t* msg)
{
    if (mq_send(m->monitor, (char*)msg, MSG_SIZE, 0) == -1)
    {
        perror("cannot send message to `msg` queue");
        msgq_driver_close(m);
        exit(1);
    }
}

void
msgq_rx_result(rm_monitor_t* m, result_t * result)
{
    if (mq_receive(m->driver, (char*)result, RESULT_SIZE, 0) == -1)
    {
        perror("cannot receive message from `result` queue");
        msgq_driver_close(m);
        exit(1);
    }
}

void
msgq_tx_result(rm_monitor_t* m, result_t* result)
{
    if (mq_send(m->driver, (char*)result, RESULT_SIZE, 0) == -1)
    {
        perror("cannot send message to `result` queue");
        msgq_monitor_close(m);
        exit(1);
    }
}

struct msgq_async_ctx_t
{
    rm_monitor_t* m;
    sigevent_t sev;
    void * callback;
};

static struct msgq_async_ctx_t rx_msg_ctx;
static struct msgq_async_ctx_t rx_result_ctx;

static void on_rx_msg(union sigval sv)
{
    msg_t msg;
    struct mq_attr attr;
    mq_getattr(rx_msg_ctx.m->monitor, &attr);
    size_t n = attr.mq_curmsgs;
    while (n-- > 0)
    {
        msgq_rx_msg(rx_msg_ctx.m, &msg);
        ((void (*)(msg_t*))rx_msg_ctx.callback)(&msg);
    }

    if (mq_notify(rx_msg_ctx.m->monitor, &rx_msg_ctx.sev) == -1)
    {
        perror("cannot set notification for nand monitor");
        msgq_monitor_close(rx_msg_ctx.m);
        exit(1);
    }
}

static void on_rx_result(union sigval sv)
{
    result_t result;
    struct mq_attr attr;
    mq_getattr(rx_result_ctx.m->driver, &attr);
    size_t n = attr.mq_curmsgs;
    while (n-- > 0)
    {
        msgq_rx_result(rx_result_ctx.m, &result);
        ((void (*)(result_t*))rx_result_ctx.callback)(&result);
    }

    if (mq_notify(rx_result_ctx.m->driver, &rx_result_ctx.sev) == -1)
    {
        perror("cannot set notification for nand monitor");
        msgq_driver_close(rx_result_ctx.m);
        exit(1);
    }
}

void msgq_setup_rx_msg_async(rm_monitor_t* m, void (*callback)(msg_t *))
{
    rx_msg_ctx.m = m;
    rx_msg_ctx.callback = callback;
    rx_msg_ctx.sev.sigev_notify = SIGEV_THREAD;
    rx_msg_ctx.sev.sigev_notify_function = on_rx_msg;
    rx_msg_ctx.sev.sigev_notify_attributes = NULL;
    rx_msg_ctx.sev.sigev_value.sival_ptr = &rx_msg_ctx;

    if (mq_notify(m->monitor, &rx_msg_ctx.sev) == -1)
    {
        perror("cannot set notification for nand monitor");
        msgq_monitor_close(m);
        exit(1);
    }
}

void msgq_setup_rx_result_async(rm_monitor_t* m, void (*callback)(result_t *))
{
    rx_result_ctx.m = m;
    rx_result_ctx.callback = callback;
    rx_result_ctx.sev.sigev_notify = SIGEV_THREAD;
    rx_result_ctx.sev.sigev_notify_function = on_rx_result;
    rx_result_ctx.sev.sigev_notify_attributes = NULL;
    rx_result_ctx.sev.sigev_value.sival_ptr = &rx_result_ctx;

    if (mq_notify(m->driver, &rx_result_ctx.sev) == -1)
    {
        perror("cannot set notification for nand monitor");
        msgq_driver_close(m);
        exit(1);
    }
}

