CFLAGS = -g -O3
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

compile: engine.c
	gcc $(CFLAGS) -o engine engine.c $(LDFLAGS)

.PHONY: test clean

test:	compile
	./engine

clean:
	rm -f VulkanTest
