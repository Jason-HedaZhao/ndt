/*
 *  This is a modified version of traceroute.  It is designed to be
 *  a part of the Internet2 FLM system.  It links into the fakewww
 *  server program and traces routes from the NDT server to the
 *  remote client.  It returns a list of IP addresses found on this
 *  return path.  This list will then be compared to the traceroute
 *  map generated by the tr-tree program.  
 *
 *  Richard Carlson
 *  rcarlson@interent2.edu
 *  March 9, 2004
 */

/*-
 * Copyright (c) 1990, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Van Jacobson.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if 0
#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1990, 1993\n\
  The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)troute.c  8.1 (Berkeley) 6/6/93";
#endif /* not lint */
#endif

#include <sys/param.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>

#include "logging.h"

#define  MAXPACKET  65535  /* max ip packet size */
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN  64
#endif

#ifndef FD_SET
#define NFDBITS         (8*sizeof(fd_set))
#define FD_SETSIZE      NFDBITS
#define FD_SET(n, p)    ((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define FD_CLR(n, p)    ((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define FD_ISSET(n, p)  ((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)      bzero((char *)(p), sizeof(*(p)))
#endif

/*
 * format of a (udp) probe packet.
 */
struct opacket {
	struct ip ip;
	struct udphdr udp;
	u_char seq; /* sequence number of this packet */
	u_char ttl; /* ttl packet left with */
	struct timeval tv; /* time packet left */
};

u_char packet[512]; /* last inbound (icmp) packet */
struct opacket *outpacket; /* last output (udp) packet */

int wait_for_reply __P((int, struct sockaddr_in *));
void send_probe __P((int, int));
double deltaT __P((struct timeval *, struct timeval *));
int packet_ok __P((u_char *, int, struct sockaddr_in *, int));
void tvsub __P((struct timeval *, struct timeval *));

int s; /* receive (icmp) socket file descriptor */
int sndsock; /* send (udp) socket file descriptor */
struct timezone tz; /* leftover */

struct sockaddr whereto; /* Who to try to reach */
int datalen; /* How much data */

char *source = 0;
char *hostname;

int nprobes = 2;
u_short ident;
u_short port = 32768 + 666; /* start udp dest port # for probe packets */
int options; /* socket options */
int verbose;
int waittime = 1; /* time to wait for response (in seconds) */
int nflag; /* print addresses numerically */

