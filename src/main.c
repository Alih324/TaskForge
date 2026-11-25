#include "controller.h"
#include "ui.h"

int main(void)
{
    TaskRegistry registry = {0};

    controller_print_identity();
    return run_ui(&registry);
}
