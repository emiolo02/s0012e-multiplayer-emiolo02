#--------------------------------------------------------------------------
# engine
#--------------------------------------------------------------------------
ADD_LIBRARY(engine INTERFACE)
TARGET_INCLUDE_DIRECTORIES(engine INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

FILE(GLOB children RELATIVE ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/*)
FOREACH (child ${children})
    IF (IS_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/${child})
        FILE(GLOB_RECURSE sources ${child}/*.h ${child}/*.cc)
        ADD_LIBRARY(${child} ${sources})
        SOURCE_GROUP(${child} ${sources})
        #TARGET_INCLUDE_DIRECTORIES(${child} PUBLIC "${child}")

        TARGET_LINK_LIBRARIES(${child} PUBLIC engine)
        TARGET_LINK_LIBRARIES(engine INTERFACE ${child})
    ENDIF ()
ENDFOREACH ()

TARGET_LINK_LIBRARIES(engine INTERFACE exts pch)