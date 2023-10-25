#include <vulkan/vulkan.h>

#include <stdlib.h>
#include <stdio.h>

int init_vulkan() {
	return 0;
}

int main_loop() {
	return 0;
}

int cleanup() {
	return 0;
}

int run() {
	if(init_vulkan() == -1)
		return -1;
	else if(main_loop() == -1)
	main_loop();
	cleanup();
	return 0;
}

int main() {
	if(run() != -1)
		return -1;
	else {
		return 1;
	}
}