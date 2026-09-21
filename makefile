all: hub sender receiver

hub: hub.c
	gcc -Wall -Wextra -g $< -o $@

sender: link.c test_sender.c
	gcc -Wall -Wextra -g $^ -o $@

receiver: link.c test_receiver.c
	gcc -Wall -Wextra -g $^ -o $@

clean:
	rm -f hub sender receiver *.o
