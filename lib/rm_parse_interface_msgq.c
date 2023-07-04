#include <mqueue.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>

#include "nand_monitor.h"

#define QUEUE_INCOMING "/nand_monitor_msg"
#define QUEUE_OUTGOING "/nand_monitor_result"

__driver void rm_monitor_driver(rm_monitor_t * m)
{
    m->monitor = mq_open(QUEUE_INCOMING, O_WRONLY);
    m->driver = mq_open(QUEUE_OUTGOING, O_RDONLY);

    if (m->monitor == -1 || m->driver == -1)
    {
        perror("cannot open message queues for nand monitor");
        exit(1);
    }
}

__monitor void rm_monitor_close(rm_monitor_t * m)
{
    mq_close(m->monitor);
    mq_unlink(QUEUE_INCOMING);
    mq_close(m->driver);
    mq_unlink(QUEUE_OUTGOING);
    printf("monitor closed\n");
}

__monitor void rm_monitor_monitor(rm_monitor_t * m)
{
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
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
        rm_monitor_close(m);
        exit(1);
    }
    m->ready = true;
    printf("monitor opened: driver=0x%x, monitor=0x%x\n", m->driver, m->monitor);
}

__monitor enum rm_msg_type_t rm_monitor_next_token(rm_monitor_t * m, token_t *token, ms_t *machine_state)
{
    msg_t msg;
    if (mq_receive(m->monitor, (char *)&msg, MSG_SIZE, 0) == -1)
    {
        perror("cannot receive message from nand monitor");
        rm_monitor_close(m);
        exit(1);
    }

    switch (msg.type)
    {
    case MSG_TOKEN:
        *token = msg.token.token;
        *machine_state = msg.token.machine_state;
        break;
    case MSG_RESET:
        nand_monitor_reset();
        break ;
    default:
        break;
    }
    return msg.type;
}

__monitor void rm_monitor_set_result(rm_monitor_t * m, enum rm_err_t succ, ms_t machine_state)
{
    result_t result = {
        .succ = succ,
        .machine_state = machine_state,
    };
    if (mq_send(m->driver, (char *)&result, RESULT_SIZE, 0) == -1)
    {
        perror("cannot send message to nand monitor");
        rm_monitor_close(m);
        exit(1);
    }
}

static rm_monitor_t monitor =
{
    .ready = false,
};

__driver bool rm_monitor(token_t *token, ms_t *machine_state) {

    if (!monitor.ready)
    {
        rm_monitor_driver(&monitor);
        monitor.ready = true;
    }
    static msg_t msg;
    static result_t result;

    msg.type = MSG_TOKEN;
    msg.token.token = *token;
    msg.token.machine_state = *machine_state;

    if (mq_send(monitor.monitor, (char *)&msg, MSG_SIZE, 0) == -1)
    {
        perror("cannot send message to nand monitor");
        exit(1);
    }

    if (mq_receive(monitor.driver, (char *)&result, RESULT_SIZE, 0) == -1)
    {
        perror("cannot receive message from nand monitor");
        exit(1);
    }

    *machine_state = result.machine_state;
    return result.succ;
}

__driver void rm_monitor_reset()
{
    static msg_t msg;
    static result_t result;

    if (!monitor.ready)
    {
        rm_monitor_driver(&monitor);
        monitor.ready = true;
    }

    msg.type = MSG_RESET;
    if (mq_send(monitor.monitor, (char *)&msg, MSG_SIZE, 0) == -1)
    {
        perror("cannot send message to nand monitor");
        exit(1);
    }

    if (mq_receive(monitor.driver, (char *)&result, RESULT_SIZE, 0) == -1)
    {
        perror("cannot receive message from nand monitor");
        exit(1);
    }
}
