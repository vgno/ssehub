cmake_minimum_required (VERSION 2.6)
project (SSEHub)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_C_FLAGS "-Wall")

# glog
find_package( Glog REQUIRED )
include_directories( ${Glog_INCLUDE_PATH} )
link_libraries( ${Glog_LIBRARIES} )

# leveldb
find_package( LevelDB REQUIRED )
include_directories( ${LevelDB_INCLUDE_PATH} )
link_libraries( ${LevelDB_LIBRARIES} )

# snappy
find_package( Snappy REQUIRED )
include_directories( ${Snappy_INCLUDE_DIR} )
link_libraries( ${Snappy_LIBRARIES} )

# rabbitmq
find_package( RabbitMQ REQUIRED )
include_directories( ${RabbitMQ_INCLUDE_DIR} )
link_libraries( ${RabbitMQ_LIBRARIES} )

# Threads
find_package( Threads REQUIRED )
include_directories( ${Threads_INCLUDE_DIR} )

# Boost
find_package( Boost COMPONENTS system program_options thread REQUIRED )
include_directories( ${Boost_INCLUDE_DIR} )
link_libraries( ssehub ${Boost_LIBRARIES} )

# Include local header files
include_directories ("${PROJECT_SOURCE_DIR}/includes" "${PROJECT_SOURCE_DIR}/lib")

add_executable( ssehub lib/picohttpparser/picohttpparser.c src/SSEInputSource.cpp src/InputSources/amqp/AmqpInputSource.cpp src/CacheAdapters/LevelDB.cpp src/CacheAdapters/Redis.cpp src/CacheAdapters/Memory.cpp src/SSEClient.cpp src/SSEClientHandler.cpp src/SSEChannel.cpp src/HTTPRequest.cpp src/HTTPResponse.cpp src/SSEServer.cpp src/SSEConfig.cpp src/SSEEvent.cpp src/SSEStatsHandler.cpp src/main.cpp )

