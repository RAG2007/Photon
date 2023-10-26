CFLAGS = -std=c++17 -O2 -fsanitize=address,undefined
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

VulkanTest: main.c
	g++ $(CFLAGS) -o main main.c $(LDFLAGS)

.PHONY: test clean

test:	VulkanTest
	./main

clean:
	rm -f VulkanTest