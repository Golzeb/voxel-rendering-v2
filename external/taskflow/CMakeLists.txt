include(FetchContent)

FetchContent_Declare(
  taskflow
  GIT_REPOSITORY https://github.com/taskflow/taskflow.git
  GIT_TAG master
  GIT_SHALLOW TRUE
)

set(TF_BUILD_TESTS OFF)
set(TF_BUILD_EXAMPLES OFF)

FetchContent_MakeAvailable(taskflow)
