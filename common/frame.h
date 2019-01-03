#ifndef __FRAME_H__
#define __FRAME_H__

enum frame_types {
	FRAME_TYPE_NONE=0x00,	/* Undefined type */
    FRAME_TYPE_MESSAGE=0x01, /* 0x02 -- tx, rx, message type */
    FRAME_TYPE_COMMAND=0x02, /* 0x01 -- command type */
    FRAME_TYPE_DEBUG=0x03, /* 0x03 -- debug type */
};

#define MAX_PAYLOAD_SIZE 255

struct framehdr {
 //   uint8_t sfd;
    uint8_t seq;
    uint8_t msgType;
    uint8_t len;
    framehdr() : seq(0), msgType(frame_types::FRAME_TYPE_MESSAGE) {}
} __attribute__((packed));

struct message {
    struct framehdr framehdr;
    uint8_t payload[MAX_PAYLOAD_SIZE];
    message() { framehdr.msgType = frame_types::FRAME_TYPE_MESSAGE; }
}__attribute__((packed));

//message_serial to handle serial message
struct serial_message {
    uint8_t sfd;
    struct framehdr framehdr;
    uint8_t payload[MAX_PAYLOAD_SIZE];
    serial_message():sfd(0x7E) { framehdr.msgType = frame_types::FRAME_TYPE_MESSAGE; }
}__attribute__((packed));

//keeping frame stat
struct frame_stat {
    uint32_t rx;
    uint32_t tx;
    uint32_t drop;
    frame_stat():rx(0), tx(0), drop(0) {};
}__attribute__((packed));;

#endif
