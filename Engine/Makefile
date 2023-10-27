CFLAGS = -O2 -fsanitize=address,undefined -g
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -g

VulkanTest: engine.c
	gcc $(CFLAGS) -o engine engine.c $(LDFLAGS)

.PHONY: test clean

test:	VulkanTest
	./engine

clean:
	rm -f VulkanTest