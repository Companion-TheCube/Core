# Increment Build Number Script (incr_build_num.cmake)

# make sure we always run the entire script
set_property(DIRECTORY PROPERTY EP_STEP_TARGETS incr_build_num)

# Define path to the build number file
set(BUILD_NUMBER_FILE "${CMAKE_SOURCE_DIR}/../build_number.txt")

# Check if the file exists, and read the value
if(EXISTS "${BUILD_NUMBER_FILE}")
    file(READ "${BUILD_NUMBER_FILE}" BUILD_NUMBER)
else()
    set(BUILD_NUMBER 0)
endif()

# Convert the build number to an integer
math(EXPR BUILD_NUMBER "${BUILD_NUMBER}")

# Increment the build number
math(EXPR BUILD_NUMBER "${BUILD_NUMBER} + 1")

# Write the updated build number to the file
file(WRITE "${BUILD_NUMBER_FILE}" "${BUILD_NUMBER}")

# Output the build number
message(STATUS "Build Number: ${BUILD_NUMBER}")