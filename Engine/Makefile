CFLAGS = -O3
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

compile: engine.c
	gcc $(CFLAGS) -o engine engine.c $(LDFLAGS)

.PHONY: test clean

test:	compile
	ASAN_OPTIONS=fast_unwind_on_malloc=0 ./engine

clean:
	rm -f VulkanTest
