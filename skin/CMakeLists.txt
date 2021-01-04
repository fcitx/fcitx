
foreach(skin default classic dark)
    add_subdirectory(${skin})
endforeach(skin default classic dark)

install(DIRECTORY imicon DESTINATION ${pkgdatadir}
            PATTERN .* EXCLUDE
            PATTERN *~ EXCLUDE)
