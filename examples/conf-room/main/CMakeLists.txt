idf_component_register(
    SRC_DIRS
    "."
    "gui"
    "rmaker"

    INCLUDE_DIRS
    "."
    "gui"
    "rmaker"
    )

target_compile_definitions(${COMPONENT_TARGET} PRIVATE "-D RMAKER_DEMO_PROJECT_NAME=\"${CMAKE_PROJECT_NAME}\"")

