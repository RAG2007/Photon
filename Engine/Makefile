CFLAGS = -g -O3 -fsanitize=address,undefined
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

VulkanTest: engine.c
	gcc $(CFLAGS) -o engine engine.c $(LDFLAGS)

.PHONY: test clean

test:	VulkanTest
	./engine

clean:
	rm -f VulkanTest