/**
 * E1.31 (sACN) library for C/C++
 * Hugo Hromic - http://github.com/hhromic
 *
 * Some content of this file is based on:
 * https://github.com/forkineye/E131/blob/master/E131.h
 * https://github.com/forkineye/E131/blob/master/E131.cpp
 *
 * Copyright 2016 Hugo Hromic
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <string.h>
#include "socket.h"
#include "e131.h"

/* E1.31 Public Constants */
const uint16_t E131_DEFAULT_PORT = 5568;
const uint8_t E131_DEFAULT_PRIORITY = 0x64;

/* E1.31 Private Constants */
const uint16_t _E131_PREAMBLE_SIZE = 0x0010;
const uint16_t _E131_POSTAMBLE_SIZE = 0x0000;
const uint8_t _E131_ACN_PID[] = {0x41, 0x53, 0x43, 0x2d, 0x45, 0x31, 0x2e, 0x31, 0x37, 0x00, 0x00, 0x00};
const uint32_t _E131_ROOT_VECTOR = 0x00000004;
const uint32_t _E131_FRAME_VECTOR = 0x00000002;
const uint8_t _E131_DMP_VECTOR = 0x02;
const uint8_t _E131_DMP_TYPE = 0xa1;
const uint16_t _E131_DMP_FIRST_ADDR = 0x0000;
const uint16_t _E131_DMP_ADDR_INC = 0x0001;

uint8_t destip[4];
uint16_t destport;
int8_t socknum = 0;

/* Create a socket file descriptor suitable for E1.31 communication */
int e131_socket(void)
{
	socknum = socket(0, Sn_MR_UDP, E131_DEFAULT_PORT, SF_IO_NONBLOCK);
	return socknum;
}

/* Initialize an E1.31 packet using a universe and a number of slots */
int e131_pkt_init(e131_packet_t *packet, const uint16_t universe, const uint16_t num_slots)
{
	if (packet == NULL || universe < 1 || universe > 63999 || num_slots < 1 || num_slots > 512)
		return -1;

	// compute packet layer lengths
	uint16_t prop_val_cnt = num_slots + 1;
	uint16_t dmp_length = prop_val_cnt +
						  sizeof packet->dmp - sizeof packet->dmp.prop_val;
	uint16_t frame_length = sizeof packet->frame + dmp_length;
	uint16_t root_length = sizeof packet->root.flength +
						   sizeof packet->root.vector + sizeof packet->root.cid + frame_length;

	// clear packet
	memset(packet, 0, sizeof *packet);

	// set Root Layer values
	packet->root.preamble_size = htons(_E131_PREAMBLE_SIZE);
	packet->root.postamble_size = htons(_E131_POSTAMBLE_SIZE);
	memcpy(packet->root.acn_pid, _E131_ACN_PID, sizeof packet->root.acn_pid);
	packet->root.flength = htons(0x7000 | root_length);
	packet->root.vector = htonl(_E131_ROOT_VECTOR);

	// set Framing Layer values
	packet->frame.flength = htons(0x7000 | frame_length);
	packet->frame.vector = htonl(_E131_FRAME_VECTOR);
	packet->frame.priority = E131_DEFAULT_PRIORITY;
	packet->frame.universe = htons(universe);

	// set Device Management Protocol (DMP) Layer values
	packet->dmp.flength = htons(0x7000 | dmp_length);
	packet->dmp.vector = _E131_DMP_VECTOR;
	packet->dmp.type = _E131_DMP_TYPE;
	packet->dmp.first_addr = htons(_E131_DMP_FIRST_ADDR);
	packet->dmp.addr_inc = htons(_E131_DMP_ADDR_INC);
	packet->dmp.prop_val_cnt = htons(prop_val_cnt);

	return 0;
}

/* Get the state of a framing option in an E1.31 packet */
bool e131_get_option(const e131_packet_t *packet, const e131_option_t option)
{
	if (packet != NULL && packet->frame.options & (1 << (option % 8)))
		return true;
	return false;
}

/* Set the state of a framing option in an E1.31 packet */
int e131_set_option(e131_packet_t *packet, const e131_option_t option, const bool state)
{
	if (packet == NULL)
		return -1;
	packet->frame.options ^= (-state ^ packet->frame.options) & (1 << (option % 8));
	return 0;
}

/* Send an E1.31 packet to a socket file descriptor using a destination */
int e131_send(int sockfd, const e131_packet_t *packet)
{
	if (packet == NULL)
		return -1;
	uint16_t packet_length = sizeof packet->raw - sizeof packet->dmp.prop_val + htons(packet->dmp.prop_val_cnt);
	return sendto(socknum, packet->raw, packet_length, destip, destport);
}

/* Receive an E1.31 packet from a socket file descriptor */
int e131_recv(int sockfd, e131_packet_t *packet)
{
	if (packet == NULL)
		return -1;
	if (getSn_SR(socknum) == SOCK_UDP)
		return recvfrom(socknum, packet->raw, sizeof packet->raw, destip, (uint16_t*)&destport);
	return -1;
}

