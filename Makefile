BMSDK = ../Blackmagic\ DeckLink\ SDK\ 10.1.1/Mac/include

CC = gcc
CFLAGS = -I$(BMSDK) -g
LDFLAGS = -lstdc++ -lpthread -framework CoreFoundation -g

deckcontrol: deckcontrol.o DeckLinkAPIDispatch.o
	$(CC) $(LDFLAGS) -o deckcontrol deckcontrol.o DeckLinkAPIDispatch.o

deckcontrol.o: deckcontrol.cpp
	$(CC) $(CFLAGS) -c deckcontrol.cpp

DeckLinkAPIDispatch.o: $(BMSDK)/DeckLinkAPIDispatch.cpp
	$(CC) $(CFLAGS) -c $(BMSDK)/DeckLinkAPIDispatch.cpp

clean:
	rm -f *.o deckcontrol
