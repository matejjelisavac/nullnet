#ifndef WIRE_H
#define WIRE_H

// Connection message sent to a hub. Since nullnet hub has no physical wire, this is simulating the "plugging in".
#define WIRE_HELLO 0xCC
// Disconnection message sent to a hub. Since nullnet hub has no physical wire, this is simulating the "unplugging".
#define WIRE_GOODBYE 0xDD

#define WIRE_MSG_LENGTH 1

#endif