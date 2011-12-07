
foreach(size 16 24 32 48 128)
    install(DIRECTORY ${size}x${size} DESTINATION share/icons/hicolor PATTERN .* PATTERN *~ EXCLUDE)
endforeach(size 16 22 24 32 48 128)

install(DIRECTORY scabable DESTINATION share/icons/hicolor PATTERN .* PATTERN *~ EXCLUDE)