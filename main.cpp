#include <QtCore/QCoreApplication>

#include <iostream>

#include "definitions.h"
#include "worker.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    std::cout << "cloudGrain ver:" << STR_VERSION << '.' << STR_SUBVERSION << '.' << STR_COMPILATION << std::endl;

    //В този обект се изпълнява цялата логика на програмата. Той прочита настройките,
    //инициализира и периодично обира данните от сензорите и генерира POST
    Worker wrk;

    return a.exec();
}
