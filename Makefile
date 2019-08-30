chunky: linenoise.h linenoise.c chunky.c
	$(CC) -Wall -W -Os -g -o chunky linenoise.c chunky.c

linenoise_example: linenoise.h linenoise.c example.c
	$(CC) -Wall -W -Os -g -o linenoise_example linenoise.c example.c

clean:
	rm -f chunky linenoise_example
