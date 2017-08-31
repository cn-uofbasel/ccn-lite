cmake_minimum_required(VERSION 3.7)
project(ccnl-lxkernel)


SET( MODULE_NAME
        ccnl-lxkernel
        )

SET( MODULE_SOURCE_FILES
        ccn-lite-lnxkernel.c
        /home/blacksheeep/ccn-lite/src/ccnl-fwd/src/ccnl-dispatch.c
        )

SET( MODULE_FILE
        ${MODULE_NAME}.ko
        )

SET( MODULE_OUTPUT_DIR
        ${CMAKE_BINARY_DIR}/modules/${MODULE_NAME}
        )

SET( KERNEL_DIR
        "/lib/modules/${CMAKE_SYSTEM_VERSION}/build"
        )

SET( KBUILD_COMMAND
	${CMAKE_MAKE_PROGRAM} -C ${KERNEL_DIR} M=${MODULE_OUTPUT_DIR} modules
)

CONFIGURE_FILE(
        Kbuild.in
        ${MODULE_OUTPUT_DIR}/Kbuild
)

FOREACH( MODULE_SOURCE_FILE ${MODULE_SOURCE_FILES} )
    CONFIGURE_FILE(
            ${MODULE_SOURCE_FILE}
            ${MODULE_OUTPUT_DIR}/${MODULE_SOURCE_FILE}
            COPYONLY
    )
ENDFOREACH( MODULE_SOURCE_FILE )


ADD_CUSTOM_COMMAND(
        OUTPUT ${MODULE_OUTPUT_DIR}/${MODULE_FILE}
        COMMAND ${KBUILD_COMMAND}
        WORKING_DIRECTORY ${MODULE_OUTPUT_DIR}
        DEPENDS ${MODULE_SOURCE_FILES} Kbuild.in
        VERBATIM
)

ADD_CUSTOM_TARGET(
        ${MODULE_NAME}
        ALL
        DEPENDS ${MODULE_OUTPUT_DIR}/${MODULE_FILE}
)
