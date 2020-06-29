CC = gcc
CFLAGS = -g
LDFLAGS = -lstdc++ -lpthread -ldl -g

BMSDK = /usr/local/include

UNAME_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
ifeq ($(UNAME_S),Linux)
	ifneq ("$(wildcard /home/linuxbrew/.linuxbrew/include/DeckLinkAPIDispatch.cpp)","")
		BMSDK = /home/linuxbrew/.linuxbrew/include
	else
		ifneq ("$(wildcard ~/.linuxbrew/include/DeckLinkAPIDispatch.cpp)","")
			BMSDK = ~/.linuxbrew/include
		endif
	endif
endif
ifeq ($(UNAME_S),Darwin)
	LDFLAGS += -framework CoreFoundation
endif

CFLAGS += -I${BMSDK}

deckcontrol: deckcontrol.o DeckLinkAPIDispatch.o
	$(CC) $(LDFLAGS) -o deckcontrol deckcontrol.o DeckLinkAPIDispatch.o

deckcontrol.o: deckcontrol.cpp
	$(CC) $(CFLAGS) -c deckcontrol.cpp

DeckLinkAPIDispatch.o: $(BMSDK)/DeckLinkAPIDispatch.cpp
	$(CC) $(CFLAGS) -c $(BMSDK)/DeckLinkAPIDispatch.cpp

clean:
	rm -f *.o deckcontrol
