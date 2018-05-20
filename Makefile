
CPP_FLAGS=-I./model-data/opencv/include/
LINK_FLAGS=-lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_highgui

default: register rulerless

register:
	g++ src/register.cpp -o build/register ${CPP_FLAGS} ${LINK_FLAGS} -lopencv_video

rulerless:
	g++ src/rulerless.cpp -o build/rulerless ${CPP_FLAGS} ${LINK_FLAGS} -lopencv_video
