#include <stdio.h>
#include "universalProtocolInteractor.h"
#include <unistd.h>
#include <mqueue.h>
interactor_t gintr = NULL;
interactor_t gintr2 = NULL;
bool filter(interactor_t intr, uint8_t *buf, size_t len) {
    if (intr == gintr) {
        printf("intr1\n");
        fflush(stdout);
    } else if (intr == gintr2) {
        printf("intr2\n");
        fflush(stdout);
    } else {
        return false;
    }
    return true;
}

void callback(uint8_t *buf, size_t len) {
    printf("recv: %s",buf);
    fflush(stdout);
}

upi_urm_export(filter,callback);

int main(int argc, char *argv[]) {
    gintr = intercatorCreate("/home/djkj24/WorkSpaces/Universal_Protocol_Interactor/test/123.txt", 1024, 1024);
    gintr2 = intercatorCreate("/home/djkj24/WorkSpaces/Universal_Protocol_Interactor/test/123.txt", 1024, 1024);
    intercatorRequest(gintr, NULL, 0, NULL, NULL, 5000);
    sleep(2);
    intercatorDelete(gintr);
    intercatorDelete(gintr2);
    return EXIT_SUCCESS;
}