/* Validate that an E1.31 packet is well-formed */
e131_error_t e131_pkt_validate(const e131_packet_t *packet)
{
	if (packet == NULL)
		return E131_ERR_NULLPTR;
	if (ntohs(packet->root.preamble_size) != _E131_PREAMBLE_SIZE)
		return E131_ERR_PREAMBLE_SIZE;
	if (ntohs(packet->root.postamble_size) != _E131_POSTAMBLE_SIZE)
		return E131_ERR_POSTAMBLE_SIZE;
	if (memcmp(packet->root.acn_pid, _E131_ACN_PID, sizeof packet->root.acn_pid) != 0)
		return E131_ERR_ACN_PID;
	if (ntohl(packet->root.vector) != _E131_ROOT_VECTOR)
		return E131_ERR_VECTOR_ROOT;
	if (ntohl(packet->frame.vector) != _E131_FRAME_VECTOR)
		return E131_ERR_VECTOR_FRAME;
	if (packet->dmp.vector != _E131_DMP_VECTOR)
		return E131_ERR_VECTOR_DMP;
	if (packet->dmp.type != _E131_DMP_TYPE)
		return E131_ERR_TYPE_DMP;
	if (htons(packet->dmp.first_addr) != _E131_DMP_FIRST_ADDR)
		return E131_ERR_FIRST_ADDR_DMP;
	if (htons(packet->dmp.addr_inc) != _E131_DMP_ADDR_INC)
		return E131_ERR_ADDR_INC_DMP;
	return E131_ERR_NONE;
}

/* Check if an E1.31 packet should be discarded (sequence number out of order) */
bool e131_pkt_discard(const e131_packet_t *packet, const uint8_t last_seq_number)
{
	if (packet == NULL)
		return true;
	int8_t seq_num_diff = packet->frame.seq_number - last_seq_number;
	if (seq_num_diff > -20 && seq_num_diff <= 0)
		return true;
	return false;
}

/* Dump an E1.31 packet to a stream (i.e. stdout, stderr) */
int e131_pkt_dump(const e131_packet_t *packet)
{
	if (packet == NULL)
		return -1;

	printf("[Root Layer]\n");
	printf("  Preamble Size .......... %d\n", ntohs(packet->root.preamble_size));
	printf("  Post-amble Size ........ %d\n", ntohs(packet->root.postamble_size));
	printf("  ACN Packet Identifier .. %s\n", packet->root.acn_pid);
	printf("  Flags & Length ......... %d\n", ntohs(packet->root.flength));
	printf("  Layer Vector ........... %d\n", ntohl(packet->root.vector));
	printf("  Component Identifier ... ");
	for (size_t pos = 0, total = sizeof packet->root.cid; pos < total; pos++)
		printf("%02x", packet->root.cid[pos]);
	printf("\n");
	printf("[Framing Layer]\n");
	printf("  Flags & Length ......... %d\n", ntohs(packet->frame.flength));
	printf("  Layer Vector ........... %d\n", ntohl(packet->frame.vector));
	printf("  Source Name ............ %s\n", packet->frame.source_name);
	printf("  Packet Priority ........ %d\n", packet->frame.priority);
	printf("  Reserved ............... %d\n", ntohs(packet->frame.reserved));
	printf("  Sequence Number ........ %d\n", packet->frame.seq_number);
	printf("  Options Flags .......... %d\n", packet->frame.options);
	printf("  DMX Universe Number .... %d\n", ntohs(packet->frame.universe));
	printf("[Device Management Protocol (DMP) Layer]\n");
	printf("  Flags & Length ......... %d\n", ntohs(packet->dmp.flength));
	printf("  Layer Vector ........... %d\n", packet->dmp.vector);
	printf("  Address & Data Type .... %d\n", packet->dmp.type);
	printf("  First Address .......... %d\n", ntohs(packet->dmp.first_addr));
	printf("  Address Increment ...... %d\n", ntohs(packet->dmp.addr_inc));
	printf("  Property Value Count ... %d\n", ntohs(packet->dmp.prop_val_cnt));
	printf("[DMP Property Values]\n ");
	for (size_t pos = 0, total = ntohs(packet->dmp.prop_val_cnt); pos < total; pos++)
		printf(" %02x", packet->dmp.prop_val[pos]);
	printf("\n");
	return 0;
}

/* Return a string describing an E1.31 error */
const char *e131_strerror(const e131_error_t error)
{
	switch (error)
	{
	case E131_ERR_NONE:
		return "Success";
	case E131_ERR_PREAMBLE_SIZE:
		return "Invalid Preamble Size";
	case E131_ERR_POSTAMBLE_SIZE:
		return "Invalid Post-amble Size";
	case E131_ERR_ACN_PID:
		return "Invalid ACN Packet Identifier";
	case E131_ERR_VECTOR_ROOT:
		return "Invalid Root Layer Vector";
	case E131_ERR_VECTOR_FRAME:
		return "Invalid Framing Layer Vector";
	case E131_ERR_VECTOR_DMP:
		return "Invalid Device Management Protocol (DMP) Layer Vector";
	case E131_ERR_TYPE_DMP:
		return "Invalid DMP Address & Data Type";
	case E131_ERR_FIRST_ADDR_DMP:
		return "Invalid DMP First Address";
	case E131_ERR_ADDR_INC_DMP:
		return "Invalid DMP Address Increment";
	default:
		return "Unknown error";
	}
}
