#include <stdio.h>
#include "universalProtocolInteractor.h"
#include <unistd.h>
#include <mqueue.h>

int main(int argc, char *argv[]) {
    interactor_t intr = intercatorCreate("/home/mengyou/WorkSpaces/Universal_Protocol_Interactor/test/123.txt", 1024, 1024);
    intercatorRequest(intr, NULL, NULL, NULL, NULL, 100);
    intercatorDelete(intr);
    return EXIT_SUCCESS;
}
