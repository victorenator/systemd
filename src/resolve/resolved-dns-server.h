#pragma once

/***
  This file is part of systemd.

  Copyright 2014 Lennart Poettering

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/

#include "in-addr-util.h"

typedef struct DnsServer DnsServer;

typedef enum DnsServerType {
        DNS_SERVER_SYSTEM,
        DNS_SERVER_FALLBACK,
        DNS_SERVER_LINK,
} DnsServerType;
#define _DNS_SERVER_TYPE_MAX (DNS_SERVER_LINK + 1)

const char* dns_server_type_to_string(DnsServerType i) _const_;
DnsServerType dns_server_type_from_string(const char *s) _pure_;

typedef enum DnsServerFeatureLevel {
        DNS_SERVER_FEATURE_LEVEL_TCP,
        DNS_SERVER_FEATURE_LEVEL_UDP,
        DNS_SERVER_FEATURE_LEVEL_EDNS0,
        DNS_SERVER_FEATURE_LEVEL_DO,
        DNS_SERVER_FEATURE_LEVEL_LARGE,
        _DNS_SERVER_FEATURE_LEVEL_MAX,
        _DNS_SERVER_FEATURE_LEVEL_INVALID = -1
} DnsServerFeatureLevel;

#define DNS_SERVER_FEATURE_LEVEL_WORST 0
#define DNS_SERVER_FEATURE_LEVEL_BEST (_DNS_SERVER_FEATURE_LEVEL_MAX - 1)

const char* dns_server_feature_level_to_string(int i) _const_;
int dns_server_feature_level_from_string(const char *s) _pure_;

#include "resolved-link.h"
#include "resolved-manager.h"

struct DnsServer {
        Manager *manager;

        unsigned n_ref;

        DnsServerType type;
        Link *link;

        int family;
        union in_addr_union address;
        int ifindex; /* for IPv6 link-local DNS servers */

        char *server_string;

        usec_t resend_timeout;
        usec_t max_rtt;

        DnsServerFeatureLevel verified_feature_level;
        DnsServerFeatureLevel possible_feature_level;

        size_t received_udp_packet_max;

        unsigned n_failed_udp;
        unsigned n_failed_tcp;

        bool packet_truncated:1;
        bool packet_bad_opt:1;
        bool packet_rrsig_missing:1;

        usec_t verified_usec;
        usec_t features_grace_period_usec;

        /* Whether we already warned about downgrading to non-DNSSEC mode for this server */
        bool warned_downgrade:1;

        /* Used when GC'ing old DNS servers when configuration changes. */
        bool marked:1;

        /* If linked is set, then this server appears in the servers linked list */
        bool linked:1;
        LIST_FIELDS(DnsServer, servers);
};

int dns_server_new(
                Manager *m,
                DnsServer **ret,
                DnsServerType type,
                Link *link,
                int family,
                const union in_addr_union *address,
                int ifindex);

DnsServer* dns_server_ref(DnsServer *s);
DnsServer* dns_server_unref(DnsServer *s);

void dns_server_unlink(DnsServer *s);
void dns_server_move_back_and_unmark(DnsServer *s);

void dns_server_packet_received(DnsServer *s, int protocol, DnsServerFeatureLevel level, usec_t rtt, size_t size);
void dns_server_packet_lost(DnsServer *s, int protocol, DnsServerFeatureLevel level, usec_t usec);
void dns_server_packet_truncated(DnsServer *s, DnsServerFeatureLevel level);
void dns_server_packet_rrsig_missing(DnsServer *s, DnsServerFeatureLevel level);
void dns_server_packet_bad_opt(DnsServer *s, DnsServerFeatureLevel level);
void dns_server_packet_rcode_downgrade(DnsServer *s, DnsServerFeatureLevel level);

DnsServerFeatureLevel dns_server_possible_feature_level(DnsServer *s);

int dns_server_adjust_opt(DnsServer *server, DnsPacket *packet, DnsServerFeatureLevel level);

const char *dns_server_string(DnsServer *server);
int dns_server_ifindex(const DnsServer *s);

bool dns_server_dnssec_supported(DnsServer *server);

void dns_server_warn_downgrade(DnsServer *server);

DnsServer *dns_server_find(DnsServer *first, int family, const union in_addr_union *in_addr, int ifindex);

void dns_server_unlink_all(DnsServer *first);
void dns_server_unlink_marked(DnsServer *first);
void dns_server_mark_all(DnsServer *first);

DnsServer *manager_get_first_dns_server(Manager *m, DnsServerType t);

DnsServer *manager_set_dns_server(Manager *m, DnsServer *s);
DnsServer *manager_get_dns_server(Manager *m);
void manager_next_dns_server(Manager *m);

bool dns_server_address_valid(int family, const union in_addr_union *sa);

DEFINE_TRIVIAL_CLEANUP_FUNC(DnsServer*, dns_server_unref);

extern const struct hash_ops dns_server_hash_ops;
