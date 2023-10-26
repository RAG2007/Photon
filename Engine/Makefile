CFLAGS = -O2 -fsanitize=address,undefined
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -g

VulkanTest: main.c
	gcc $(CFLAGS) -o main main.c $(LDFLAGS)

.PHONY: test clean

test:	VulkanTest
	./main

clean:
	rm -f VulkanTest