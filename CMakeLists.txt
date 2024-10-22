cmake_minimum_required(VERSION 3.10) #3.10

# Set project name and version
project(FaceDetection VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find OpenCV package
find_package(OpenCV REQUIRED)

# Include directories
include_directories(${OpenCV_INCLUDE_DIRS})

# Add the executable
add_executable(FaceDetection face_detection_yunet1.cpp)

# Link OpenCV libraries
target_link_libraries(FaceDetection ${OpenCV_LIBS})

# Optionally, specify where to find the YuNet model
set(YUNET_MODEL_PATH "/home/itstarkenn/Downloads/face_detection_yunet_2023mar.onnx")
configure_file(${YUNET_MODEL_PATH} ${CMAKE_CURRENT_BINARY_DIR}/face_detection_yunet_2023mar.onnx COPYONLY)


