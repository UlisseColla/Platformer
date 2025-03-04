NAME= Platformer
SRC= Platformer.cpp

FLAGS= -W -W -W -pthread -std=c++11

all: ${NAME}

${NAME}: ${SRC}
	@c++ ${FLAGS} ${SRC} -o ${NAME}
	@echo "Compiled succesfully"
