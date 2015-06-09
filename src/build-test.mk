# build-test.mk

PROFILES=bt-relay-barebones \
	bt-relay-vanilla \
	bt-relay-all

all: ${PROFILES}

bt-relay-barebones:
	@cp ccn-lite-relay.c ccn-lite-relay.c.backup
	@sed -e 's!#define USE_HMAC256!// #define USE_HMAC256!' <ccn-lite-relay.c >t.c
	@mv t.c ccn-lite-relay.c
	@./build-test-helper.sh $@ ccn-lite-relay
	@mv ccn-lite-relay.c.backup ccn-lite-relay.c

bt-relay-vanilla:
	@./build-test-helper.sh $@ clean ccn-lite-relay

bt-relay-all:
	@cp ccn-lite-relay.c ccn-lite-relay.c.backup
	@sed -e 's!//\s*#define USE_FRAG!#define USE_FRAG!' <ccn-lite-relay.c >t.c
	@mv t.c ccn-lite-relay.c
	@./build-test-helper.sh $@ ccn-lite-relay
	@mv ccn-lite-relay.c.backup ccn-lite-relay.c