void find_route(u_int32_t destIP, u_int32_t IPlist[], int max_ttl) {
	extern char *optarg;
	extern int optind;
	struct protoent *pe;
	struct sockaddr_in from, *to;
	int i, on, probe, seq, tos, ttl;

	on = 1;
	seq = tos = 0;
	to = (struct sockaddr_in *) &whereto;
	(void) bzero((char *) &whereto, sizeof(struct sockaddr));
	to->sin_family = AF_INET;
	to->sin_addr.s_addr = destIP;

	datalen += sizeof(struct opacket);
	outpacket = (struct opacket *) malloc((unsigned) datalen);
	if (!outpacket) {
		perror("traceroute: malloc");
		exit(1);
	}
	(void) bzero((char *) outpacket, datalen);
	outpacket->ip.ip_dst = to->sin_addr;
	outpacket->ip.ip_tos = tos;
	outpacket->ip.ip_v = IPVERSION;
	outpacket->ip.ip_id = 0;

	ident = (getpid() & 0xffff) | 0x8000;

	if ((pe = getprotobyname("icmp")) == NULL) {
		fprintf(stderr, "icmp: unknown protocol\n");
		exit(10);
	}
	if ((s = socket(AF_INET, SOCK_RAW, pe->p_proto)) < 0) {
		perror("traceroute: icmp socket");
		exit(5);
	}

	if ((sndsock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		perror("traceroute: raw socket");
		exit(5);
	}

	if (source) {
		(void) bzero((char *) &from, sizeof(struct sockaddr));
		from.sin_family = AF_INET;
		from.sin_addr.s_addr = inet_addr(source);
		if (from.sin_addr.s_addr == -1) {
			printf("traceroute: unknown host %s\n", source);
			exit(1);
		}
		outpacket->ip.ip_src = from.sin_addr;
#ifndef IP_HDRINCL
		if (bind(sndsock, (struct sockaddr *) &from, sizeof(from)) < 0) {
			perror("traceroute: bind:");
			exit(1);
		}
#endif /* IP_HDRINCL */
	}

	for (ttl = 1; ttl <= max_ttl; ++ttl) {
		u_long lastaddr = 0;
		int got_there = 0;
		int unreachable = 0;

		for (probe = 0; probe < nprobes; ++probe) {
			int cc;
			struct timeval t1, t2;
			struct timezone tz;
			struct ip *ip;

			(void) gettimeofday(&t1, &tz);
			send_probe(++seq, ttl);
			while ((cc = wait_for_reply(s, &from))) {
				(void) gettimeofday(&t2, &tz);
				if ((i = packet_ok(packet, cc, &from, seq))) {
					if (from.sin_addr.s_addr != lastaddr) {
						IPlist[ttl - 1] = from.sin_addr.s_addr;
						lastaddr = from.sin_addr.s_addr;
					}
					log_println(5, "Probe %d resulted in reply from [%s]",
							probe, inet_ntoa(from.sin_addr));

					switch (i - 1) {
					case ICMP_UNREACH_PORT:
#ifndef ARCHAIC
						ip = (struct ip *) packet;
#endif /* ARCHAIC */
						++got_there;
						break;
					case ICMP_UNREACH_NET:
						++unreachable;
						break;
					case ICMP_UNREACH_HOST:
						++unreachable;
						break;
					case ICMP_UNREACH_PROTOCOL:
						++got_there;
						break;
					case ICMP_UNREACH_NEEDFRAG:
						++unreachable;
						break;
					case ICMP_UNREACH_SRCFAIL:
						++unreachable;
						break;
					}
					break;
				}
			}
		}
		if (got_there || unreachable >= nprobes - 1) {
			return;
		}
	}
}

int wait_for_reply(sock, from)
	int sock;struct sockaddr_in *from; {
	fd_set fds;
	struct timeval wait;
	int cc = 0;
	socklen_t fromlen = sizeof(*from);

	FD_ZERO(&fds);
	FD_SET(sock, &fds);
	wait.tv_sec = waittime;
	wait.tv_usec = 0;

	if (select(sock + 1, &fds, (fd_set *) 0, (fd_set *) 0, &wait) > 0)
		cc = recvfrom(s, (char *) packet, sizeof(packet), 0,
				(struct sockaddr *) from, &fromlen);

	return (cc);
}

void send_probe(seq, ttl)
	int seq, ttl; {
	struct opacket *op = outpacket;
	struct ip *ip = &op->ip;
	struct udphdr *up = &op->udp;
	int i;

	ip->ip_off = 0;
	ip->ip_hl = sizeof(*ip) >> 2;
	ip->ip_p = IPPROTO_UDP;
	ip->ip_len = datalen;
	ip->ip_ttl = ttl;
	ip->ip_v = IPVERSION;
	ip->ip_id = htons(ident + seq);

#ifdef __FAVOR_BSD
	up->uh_sport = htons(ident);
	up->uh_dport = htons(port+seq);
	up->uh_ulen = htons((u_short)(datalen - sizeof(struct ip)));
	up->uh_sum = 0;
#else
	up->source = htons(ident);
	up->dest = htons(port + seq);
	up->len = htons((u_short)(datalen - sizeof(struct ip)));
	up->check = 0;
#endif

	op->seq = seq;
	op->ttl = ttl;
	(void) gettimeofday(&op->tv, &tz);

	i = sendto(sndsock, (char *) outpacket, datalen, 0, &whereto,
			sizeof(struct sockaddr));
	if (i < 0 || i != datalen) {
		if (i < 0)
			perror("sendto");
	}
}

double deltaT(t1p, t2p)
	struct timeval *t1p, *t2p; {
	register double dt;

	dt = (double) (t2p->tv_sec - t1p->tv_sec) * 1000.0
			+ (double) (t2p->tv_usec - t1p->tv_usec) / 1000.0;
	return (dt);
}

int packet_ok(buf, cc, from, seq)
	u_char *buf;int cc;struct sockaddr_in *from;int seq; {
	register struct icmp *icp;
	u_char type, code;
	int hlen;
#ifndef ARCHAIC
	struct ip *ip;

	ip = (struct ip *) buf;
	hlen = ip->ip_hl << 2;
	if (cc < hlen + ICMP_MINLEN) {
		return (0);
	}
	cc -= hlen;
	icp =
			(struct icmp *) (buf + hlen);
#else
			icp = (struct icmp *)buf;
#endif /* ARCHAIC */
			type = icp->icmp_type; code = icp->icmp_code;
			if ((type == ICMP_TIMXCEED && code == ICMP_TIMXCEED_INTRANS) ||
					type == ICMP_UNREACH) {
				struct ip *hip;
				struct udphdr *up;

				hip = &icp->icmp_ip;
				hlen = hip->ip_hl << 2;
				up = (struct udphdr *)((u_char *)hip + hlen);
				if (hlen + 12 <= cc && hip->ip_p == IPPROTO_UDP &&
#ifdef __FAVOR_BSD
			up->uh_sport == htons(ident) &&
			up->uh_dport == htons(port+seq))
#else
			up->source == htons(ident) &&
			up->dest == htons(port+seq))
#endif
			return (type == ICMP_TIMXCEED? -1 : code+1);
		}
		return(0)
;}
#ifdef notyet
	/*
	 * Checksum routine for Internet Protocol family headers (C Version)
	 */
	u_short
	in_cksum(addr, len)
	u_short *addr;
	int len;
	{
		register int nleft = len;
		register u_short *w = addr;
		register u_short answer;
		register int sum = 0;

		/*
		 *  Our algorithm is simple, using a 32 bit accumulator (sum),
		 *  we add sequential 16 bit words to it, and at the end, fold
		 *  back all the carry bits from the top 16 bits into the lower
		 *  16 bits.
		 */
		while (nleft > 1) {
			sum += *w++;
			nleft -= 2;
		}

		/* mop up an odd byte, if necessary */
		if (nleft == 1)
		sum += *(u_char *)w;

		/*
		 * add back carry outs from top 16 bits to low 16 bits
		 */
		sum = (sum >> 16) + (sum & 0xffff); /* add hi 16 to low 16 */
		sum += (sum >> 16); /* add carry */
		answer = ~sum; /* truncate to 16 bits */
		return (answer);
	}
#endif /* notyet */

	/*
	 * Subtract 2 timeval structs:  out = out - in.
	 * Out is assumed to be >= in.
	 */
voidtvsub (out, in)
	register struct timeval *out, *in; {
	if ((out->tv_usec -= in->tv_usec) < 0) {
		out->tv_sec--;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}

