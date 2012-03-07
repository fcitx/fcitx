
foreach(size 16 24 32 48 128)
    install(DIRECTORY ${size}x${size} DESTINATION share/icons/hicolor
             PATTERN .* EXCLUDE
             PATTERN *~ EXCLUDE)
endforeach(size 16 22 24 32 48 128)

install(DIRECTORY scabable DESTINATION share/icons/hicolor 
         PATTERN ".*" EXCLUDE
         PATTERN "*~" EXCLUDE)

install(DIRECTORY symbolic/scabable DESTINATION share/icons/gnome
         PATTERN ".*" EXCLUDE
         PATTERN "*~" EXCLUDE )