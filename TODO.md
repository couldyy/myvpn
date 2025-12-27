Roadmap:
- [ ] implement reliability for every message type using seq and ack numbers:
    - [ ] implement global timer
    - for each message type implement timer and N times retransmission if ACK or reply packet
            was not received in K seconds
    - for data packets just use seq and (ack?) with no retransmission

- [ ] use arena allocator for handling each packet instead of small dedicated mallocs
- [ ] implement packet encryption
- [ ] implement block/allow lists
- [ ] implement separate cli program to send commands to server/client via UNIX domain sockets
- [ ] parse VPN params from .conf file
- [ ] GUI for client (separate binary)
