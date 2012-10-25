
foreach(skin default classic dark)
    install(DIRECTORY ${skin} DESTINATION ${pkgdatadir}/skin
             PATTERN .* EXCLUDE
             PATTERN *~ EXCLUDE)
endforeach(skin default classic dark)

install(DIRECTORY imicon DESTINATION ${pkgdatadir}
            PATTERN .* EXCLUDE
            PATTERN *~ EXCLUDE)
