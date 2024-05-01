test:
	@echo "Compiling shaders"
	bash ./Engine/shaders/compile.sh
	@echo "Compiling engine.c"
	gcc ./Engine/engine.c -o ./Engine/engine -fsanitize=address,undefined -g -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
	./Engine/engine
build:
	@echo "Compiling engine.c"
	gcc ./Engine/engine.c -o ./Engine/engine -O3 -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
