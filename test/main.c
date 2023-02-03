#include <stdio.h>
#include "universalProtocolInteractor.h"
#include <unistd.h>
#include <mqueue.h>

int main(int argc, char *argv[]) {
    interactor_t intr = intercatorCreate("/home/djkj24/WorkSpaces/Universal_Protocol_Interactor/test/123.txt", 1024, 1024);
    intercatorRequest(intr, NULL, 0, NULL, 0, 1);
    intercatorDelete(intr);
    return EXIT_SUCCESS;
}
