all: ping

ping:
	cc ping.c -o ping.out

clean:
	rm -f ping.out
