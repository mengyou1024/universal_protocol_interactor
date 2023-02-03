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
    // mq_unlink("/intr00");
    // mq_unlink("/intr01");
    // mq_unlink("/intrMq");
    gintr = interactorCreate("/home/djkj24/WorkSpaces/Universal_Protocol_Interactor/test/123.txt", 1024, NULL);
    gintr2 = interactorCreate("/home/djkj24/WorkSpaces/Universal_Protocol_Interactor/test/123.txt", 1024, NULL);
    interactorRequest(gintr, "123123", 6, NULL, NULL, 5000);
    sleep(2);
    interactorDelete(gintr);
    interactorDelete(gintr2);
    return EXIT_SUCCESS;
}
