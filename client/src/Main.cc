#include <Havoc.h>

HcApplication* Havoc = {};

auto main(
    int    argc,
    char** argv
) -> int {
    auto Application = QApplication( argc, argv );

    Havoc = new HcApplication;
    Havoc->Main( argc, argv );

    QApplication::exec();
}

