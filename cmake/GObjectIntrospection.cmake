include(CMakeParseArguments)
include(FindPkgConfig)

pkg_check_modules(GOBJECT_INTROSPECTION REQUIRED gobject-introspection-1.0)
_pkgconfig_invoke("gobject-introspection-1.0" GOBJECT_INTROSPECTION GIRDIR "" "--variable=girdir")
_pkgconfig_invoke("gobject-introspection-1.0" GOBJECT_INTROSPECTION TYPELIBDIR "" "--variable=typelibdir")

find_program(GIR_SCANNER NAMES g-ir-scanner DOC "g-ir-scanner executable")
mark_as_advanced(GIR_SCANNER)
find_program(GIR_COMPILER NAMES g-ir-compiler DOC "g-ir-compiler executable")
mark_as_advanced(GIR_COMPILER)
find_program(GIR_GENERATE NAMES g-ir-generate DOC "g-ir-generate executable")
mark_as_advanced(GIR_GENERATE)

macro(_gir_list_prefix _newlist _list _prefix)
	set(${_newlist})
	foreach(_item IN LISTS ${_list})
		list(APPEND ${_newlist} ${_prefix}${_item})
	endforeach(_item)
endmacro(_gir_list_prefix)

function(gobject_introspection _FIRST_ARG)
	set(options QUIET VERBOSE)
	set(oneValueArgs
		FILENAME
		FORMAT
		LIBRARY
		NAMESPACE
		NSVERSION
		PROGRAM
		PROGRAM_ARG
        PACKAGE_EXPORT
	)
	set(multiValueArgs
		BUILT_SOURCES
		CFLAGS
		COMPILER_ARGS
		HEADERS
		IDENTIFIER_PREFIXES
		PACKAGES
        INCLUDE
		SCANNER_ARGS
		SOURCES
		SYMBOL_PREFIXES
	)

	CMAKE_PARSE_ARGUMENTS(GIR "${options}" "${oneValueArgs}" "${multiValueArgs}" ${_FIRST_ARG} ${ARGN})

	if(ADD_GIR_UNPARSED_ARGUMENTS)
		message(FATAL_ERROR "Unknown keys given to ADD_GIR_INTROSPECTION(): \"${ADD_GIR_UNPARSED_ARGUMENTS}\"")
	endif(ADD_GIR_UNPARSED_ARGUMENTS)

	###########################################################################
	# make sure that the user set some variables...
	###########################################################################
	if(NOT GIR_FILENAME)
		message(FATAL_ERROR "No gir filename given")
	endif(NOT GIR_FILENAME)

	if(NOT GIR_NAMESPACE)
		# the caller didn't give us a namespace, try to grab it from the filename
		string(REGEX REPLACE "([^-]+)-.*" "\\1" GIR_NAMESPACE "${GIR_FILENAME}")
		if(NOT GIR_NAMESPACE)
			message(FATAL_ERROR "No namespace given and couldn't find one in FILENAME")
		endif(NOT GIR_NAMESPACE)
	endif(NOT GIR_NAMESPACE)

	if(NOT GIR_NSVERSION)
		# the caller didn't give us a namespace version, try to grab it from the filemenu
		string(REGEX REPLACE ".*-([^-]+).gir" "\\1" GIR_NSVERSION "${GIR_FILENAME}")
		if(NOT GIR_NSVERSION)
			message(FATAL_ERROR "No namespace version given and couldn't find one in FILENAME")
		endif(NOT GIR_NSVERSION)
	endif(NOT GIR_NSVERSION)

	if(NOT GIR_CFLAGS)
		get_directory_property(GIR_CFLAGS INCLUDE_DIRECTORIES)
		_gir_list_prefix(GIR_REAL_CFLAGS GIR_CFLAGS "-I")
	endif(NOT GIR_CFLAGS)

	###########################################################################
	# Fix up some of our arguments
	###########################################################################
	if(GIR_VERBOSE)
		set(GIR_VERBOSE "--verbose")
	else(GIR_VERBOSE)
		set(GIR_VERBOSE "")
	endif(GIR_VERBOSE)

	if(GIR_QUIET)
		set(GIR_QUIET "--quiet")
	else(GIR_QUIET)
		set(GIR_QUIET "")
	endif(GIR_QUIET)

	if(GIR_FORMAT)
		set(GIR_FORMAT "--format=${GIR_FORMAT}")
	endif(GIR_FORMAT)

	# if library is set, we need to prepend --library= on to it
	if(GIR_LIBRARY)
		set(GIR_REAL_LIBRARY "--library=${GIR_LIBRARY}")
	endif(GIR_LIBRARY)

	# if program has been set, we prepend --program= on to it
	if(GIR_PROGRAM)
		set(GIR_PROGRAM "--program=${GIR_PROGRAM}")
	endif(GIR_PROGRAM)

	# if program_arg has been set, we prepend --program-arg= on to it
	if(GIR_PROGRAM_ARG)
		set(GIR_PROGRAM_ARG "--program-arg=${GIR_PROGRAM_ARG}")
	endif(GIR_PROGRAM_ARG)

    # if the user specified PACKAGE_EXPORT we need to prefix each with --pkg-export
    if(GIR_PACKAGE_EXPORT)
        set(GIR_REAL_PACKAGE_EXPORT GIR_PACKAGE_EXPORT "--pkg-export=")
    endif(GIR_PACKAGE_EXPORT)

	###########################################################################
	# Clean up any of the multivalue items that all need to be prefixed
	###########################################################################

	# if the user specified IDENTIFIER_PREFIXES we need to prefix each with --identifier-prefix
	if(GIR_IDENTIFIER_PREFIXES)
		_gir_list_prefix(GIR_REAL_IDENTIFIER_PREFIXES GIR_IDENTIFIER_PREFIXES "--identifier-prefix=")
	endif(GIR_IDENTIFIER_PREFIXES)

	# if the user specified SYMBOL_PREFIXES we need to prefix each with --symbol-prefix=
	if(GIR_SYMBOL_PREFIXES)
		_gir_list_prefix(GIR_REAL_SYMBOL_PREFIXES GIR_SYMBOL_PREFIXES "--symbol-prefix=")
	endif(GIR_SYMBOL_PREFIXES)

	# if the user specified PACKAGES we need to prefix each with --pkg
	if(GIR_PACKAGES)
		_gir_list_prefix(GIR_REAL_PACKAGES GIR_PACKAGES "--pkg=")
	endif(GIR_PACKAGES)

    # if the user specified PACKAGES we need to prefix each with --pkg
    if(GIR_INCLUDE)
        _gir_list_prefix(GIR_REAL_INCLUDE GIR_INCLUDE "--include=")
    endif(GIR_INCLUDE)

	# if the user specified BUILT_SOURCES, we need to get their paths since
	# they could be in CMAKE_CURRENT_BUILD_DIR
	if(GIR_BUILT_SOURCES)
		set(GIR_REAL_BUILT_SOURCES)

		foreach(ITEM ${GIR_BUILT_SOURCES})
			get_source_file_property(LOCATION ${ITEM} LOCATION)
			list(APPEND GIR_REAL_BUILT_SOURCES "${LOCATION}")
		endforeach(ITEM)
	endif(GIR_BUILT_SOURCES)

	###########################################################################
	# Add the custom commands
	###########################################################################
	set(ENV{CFLAGS} ${GIR_REAL_CFLAGS})
	add_custom_command(
		COMMAND ${GIR_SCANNER} ${GIR_SCANNER_ARGS}
			--namespace=${GIR_NAMESPACE}
			--nsversion=${GIR_NSVERSION}
			${GIR_REAL_CFLAGS}
			${GIR_FORMAT}
			${GIR_REAL_LIBRARY}
			${GIR_PROGRAM} ${GIR_PROGRAM_ARGS}
			${GIR_QUIET} ${GIR_VERBOSE}
			${GIR_REAL_IDENTIFIER_PREFIXES}
			${GIR_REAL_SYMBOL_PREFIXES}
			${GIR_REAL_PACKAGES}
            ${GIR_REAL_INCLUDE}
			--no-libtool
			-L${CMAKE_CURRENT_BINARY_DIR}
			--output=${CMAKE_CURRENT_BINARY_DIR}/${GIR_FILENAME}
            ${GIR_PACKAGE_EXPORT}
            ${GIR_SCANNER_FLAGS}
			${GIR_SOURCES}
			${GIR_REAL_BUILT_SOURCES}
		OUTPUT ${GIR_FILENAME}
		DEPENDS ${GIR_LIBRARY}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		VERBATIM
	)

	add_custom_target(${GIR_FILENAME}.target ALL DEPENDS ${GIR_LIBRARY} ${GIR_FILENAME})

	# create the name of the typelib
	string(REPLACE ".gir" ".typelib" GIR_TYPELIB "${GIR_FILENAME}")

	add_custom_command(
		COMMAND ${GIR_COMPILER} ${GIR_COMPILER_ARGS}
			${CMAKE_CURRENT_BINARY_DIR}/${GIR_FILENAME}
			--output=${CMAKE_CURRENT_BINARY_DIR}/${GIR_TYPELIB}
		OUTPUT ${GIR_TYPELIB}
		DEPENDS ${GIR_FILENAME}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	)

	add_custom_target(${GIR_TYPELIB}.target ALL DEPENDS ${GIR_LIBRARY} ${GIR_FILENAME} ${GIR_TYPELIB})
endfunction(gobject_introspection)

