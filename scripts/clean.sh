CLEANTYPE=$1

clean() {
    rm -rf build
    rm -f vgcore.*
    rm -rf *.log
}

if [ "$CLEANTYPE" = "clean" ]; then
    clean
elif [ "$CLEANTYPE" = "fclean" ]; then
    clean
    rm -f r-type_server
    rm -f r-type_client
    # Clean the correct library based on platform
    rm -f libr-type_game.so libr-type_game..dylib
    rm -f libr-type_renderer.so libr-type_renderer..dylib
    rm -f libr-type_game_server.so libr-type_game_server.dylib
    rm -f libr-type_engine.so libr-type_engine.dylib
    rm -f libr-type_engine_server.so libr-type_engine_server.dylib
fi